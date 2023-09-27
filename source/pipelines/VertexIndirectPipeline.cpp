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
			.vertexOffset = static_cast<int32_t>(mShared->mMeshData[i].mVertexOffset),	// Note: Not strictly necessary, we could also set it to 0 and pull the vertex offset from the mesh buffer
			.firstInstance = static_cast<uint32_t>(i),							// we missuse that such that we know where to access the mesh array in the shader
		};
	}
	mIndirectDrawCommandBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::indirect_buffer_meta::create_from_data(gpuDrawCommands),
		avk::storage_buffer_meta::create_from_data(gpuDrawCommands)
	);
	avk::context().record_and_submit_with_fence({
			mIndirectDrawCommandBuffer->fill(gpuDrawCommands.data(), 0)
		}, *queue
	)->wait_until_signalled();

	mPipeline = avk::context().create_graphics_pipeline_for(
		avk::vertex_shader("shaders/transform_and_pass_pulled_pos_nrm_uv.vert"),
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
		avk::descriptor_binding(4, 1, mShared->mMeshesBuffer)
	);
	// TODO HOT RELOAD!
}

avk::command::action_type_command VertexIndirectPipeline::render(int64_t inFlightIndex)
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
					descriptor_binding(4, 1, mShared->mMeshesBuffer)
				})),

				// TODO GET RID OF PUSH CONSTANTS
				command::push_constants(mPipeline->layout(), push_constants{
					true,
					0,
					0
				}),
		});
}

void VertexIndirectPipeline::doDestroy()
{
	mIndirectDrawCommandBuffer = avk::buffer();
}
