#include "MeshletRiggedCompression.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"

#include "../meshletbuilder/MeshletbuilderInterface.h"

void MeshletRiggedCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(mWithShuffle, mWithMerge, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		auto& newVert = mVertexData.emplace_back(vertex_data_meshlet_coding{});

		newVert.mPosition = encodeVec3ToUVec2(glm::vec3(vert.mPositionTxX.x, vert.mPositionTxX.y, vert.mPositionTxX.z));
		newVert.mNormal = packNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
		newVert.mTexCoords = packTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
	}

	auto meshlets = mShared->getCurrentMeshletBuilder()->getMeshletsNative();
	uint32_t overflowedLuidCount = 0;
	uint32_t overflowedMeshletCount = 0;
	for (uint32_t mid = 0; mid < meshlets.size(); mid++) {
		uint32_t currentMbiIndex = 0;
		auto& mlt = meshlets[mid];
		auto& newMltData = mAdditionalMeshletData.emplace_back();
		uint32_t overflowedLuidCount_before = overflowedLuidCount;
		for (auto vid : mlt.mVertices) {
			auto& vert = mShared->mVertexData[vid];
			auto& luid = vertexLUIndexTable[vid];
			uint32_t mbiluid = UINT32_MAX;
			// check if luid already in mbitable
			for (uint32_t tmpMbiluid = 0; tmpMbiluid < currentMbiIndex; tmpMbiluid++) {
				if (luid == newMltData.mMbiTable[tmpMbiluid]) {
					mbiluid = tmpMbiluid;
					break;
				}
			}
			if (mbiluid == UINT32_MAX) {
				// In this case its not in the table so we have to newly add it
				// but first make sure wether we still can (max 4 per meshlet)
				if (currentMbiIndex == 4) {
					overflowedLuidCount++;
					//std::cerr << "Can't add luid " << luid << " of vertex " << vid << " to meshlet " << mid << " since all luid spots are occupied." << std::endl;
					mbiluid = 0; // falsely set to the first one (There will be graphical errors)
				}
				else {
					mbiluid = currentMbiIndex;
					newMltData.mMbiTable[currentMbiIndex] = luid;
					currentMbiIndex++;
				}
			}
			// for v1 lets just set mBoneIndicesLUID.z to the mbiluid value
			//mVertexData[vid].mBoneIndicesLUID.z = mbiluid;
			mVertexData[vid].mWeightsImbiluidPermutation = PermutationCodec::encode(
				vert.mBoneWeights, 
				packMbiluidAndPermutation(mbiluid, vertexLUPermutation[vid])
			);
		}
		if (overflowedLuidCount > overflowedLuidCount_before) overflowedMeshletCount++;
	}
	if (overflowedLuidCount > 0) {
		std::cerr << overflowedLuidCount << " / " << mShared->mVertexData.size() << " vertices luids out of bound affecting "
			<< overflowedMeshletCount << " / " << meshlets.size() << " meshlets." << std::endl;
	}

	mAdditionalMeshletBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mAdditionalMeshletData)
	);
	avk::context().record_and_submit_with_fence({ mAdditionalMeshletBuffer->fill(mAdditionalMeshletData.data(), 0) }, *queue)->wait_until_signalled();

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
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 2, mAdditionalMeshletBuffer));

	mShared->mPropertyManager->get("lut_count")->setUint(mBoneLUTData.size());
	mShared->mPropertyManager->get("lut_size")->setUint(mBoneLUTData.size() * sizeof(glm::u16vec4));
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(vertex_data_meshlet_coding) * mVertexData.size());
	mShared->mPropertyManager->get("amb_size")->setUint(sizeof(mrc_per_meshlet_data) * mAdditionalMeshletData.size());
}

void MeshletRiggedCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mAdditionalMeshletData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
	mAdditionalMeshletBuffer = avk::buffer();
}

void MeshletRiggedCompression::hud_config(bool& config_has_changed)
{
	ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
	if (mWithShuffle) {
		ImGui::Checkbox("LUT with Merging", &mWithMerge);
	}
}
