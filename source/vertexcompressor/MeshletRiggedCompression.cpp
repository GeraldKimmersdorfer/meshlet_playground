#include "MeshletRiggedCompression.h"
#include "lut_helper.h"
#include <imgui.h>
#include "../helpers/packing_helper.h"
#include "PermutationCodec.h"

void MeshletRiggedCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		//glm::vec4 boneWeights = vert.mBoneWeights;
		//if (mWithShuffle) boneWeights = applyPermutation(boneWeights, vertexLUPermutation[vid]);
		auto& newVert = mVertexData.emplace_back(vertex_data_meshlet_coding{});

		
		newVert.mNormal = glm::vec4(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w, 0.0f);
		newVert.mTexCoord = glm::vec4(vert.mPositionTxX.w, vert.mTxYNormal.x, 0.0f, 0.0f);
		newVert.mBoneWeights = vert.mBoneWeights;
		newVert.mBoneIndicesLUID = glm::uvec4(static_cast<uint32_t>(vertexLUIndexTable[vid]),0,0,0);

		newVert.mPosition = glm::uvec4(
			(static_cast<uint32_t>(vert.mPositionTxX[0] * 0xFFFF) << 16u) | static_cast<uint32_t>(vert.mPositionTxX[1] * 0xFFFF),
			(static_cast<uint32_t>(vert.mPositionTxX[2] * 0xFFFF) << 16u),
			packNormal(newVert.mNormal),
			packTextureCoords(newVert.mTexCoord)
		);

		newVert.mBoneIndicesLUID.y = PermutationCodec::encode(vert.mBoneWeights, static_cast<uint16_t>(newVert.mBoneIndicesLUID.x));
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

void MeshletRiggedCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void MeshletRiggedCompression::hud_config(bool& config_has_changed)
{
	//ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
}
