#include "MeshPipeline.h"
#include <vk_convenience_functions.hpp>
#include "../packing_helper.h"
#include "../shadercompiler/ShaderMetaCompiler.h"

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
	auto builder = mShared->getCurrentMeshletBuilder();
	builder->generate();
	if (mMeshletType.first == _NATIVE) {
		auto& meshletsNative = builder->getMeshletsNative();
		mMeshletsBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(meshletsNative));
		avk::context().record_and_submit_with_fence({ mMeshletsBuffer->fill(meshletsNative.data(), 0) }, *queue)->wait_until_signalled();
		mShared->mConfig.mMeshletsCount = meshletsNative.size();
	}
	else if (mMeshletType.first == _REDIR) {
		auto& [meshletsRedirect, redirectIndexBuffer] = builder->getMeshletsRedirect();
		mMeshletsBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(meshletsRedirect));
		avk::context().record_and_submit_with_fence({ mMeshletsBuffer->fill(meshletsRedirect.data(), 0) }, *queue)->wait_until_signalled();
		mPackedIndexBuffer = avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_data(redirectIndexBuffer));
		avk::context().record_and_submit_with_fence({ mPackedIndexBuffer->fill(redirectIndexBuffer.data(), 0) }, *queue)->wait_until_signalled();
		mShared->mConfig.mMeshletsCount = meshletsRedirect.size();
	}

	mTaskInvocations = mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredTaskWorkGroupInvocations;
	mMeshInvocations = mMeshletExtension.first == _NV ? mShared->mPropsMeshShaderNV.maxMeshWorkGroupInvocations : mShared->mPropsMeshShader.maxPreferredMeshWorkGroupInvocations;
	
	mShared->uploadConfig();

	if (mMeshletType.first == _NATIVE) {
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::task_shader(mPathTaskShader, "main", true)
			.set_specialization_constant(0, mTaskInvocations),
			avk::mesh_shader(mPathMeshShader, "main", true)
			.set_specialization_constant(0, mTaskInvocations)
			.set_specialization_constant(1, mMeshInvocations),
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
			.set_specialization_constant(0, mTaskInvocations),
			avk::mesh_shader(mPathMeshShader, "main", true)
			.set_specialization_constant(0, mTaskInvocations)
			.set_specialization_constant(1, mMeshInvocations), 
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
	mPipeline = avk::graphics_pipeline();
	mMeshletsBuffer = avk::buffer();
	mPackedIndexBuffer = avk::buffer();
}
