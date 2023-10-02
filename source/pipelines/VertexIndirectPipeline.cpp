#include "VertexIndirectPipeline.h"


VertexIndirectPipeline::VertexIndirectPipeline(SharedData* shared)
	:PipelineInterface(shared, "Indirect")
{
}

void VertexIndirectPipeline::doInitialize(avk::queue* queue)
{
	auto gpuDrawCommands = std::vector<VkDrawIndexedIndirectCommand>(mShared->mMeshData.size());
	for (int i = 0; i < gpuDrawCommands.size(); i++) {
		gpuDrawCommands[i] = VkDrawIndexedIndirectCommand{
			.indexCount = mShared->mMeshData[i].mIndexCount,
			.instanceCount = 1,
			.firstIndex = mShared->mMeshData[i].mIndexOffset,
			.vertexOffset = static_cast<int32_t>(mShared->mMeshData[i].mVertexOffset),
			.firstInstance = static_cast<uint32_t>(i),							// we missuse this property such that we know where to access the mesh array in the shader
		};
	}
	mIndirectDrawCommandBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::indirect_buffer_meta::create_from_data(gpuDrawCommands),
		avk::storage_buffer_meta::create_from_data(gpuDrawCommands)
	);
	avk::context().record_and_submit_with_fence({ mIndirectDrawCommandBuffer->fill(gpuDrawCommands.data(), 0)}, *queue)->wait_until_signalled();

	auto offsetForTexcoord = sizeof(glm::vec3);
	auto offsetForNormal = offsetForTexcoord + sizeof(glm::vec2);
	auto offsetForBoneIndices = offsetForNormal + sizeof(glm::vec3);
	auto offsetForBoneWeight = offsetForBoneIndices + sizeof(glm::uvec4);

	mPipeline = avk::context().create_graphics_pipeline_for(
		avk::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
		avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
		avk::from_buffer_binding(0) -> stream_per_vertex(0, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(0), //stream_per_vertex<glm::vec3>()->to_location(0),
		avk::from_buffer_binding(0) -> stream_per_vertex(offsetForTexcoord, vk::Format::eR32G32Sfloat, sizeof(vertex_data))->to_location(1),
		avk::from_buffer_binding(0) -> stream_per_vertex(offsetForNormal, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(2),
		avk::from_buffer_binding(0) -> stream_per_vertex(offsetForBoneIndices, vk::Format::eR32G32B32A32Uint, sizeof(vertex_data))->to_location(3),
		avk::from_buffer_binding(0) -> stream_per_vertex(offsetForBoneWeight, vk::Format::eR32G32B32A32Sfloat, sizeof(vertex_data))->to_location(4),
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
		avk::descriptor_binding(4, 1, mShared->mMeshesBuffer)
	);
	mShared->mSharedUpdater->on(avk::shader_files_changed_event(mPipeline.as_reference())).update(mPipeline);
}

avk::command::action_type_command VertexIndirectPipeline::render(int64_t inFlightIndex)
{
	using namespace avk;
	return command::render_pass(mPipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(mPipeline.as_reference()),
				command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, as_combined_image_samplers(mShared->mImageSamplers, layout::shader_read_only_optimal)),
					descriptor_binding(0, 1, mShared->mViewProjBuffers[inFlightIndex]),
					descriptor_binding(0, 2, mShared->mConfigurationBuffer),
					descriptor_binding(1, 0, mShared->mMaterialsBuffer),
					descriptor_binding(2, 0, mShared->mBoneTransformBuffers[inFlightIndex]),
					descriptor_binding(4, 1, mShared->mMeshesBuffer)
				})),

				command::draw_indexed_indirect(
					mIndirectDrawCommandBuffer.as_reference(),
					mShared->mIndexBuffer.as_reference(),
					mShared->mMeshData.size(),
					mShared->mVertexBuffer.as_reference()
				),
		});
}

void VertexIndirectPipeline::hud_config(bool& config_has_changed)
{
	config_has_changed |= ImGui::Checkbox("Highlight meshes", (bool*)(void*)&mShared->mConfig.mOverlayMeshlets);
}

void VertexIndirectPipeline::doDestroy()
{
	mIndirectDrawCommandBuffer = avk::buffer();
	mPositionsBuffer = avk::buffer();
	mTexCoordsBuffer = avk::buffer();
	mNormalsBuffer = avk::buffer();
	mBoneIndicesBuffer = avk::buffer();
	mBoneWeightsBuffer = avk::buffer();
}
