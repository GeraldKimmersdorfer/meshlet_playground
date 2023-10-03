#include "VertexIndirectPipeline.h"
#include "../shadercompiler/ShaderMetaCompiler.h"

// its not in Auto-VK. Yet?
avk::command::action_type_command draw_indexed_indirect_nobind(const avk::buffer_t& aParametersBuffer, const avk::buffer_t& aIndexBuffer, uint32_t aNumberOfDraws, vk::DeviceSize aParametersOffset, uint32_t aParametersStride)
{
	using namespace avk::command;
	const auto& indexMeta = aIndexBuffer.template meta<avk::index_buffer_meta>();
	vk::IndexType indexType;
	switch (indexMeta.sizeof_one_element()) {
		case sizeof(uint16_t) : indexType = vk::IndexType::eUint16; break;
			case sizeof(uint32_t) : indexType = vk::IndexType::eUint32; break;
			default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
	}

	return action_type_command{
		avk::sync::sync_hint {
			{{ // DESTINATION dependencies for previous commands:
				vk::PipelineStageFlagBits2KHR::eAllGraphics,
				vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
			}},
			{{ // SOURCE dependencies for subsequent commands:
				vk::PipelineStageFlagBits2KHR::eAllGraphics,
				vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
			}}
		},
		{}, // no resource-specific sync hints
		[
			indexType,
			lParametersBufferHandle = aParametersBuffer.handle(),
			lIndexBufferHandle = aIndexBuffer.handle(),
			aNumberOfDraws, aParametersOffset, aParametersStride
		](avk::command_buffer_t& cb) {
			cb.handle().bindIndexBuffer(lIndexBufferHandle, 0u, indexType);
			cb.handle().drawIndexedIndirect(lParametersBufferHandle, aParametersOffset, aNumberOfDraws, aParametersStride);
		}
	};
}

VertexIndirectPipeline::VertexIndirectPipeline(SharedData* shared)
	:PipelineInterface(shared, "Vertex Indirect")
{
}

void VertexIndirectPipeline::doInitialize(avk::queue* queue)
{
	if (mShadersRecompiled) {
		mVertexGatherType.first = mVertexGatherType.second;
		mShadersRecompiled = false;
	}
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

	if (mVertexGatherType.first == _PUSH) {
		auto offsetForTexcoord = sizeof(glm::vec3);
		auto offsetForNormal = offsetForTexcoord + sizeof(glm::vec2);
		auto offsetForBoneIndices = offsetForNormal + sizeof(glm::vec3);
		auto offsetForBoneWeight = offsetForBoneIndices + sizeof(glm::uvec4);

		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader(mPathVertexShader),
			avk::fragment_shader(mPathFragmentShader),
			avk::from_buffer_binding(0)->stream_per_vertex(0, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(0), //stream_per_vertex<glm::vec3>()->to_location(0),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForTexcoord, vk::Format::eR32G32Sfloat, sizeof(vertex_data))->to_location(1),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForNormal, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(2),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForBoneIndices, vk::Format::eR32G32B32A32Uint, sizeof(vertex_data))->to_location(3),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForBoneWeight, vk::Format::eR32G32B32A32Sfloat, sizeof(vertex_data))->to_location(4),
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
	}
	else if (mVertexGatherType.first == _PULL) {
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader(mPathVertexShader),
			avk::fragment_shader(mPathFragmentShader),
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
			avk::descriptor_binding(4, 1, mShared->mMeshesBuffer)
		);
	}
}

avk::command::action_type_command VertexIndirectPipeline::render(int64_t inFlightIndex)
{
	using namespace avk;
	return command::render_pass(mPipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(mPipeline.as_reference()),
				command::conditional(
					[this]() { return mVertexGatherType.first == _PUSH; },
					[this, inFlightIndex]() {
						return command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
							descriptor_binding(0, 0, as_combined_image_samplers(mShared->mImageSamplers, layout::shader_read_only_optimal)),
							descriptor_binding(0, 1, mShared->mViewProjBuffers[inFlightIndex]),
							descriptor_binding(0, 2, mShared->mConfigurationBuffer),
							descriptor_binding(1, 0, mShared->mMaterialsBuffer),
							descriptor_binding(2, 0, mShared->mBoneTransformBuffers[inFlightIndex]),
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
							descriptor_binding(4, 1, mShared->mMeshesBuffer)
						}));
					}
				),
				command::conditional(
					[this]() { return mVertexGatherType.first == _PUSH; },
					[this]() { return command::draw_indexed_indirect(mIndirectDrawCommandBuffer.as_reference(), mShared->mIndexBuffer.as_reference(), mShared->mMeshData.size(), mShared->mVertexBuffer.as_reference()); },
					[this]() { return draw_indexed_indirect_nobind(mIndirectDrawCommandBuffer.as_reference(), mShared->mIndexBuffer.as_reference(), mShared->mMeshData.size(), 0, static_cast<uint32_t>(sizeof(VkDrawIndexedIndirectCommand))); }
				)
		});
}

void VertexIndirectPipeline::hud_config(bool& config_has_changed)
{
	config_has_changed |= ImGui::Checkbox("Highlight meshes", (bool*)(void*)&mShared->mConfig.mOverlayMeshlets);
}

void VertexIndirectPipeline::hud_setup(bool& config_has_changes)
{
	ImGui::Combo("Vertex Acquisition", (int*)(void*)&(mVertexGatherType.second), "Push\0Pull\0");
}

void VertexIndirectPipeline::compile()
{
	mPathVertexShader = ShaderMetaCompiler::precompile("vertex.vert", {
		{"VERTEX_GATHER_TYPE", MCC_to_string(mVertexGatherType.second)}
	});
	mPathFragmentShader = ShaderMetaCompiler::precompile("diffuse_shading_fixed_lightsource.frag", {});
	mShadersRecompiled = true;
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
