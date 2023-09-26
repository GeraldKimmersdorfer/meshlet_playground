#include "MeshNvNoCompressionPipeline.h"
#include <vk_convenience_functions.hpp>

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


MeshNvNoCompressionPipeline::MeshNvNoCompressionPipeline(SharedData* shared)
	:PipelineInterface(shared)
{
}

void MeshNvNoCompressionPipeline::doInitialize(avk::queue* queue)
{
	for (int meshIndex = 0; meshIndex < mShared->mMeshData.size(); meshIndex++) {
		// create selection for the meshlets
		auto meshletSelection = avk::make_models_and_mesh_indices_selection(mShared->mModel, meshIndex);

		// TODO ALLOW POSSIBILITY FOR DIFFERENT MESHLET CREATORS
		std::vector<avk::meshlet> cpuMeshlets;
		cpuMeshlets = std::move(avk::divide_into_meshlets(meshletSelection, meshlet_division_meshoptimizer, true, sNumVertices, sNumIndices - ((sNumIndices / 3) % 4) * 3));

		auto [gpuMeshlets, _] = avk::convert_for_gpu_usage<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(cpuMeshlets);

		for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
			auto& genMeshlet = gpuMeshlets[mshltidx];
			mMeshlets.push_back(meshlet{
				.mMeshIndex = static_cast<uint32_t>(meshIndex),
				.mGeometry = genMeshlet
				}
			);
		}
	}
	mMeshletsBuffer = avk::context().create_buffer(
		avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mMeshlets)
	);
	avk::context().record_and_submit_with_fence({
		mMeshletsBuffer->fill(mMeshlets.data(), 0)
		}, *queue)->wait_until_signalled();

	mShowMeshletsTo = static_cast<int>(mMeshlets.size());
	mTaskInvocationsNv = mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations;

	mPipeline = avk::context().create_graphics_pipeline_for(
		avk::task_shader("shaders/meshlet.nv.task")
		.set_specialization_constant(0, mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations),
		avk::mesh_shader("shaders/meshlet.nv.mesh")
		.set_specialization_constant(0, mShared->mPropsMeshShaderNV.maxTaskWorkGroupInvocations)
		.set_specialization_constant(1, mShared->mPropsMeshShaderNV.maxMeshWorkGroupInvocations),
		avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
		avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
		avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
		avk::context().create_renderpass({
			avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
			avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
		avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(push_constants) },
		avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mShared->mImageSamplers, avk::layout::shader_read_only_optimal)),
		avk::descriptor_binding(0, 1, mShared->mViewProjBuffers[0]),
		avk::descriptor_binding(1, 0, mShared->mMaterialsBuffer),
		avk::descriptor_binding(2, 0, mShared->mBoneTransformBuffers[0]),
		avk::descriptor_binding(3, 0, mShared->mVertexBuffer),
		avk::descriptor_binding(4, 0, mMeshletsBuffer),
		avk::descriptor_binding(4, 1, mShared->mMeshesBuffer)
	);
	// TODO HOT RELOAD!
}

avk::command::action_type_command MeshNvNoCompressionPipeline::render(int64_t inFlightIndex)
{
	using namespace avk;
	return command::render_pass(mPipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(mPipeline.as_reference()),
				command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, as_combined_image_samplers(mShared->mImageSamplers, layout::shader_read_only_optimal)),
					descriptor_binding(0, 1, mShared->mViewProjBuffers[inFlightIndex]),
					descriptor_binding(1, 0, mShared->mMaterialsBuffer),
					descriptor_binding(2, 0, mShared->mBoneTransformBuffers[inFlightIndex]),
					descriptor_binding(3, 0, mShared->mVertexBuffer),
					descriptor_binding(4, 0, mMeshletsBuffer),
					descriptor_binding(4, 1, mShared->mMeshesBuffer)
				})),

				command::push_constants(mPipeline->layout(), push_constants{
					mHighlightMeshlets,
					static_cast<int32_t>(mShowMeshletsFrom),
					static_cast<int32_t>(mShowMeshletsTo)
				}),
				command::draw_mesh_tasks_nv(div_ceil(mMeshlets.size(), mTaskInvocationsNv), 0)
		});
}

void MeshNvNoCompressionPipeline::hud()
{
	// Select the range of meshlets to be rendered:
	ImGui::Checkbox("Highlight meshlets", &mHighlightMeshlets);
	ImGui::Text("Select meshlets to be rendered:");
	ImGui::DragIntRange2("Visible range", &mShowMeshletsFrom, &mShowMeshletsTo, 1, 0, static_cast<int>(mMeshlets.size()));
}

void MeshNvNoCompressionPipeline::doDestroy()
{
	mMeshlets.clear();
	mPipeline = avk::graphics_pipeline();
	mMeshletsBuffer = avk::buffer();
}
