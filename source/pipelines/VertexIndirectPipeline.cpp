#include "VertexIndirectPipeline.h"

#include "imgui.h"
#include "../shadercompiler/ShaderMetaCompiler.h"
#include "../avk_extensions.hpp"

#include "../vertexcompressor/VertexCompressionInterface.h"

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

	return avk::command::action_type_command{
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
	compile();
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

	avk::graphics_pipeline_config pipelineConfig;

	if (mVertexGatherType.first == _PUSH) {
		auto offsetForTexcoord = sizeof(glm::vec3);
		auto offsetForNormal = offsetForTexcoord + sizeof(glm::vec2);
		auto offsetForBoneIndices = offsetForNormal + sizeof(glm::vec3);
		auto offsetForBoneWeight = offsetForBoneIndices + sizeof(glm::uvec4);

		pipelineConfig = avk::create_graphics_pipeline_config(
			avk::vertex_shader(mPathVertexShader),
			avk::fragment_shader(mPathFragmentShader),
			avk::from_buffer_binding(0)->stream_per_vertex(0, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(0),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForTexcoord, vk::Format::eR32G32Sfloat, sizeof(vertex_data))->to_location(1),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForNormal, vk::Format::eR32G32B32Sfloat, sizeof(vertex_data))->to_location(2),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForBoneIndices, vk::Format::eR32G32B32A32Uint, sizeof(vertex_data))->to_location(3),
			avk::from_buffer_binding(0)->stream_per_vertex(offsetForBoneWeight, vk::Format::eR32G32B32A32Sfloat, sizeof(vertex_data))->to_location(4),
			avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(copy_push_data) }
		);

	}
	else if (mVertexGatherType.first == _PULL) {
		pipelineConfig = avk::create_graphics_pipeline_config(
			avk::vertex_shader(mPathVertexShader),
			avk::fragment_shader(mPathFragmentShader),
			avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(copy_push_data) }
		);

		auto vCompressor = mShared->getCurrentVertexCompressor();
		vCompressor->compress(queue);
		auto vertexBindings = vCompressor->getBindings();
		mAdditionalStaticDescriptorBindings.insert(mAdditionalStaticDescriptorBindings.end(), vertexBindings.begin(), vertexBindings.end());

	}
	mShared->attachSharedPipelineConfiguration(&pipelineConfig, &mAdditionalStaticDescriptorBindings);


	// Add static descriptor bindings to pipeline definition
	for (auto& db : mAdditionalStaticDescriptorBindings) {
		pipelineConfig.mResourceBindings.push_back(std::move(db));
	}
	mPipeline = avk::context().create_graphics_pipeline(std::move(pipelineConfig));

}

avk::command::action_type_command VertexIndirectPipeline::render(int64_t inFlightIndex)
{
	using namespace avk;
	return command::render_pass(mPipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(mPipeline.as_reference()),
				command::bind_descriptors(mPipeline->layout(), mShared->mDescriptorCache->get_or_create_descriptor_sets(
					avk::mergeVectors(
						std::vector<avk::binding_data>{
							std::move(avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mShared->mImageSamplers, avk::layout::shader_read_only_optimal))), // I CANT MOVE THAT TO mShared->getDynamicDescriptorBindings and its making me mad...
						},
						mAdditionalStaticDescriptorBindings,
						mShared->getDynamicDescriptorBindings(inFlightIndex)
					))),
				command::custom_commands([this](avk::command_buffer_t& cb) {
					if (mVertexGatherType.first == _PUSH) {
						for (int i = 0; i < mShared->mConfig.mCopyCount; i++) {
							cb.record({
								avk::command::push_constants(mPipeline->layout(), mShared->getCopyDataForId(i)),
								command::draw_indexed_indirect(mIndirectDrawCommandBuffer.as_reference(), mShared->mIndexBuffer.as_reference(), mShared->mMeshData.size(), mShared->mVertexBuffer.as_reference())
								});
						}
					}
					else {
						for (int i = 0; i < mShared->mConfig.mCopyCount; i++) {
							cb.record({
								avk::command::push_constants(mPipeline->layout(), mShared->getCopyDataForId(i)),
								draw_indexed_indirect_nobind(mIndirectDrawCommandBuffer.as_reference(), mShared->mIndexBuffer.as_reference(), mShared->mMeshData.size(), 0, static_cast<uint32_t>(sizeof(VkDrawIndexedIndirectCommand))),
								});
						}
					}
				}),
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
		{"VERTEX_GATHER_TYPE", MCC_to_string(mVertexGatherType.second)},
		{"VERTEX_COMPRESSION", mShared->getCurrentVertexCompressor()->getMccId()}
	});
	mPathFragmentShader = ShaderMetaCompiler::precompile("diffuse_shading_fixed_lightsource.frag", {});
	mShadersRecompiled = true;
}

void VertexIndirectPipeline::doDestroy()
{
	mShared->getCurrentVertexCompressor()->destroy();
	mPipeline = avk::graphics_pipeline();
	mIndirectDrawCommandBuffer = avk::buffer();
	mAdditionalStaticDescriptorBindings.clear();
}
