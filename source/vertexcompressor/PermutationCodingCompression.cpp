#include "PermutationCodingCompression.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"

#include <glm/gtc/epsilon.hpp>

void PermutationCodingCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		//glm::vec4 boneWeights = vert.mBoneWeights;
		//if (mWithShuffle) boneWeights = applyPermutation(boneWeights, vertexLUPermutation[vid]);
		auto& newVert = mVertexData.emplace_back(vertex_data_permutation_coding{});

		newVert.mPosition = encodeVec3ToUVec2(glm::vec3(vert.mPositionTxX.x, vert.mPositionTxX.y, vert.mPositionTxX.z));
		auto decoded = decodeUVec2ToVec3(newVert.mPosition);
		/*if (!glm::all(glm::epsilonEqual(decoded, glm::vec3(vert.mPositionTxX.x, vert.mPositionTxX.y, vert.mPositionTxX.z), 0.0001f))) {
			std::cout << "FEHLER";
		}*/
		
		newVert.mNormal = packNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
		newVert.mTexCoords = packTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
		newVert.mBoneData = PermutationCodec::encode(vert.mBoneWeights, vertexLUIndexTable[vid]);
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

	// report to props:
	//mShared->mPropertyLutSize->setValue(mBoneLUTData.size() * sizeof(glm::u16vec4));
	//mShared->mPropertyLutCount->setValue(mBoneLUTData.size());
	//mShared->mPropertyVbSize->setValue(sizeof(vertex_data_permutation_coding) * mVertexData.size());
}

void PermutationCodingCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void PermutationCodingCompression::hud_config(bool& config_has_changed)
{
	//ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
}
