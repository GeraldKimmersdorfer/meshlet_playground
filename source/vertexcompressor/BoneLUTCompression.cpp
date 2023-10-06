#include "BoneLUTCompression.h"
#include "lut_helper.h"
#include <imgui.h>


void BoneLUTCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(mWithShuffle, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		glm::vec4 boneWeights = vert.mBoneWeights;
		if (mWithShuffle) boneWeights = applyPermutation(boneWeights, vertexLUPermutation[vid]);
		mVertexData.push_back(vertex_data_bone_lookup{
			.mPositionTxX = vert.mPositionTxX,
			.mTxYNormal = vert.mTxYNormal,
			.mBoneWeights = glm::vec3(boneWeights),
			.mBoneIndicesLUID = static_cast<uint32_t>(vertexLUIndexTable[vid])
			});
	}
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device,
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mVertexData.data(), 0) }, *queue)->wait_until_signalled();

	mBoneLUTBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mBoneLUTData)
	);
	avk::context().record_and_submit_with_fence({ mBoneLUTBuffer->fill(mBoneLUTData.data(), 0) }, *queue)->wait_until_signalled();

	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 0, mVertexBuffer));
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 1, mBoneLUTBuffer));
}

void BoneLUTCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void BoneLUTCompression::hud_config(bool& config_has_changed)
{
	ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
}
