#include "MeshPipeline.h"
#include <vk_convenience_functions.hpp>
#include "../packing_helper.h"
#include "../shadercompiler/ShaderMetaCompiler.h"

auto meshlet_division_meshoptimizer = [](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices, const avk::model_t& aModel, std::optional<avk::mesh_index_t> aMeshIndex, uint32_t aMaxVertices, uint32_t aMaxIndices) {
	// definitions
	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	// get the maximum number of meshlets that could be generated
	size_t max_meshlets = meshopt_buildMeshletsBound(aIndices.size(), aMaxVertices, max_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	// let meshoptimizer build the meshlets for us
	size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
		aIndices.data(), aIndices.size(), &tVertices[0].x, tVertices.size(), sizeof(glm::vec3),
		aMaxVertices, max_triangles, cone_weight);

	// copy the data over to Auto-Vk-Toolkit's meshlet structure
	std::vector<avk::meshlet> generatedMeshlets(meshlet_count);
	generatedMeshlets.resize(meshlet_count);
	generatedMeshlets.reserve(meshlet_count);
	for (int k = 0; k < meshlet_count; k++) {
		auto& m = meshlets[k];
		auto& gm = generatedMeshlets[k];
		gm.mIndexCount = m.triangle_count * 3;
		gm.mVertexCount = m.vertex_count;
		gm.mVertices.reserve(m.vertex_count);
		gm.mVertices.resize(m.vertex_count);
		gm.mIndices.reserve(gm.mIndexCount);
		gm.mIndices.resize(gm.mIndexCount);
		std::ranges::copy(meshlet_vertices.begin() + m.vertex_offset,
			meshlet_vertices.begin() + m.vertex_offset + m.vertex_count,
			gm.mVertices.begin());
		std::ranges::copy(meshlet_triangles.begin() + m.triangle_offset,
			meshlet_triangles.begin() + m.triangle_offset + gm.mIndexCount,
			gm.mIndices.begin());
	}
	return generatedMeshlets;
	};


MeshPipeline::MeshPipeline(SharedData* shared)
	:PipelineInterface(shared, "Meshlet")
{
}

void MeshPipeline::doInitialize(avk::queue* queue)
{
	if (mShadersRecompiled) {
		mMeshletExtension.first = mMeshletExtension.second;
		mMeshletType.first = mMeshletType.second;
		mShadersRecompiled = false;
	}
	if (mMeshletType.first == _NATIVE) {
		for (size_t meshIndex = 0; meshIndex < mShared->mMeshData.size(); meshIndex++) {
			auto meshletSelection = avk::make_models_and_mesh_indices_selection(mShared->mModel, meshIndex);
			auto cpuMeshlets = std::move(avk::divide_into_meshlets(meshletSelection, meshlet_division_meshoptimizer, true, sNumVertices, sNumIndices - ((sNumIndices / 3) % 4) * 3));

			auto [gpuMeshlets, _] = avk::convert_for_gpu_usage<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(cpuMeshlets);

			for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
				auto& genMeshlet = gpuMeshlets[mshltidx];
				auto& natMeshlet = mMeshletsNative.emplace_back(meshlet_native{
					.mMeshIdxVcTc = packMeshIdxVcTc(meshIndex, genMeshlet.mVertexCount, genMeshlet.mPrimitiveCount)
					}
				);
				std::copy(&genMeshlet.mVertices[0], &genMeshlet.mVertices[static_cast<uint32_t>(genMeshlet.mVertexCount)], &natMeshlet.mVertices[0]);
				memcpy(&natMeshlet.mIndicesPacked[0], &genMeshlet.mIndices[0], genMeshlet.mPrimitiveCount * 3);	// good old memcpy so that i dont have to deal with black magic typecasting...
			}
		}
		mMeshletsBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(mMeshletsNative));
		avk::context().record_and_submit_with_fence({ mMeshletsBuffer->fill(mMeshletsNative.data(), 0) }, *queue)->wait_until_signalled();
		mShared->mConfig.mMeshletsCount = mMeshletsNative.size();
	}
	else if (mMeshletType.first == _REDIR) {
		for (size_t meshIndex = 0; meshIndex < mShared->mMeshData.size(); meshIndex++) {
			auto meshletSelection = avk::make_models_and_mesh_indices_selection(mShared->mModel, meshIndex);
			auto cpuMeshlets = std::move(avk::divide_into_meshlets(meshletSelection, meshlet_division_meshoptimizer, true, sNumVertices, sNumIndices - ((sNumIndices / 3) % 4) * 3));

			auto [gpuMeshlets, gpuIndicesData] = avk::convert_for_gpu_usage<avk::meshlet_redirected_gpu_data, sNumVertices, sNumIndices>(cpuMeshlets);

			for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
				auto& genMeshlet = gpuMeshlets[mshltidx];
				auto& meshlet = mMeshletsRedirect.emplace_back(meshlet_redirect{
					.mDataOffset = genMeshlet.mDataOffset + static_cast<uint32_t>(mPackedIndices.size()), 	// add packed indices size because we only have one index array as oposed to one for each model
					.mMeshIdxVcTc = packMeshIdxVcTc(meshIndex, genMeshlet.mVertexCount, genMeshlet.mPrimitiveCount)
					}
				);
			}
			mPackedIndices.insert(mPackedIndices.end(), gpuIndicesData->begin(), gpuIndicesData->end());
		}
		mMeshletsBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(mMeshletsRedirect));
		avk::context().record_and_submit_with_fence({ mMeshletsBuffer->fill(mMeshletsRedirect.data(), 0) }, *queue)->wait_until_signalled();
		mPackedIndexBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(mPackedIndices));
		avk::context().record_and_submit_with_fence({ mPackedIndexBuffer->fill(mPackedIndices.data(), 0) }, *queue)->wait_until_signalled();
		mShared->mConfig.mMeshletsCount = mMeshletsRedirect.size();
	}

	mTaskInvocations = mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations;
	mMeshInvocations = mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxMeshWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredMeshWorkGroupInvocations;
	
	mShared->uploadConfig();

	if (mMeshletType.first == _NATIVE) {
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::task_shader(mPathTaskShader, "main", true)
			.set_specialization_constant(0, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations),
			avk::mesh_shader(mPathMeshShader, "main", true)
			.set_specialization_constant(0, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations)
			.set_specialization_constant(1, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxMeshWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredMeshWorkGroupInvocations),
			avk::fragment_shader(mPathFragmentShader, "main", true),
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			avk::context().create_renderpass({
				avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
				avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
				}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
			avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mShared->mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(0, 1, mShared->mViewProjBuffers[0]),
			avk::descriptor_binding(0, 2, mShared->mConfigurationBuffer),
			avk::descriptor_binding(1, 0, mShared->mMaterialsBuffer),
			avk::descriptor_binding(2, 0, mShared->mBoneTransformBuffers[0]),
			avk::descriptor_binding(3, 0, mShared->mVertexBuffer),
			avk::descriptor_binding(4, 0, mMeshletsBuffer),
			avk::descriptor_binding(4, 1, mShared->mMeshesBuffer)
		);
	}
	else if (mMeshletType.first == _REDIR) {
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::task_shader(mPathTaskShader, "main", true)
			.set_specialization_constant(0, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations),
			avk::mesh_shader(mPathMeshShader, "main", true)
			.set_specialization_constant(0, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations)
			.set_specialization_constant(1, mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxMeshWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredMeshWorkGroupInvocations),
			avk::fragment_shader(mPathFragmentShader, "main", true),
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			avk::context().create_renderpass({
				avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
				avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
				}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
			avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mShared->mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(0, 1, mShared->mViewProjBuffers[0]),
			avk::descriptor_binding(0, 2, mShared->mConfigurationBuffer),
			avk::descriptor_binding(1, 0, mShared->mMaterialsBuffer),
			avk::descriptor_binding(2, 0, mShared->mBoneTransformBuffers[0]),
			avk::descriptor_binding(3, 0, mShared->mVertexBuffer),
			avk::descriptor_binding(4, 0, mMeshletsBuffer),
			avk::descriptor_binding(4, 1, mShared->mMeshesBuffer),
			avk::descriptor_binding(4, 2, mPackedIndexBuffer)
		);
	}

}

avk::command::action_type_command MeshPipeline::render(int64_t inFlightIndex)
{
	using namespace avk;
	return command::render_pass(mPipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(mPipeline.as_reference()),
				command::conditional(
					[this]() { return mMeshletType.first == _NATIVE; },
					[this, inFlightIndex]() { 
						return command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
							descriptor_binding(0, 0, as_combined_image_samplers(mShared->mImageSamplers, layout::shader_read_only_optimal)),
							descriptor_binding(0, 1, mShared->mViewProjBuffers[inFlightIndex]),
							descriptor_binding(0, 2, mShared->mConfigurationBuffer),
							descriptor_binding(1, 0, mShared->mMaterialsBuffer),
							descriptor_binding(2, 0, mShared->mBoneTransformBuffers[inFlightIndex]),
							descriptor_binding(3, 0, mShared->mVertexBuffer),
							descriptor_binding(4, 0, mMeshletsBuffer),
							descriptor_binding(4, 1, mShared->mMeshesBuffer)
						}));
					},
					[this, inFlightIndex]() { 
						return command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
							descriptor_binding(0, 0, as_combined_image_samplers(mShared->mImageSamplers, layout::shader_read_only_optimal)),
							descriptor_binding(0, 1, mShared->mViewProjBuffers[inFlightIndex]),
							descriptor_binding(0, 2, mShared->mConfigurationBuffer),
							descriptor_binding(1, 0, mShared->mMaterialsBuffer),
							descriptor_binding(2, 0, mShared->mBoneTransformBuffers[inFlightIndex]),
							descriptor_binding(3, 0, mShared->mVertexBuffer),
							descriptor_binding(4, 0, mMeshletsBuffer),
							descriptor_binding(4, 1, mShared->mMeshesBuffer),
							descriptor_binding(4, 2, mPackedIndexBuffer)
						}));
					}
				),
				command::conditional(
					[this]() { return mMeshletExtension.first == _NV; },
					[this]() { return command::draw_mesh_tasks_nv(div_ceil(mShared->mConfig.mMeshletsCount, mTaskInvocations), 0); },
					[this]() { return command::draw_mesh_tasks_ext(div_ceil(mShared->mConfig.mMeshletsCount, mTaskInvocations), 1, 1); }
				)
				
		});
}

void MeshPipeline::hud_config(bool& config_has_changed)
{
	config_has_changed |= ImGui::Checkbox("Highlight meshlets", (bool*)(void*)&mShared->mConfig.mOverlayMeshlets);
}

void MeshPipeline::hud_setup(bool& config_has_changes)
{
	ImGui::Combo("Extension", (int*)(void*)&(mMeshletExtension.second), "EXT\0NV\0");
	ImGui::Combo("Meshlet-Type", (int*)(void*)&(mMeshletType.second), "Native\0Redirected\0");
}

void MeshPipeline::compile()
{
	mPathTaskShader = ShaderMetaCompiler::precompile("meshlet.task", { 
		{"MESHLET_EXTENSION", MCC_to_string(mMeshletExtension.second)}
		});
	mPathMeshShader = ShaderMetaCompiler::precompile("meshlet.mesh", { 
		{"MESHLET_EXTENSION", MCC_to_string(mMeshletExtension.second)},
		{"MESHLET_TYPE", MCC_to_string(mMeshletType.second)}
		});
	mPathFragmentShader = ShaderMetaCompiler::precompile("diffuse_shading_fixed_lightsource.frag", {});
	mShadersRecompiled = true;
}

void MeshPipeline::doDestroy()
{
	mMeshletsNative.clear();
	mMeshletsRedirect.clear();
	mPackedIndices.clear();
	mPipeline = avk::graphics_pipeline();
	mMeshletsBuffer = avk::buffer();
	mPackedIndexBuffer = avk::buffer();
}
