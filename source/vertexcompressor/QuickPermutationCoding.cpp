/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "QuickPermutationCoding.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"

#include <glm/gtc/epsilon.hpp>

#include "../meshletbuilder/MeshletbuilderInterface.h"

void QuickPermutationCoding::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(mWithReuse, false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	std::vector<cpu_compressed_vertex_data> compressedVertexData;
	compressedVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		auto& newVert = compressedVertexData.emplace_back(cpu_compressed_vertex_data{});
		newVert.position = glm::u16vec3(
			static_cast<uint16_t>(vert.mPositionTxX[0] * 0xFFFF),
			static_cast<uint16_t>(vert.mPositionTxX[1] * 0xFFFF),
			static_cast<uint16_t>(vert.mPositionTxX[2] * 0xFFFF)
		);
		newVert.normal = compressNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
		newVert.texCoord = compressTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
		newVert.boneAttributes = PermutationCodec::encode(vert.mBoneWeights, vertexLUIndexTable[vid], PermutationCodec::codec16bit);
	}



	// Optionally sort vertex data in the order of the first appearance in a meshlet
	if (mSortVertexDataInMeshletOrder) {
		std::vector<meshlet_native> newMeshlets;
		newMeshlets = mShared->getCurrentMeshletBuilder()->getMeshletsNative();

		std::vector<bool> vertexInNewList(compressedVertexData.size(), false);
		std::map<uint32_t, uint32_t> oldVidToNewVid;
		std::vector<cpu_compressed_vertex_data> sortedVertexData;
		sortedVertexData.reserve(compressedVertexData.size());
		// Go through all meshlets
		for (int mid = 0; mid < newMeshlets.size(); mid++) {
			auto& currMeshlet = newMeshlets[mid];
			uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
			unpackMeshIdxVcTc(currMeshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);

			// Go through all vertices of the meshlet and add them to the sorted list if they are not already in there
			// also change the vertex id in the meshlet to the new id
			for (int i = 0; i < vertexCount; i++) {
				const auto& oldvid = currMeshlet.mVertices[i];
				if (!vertexInNewList[oldvid]) {
					vertexInNewList[oldvid] = true;
					oldVidToNewVid[oldvid] = static_cast<uint32_t>(sortedVertexData.size());
					sortedVertexData.push_back(compressedVertexData[oldvid]);
				}
				currMeshlet.mVertices[i] = oldVidToNewVid[oldvid];
			}
		}
		compressedVertexData = sortedVertexData;

		mShared->getCurrentMeshletBuilder()->overwriteMeshlets(newMeshlets);
	}

	mVertexData.clear();
	// copy to uint8_t vector
	mVertexData.insert(mVertexData.end(), reinterpret_cast<uint8_t*>(compressedVertexData.data()), reinterpret_cast<uint8_t*>(compressedVertexData.data() + compressedVertexData.size()));

	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
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
	mShared->mPropertyManager->get("lut_count")->setUint(mBoneLUTData.size());
	mShared->mPropertyManager->get("lut_size")->setUint(mBoneLUTData.size() * sizeof(glm::u16vec4));
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(uint8_t) * mVertexData.size());
	mShared->mPropertyManager->get("emb_size")->setUint(0);
}

void QuickPermutationCoding::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void QuickPermutationCoding::hud_config(bool& config_has_changed)
{
	ImGui::Checkbox("LUT with Luid-Reuse", &mWithReuse);
	ImGui::Checkbox("Sort Vertex Data in Meshlet Order", &mSortVertexDataInMeshletOrder);
}
