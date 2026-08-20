/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "PDifferenceCodec32.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"
#include "../meshletbuilder/MeshletbuilderInterface.h"
#include <glm/gtx/string_cast.hpp>

void PDifferenceCodec32::doCompress(avk::queue* queue)
{
	// Step 1: Create LUT Table
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(mWithReuse, false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	// Step 2: Create Compressed VertexTable:
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
		newVert.originalBoneAttributes = glm::u16vec4(
			vertexLUIndexTable[vid],
			naiveWeightEncode(vert.mBoneWeights)
		);
	}
	auto meshletBuilder = mShared->getCurrentMeshletBuilder();
	const std::vector<meshlet_native>& meshlets = meshletBuilder->getMeshletsNative();

	// NEXT CHECK: Lets check wether vertices are accessed in different meshlets! With our encoding scheme that can't be the case
	// because the vertex data will only contain the offset to the median value of the meshlet. If thats the case, we'll have
	// to copy the vertex for both meshlets. But first lets see if thats actually necessary.
	std::map<uint32_t, std::vector<uint32_t>> mVertexReferences;	// containes for each vertex the meshlet ids it is referenced
	for (uint32_t mltid = 0; mltid < meshlets.size(); mltid++) {
		auto& meshlet = meshlets[mltid];
		std::map<uint32_t, uint32_t> mVertexReferencesForMeshlet;
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(meshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
		auto& mesh = mShared->mMeshData[meshIndex];
		for (uint32_t mltvid = 0; mltvid < vertexCount; mltvid++) {
			const auto vid = meshlet.mVertices[mltvid];
			auto it = mVertexReferencesForMeshlet.find(vid);
			if (it == mVertexReferencesForMeshlet.end()) mVertexReferencesForMeshlet[vid] = 1;
			else it->second++;
		}

		// Check wether same vertex is in meshlet twice (shouldnt happen) and copy into big meshletReferenceMap
		for (auto& pair : mVertexReferencesForMeshlet) {
			if (pair.second > 1) {
				LOG_S(ERROR) << "Vertex " << pair.first << " is referenced " << pair.second <<
					" times inside the same meshlet. That shouldn't happen!";
			}
			const auto vid = pair.first;
			mVertexReferences[vid].push_back(mltid);
		}
	}

	uint32_t uncompressedBitCount{ sizeof(cpu_compressed_vertex_data) * 8 };
	std::vector<cpu_compressed_vertex_data> minimumVertexDataForMeshlets;
	std::vector<cpu_compressed_vertex_data> bitCountsForMeshlets;
	for (uint32_t mltid = 0; mltid < meshlets.size(); mltid++) {
		auto& meshlet = meshlets[mltid];
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(meshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);

		// GATHER COMPRESSED VERTEX DATA FOR THIS MESHLET
		std::map<uint32_t, cpu_compressed_vertex_data> vertexDataForMeshlet;
		for (uint32_t mltvid = 0; mltvid < vertexCount; mltvid++) {
			const auto vid = meshlet.mVertices[mltvid];
			vertexDataForMeshlet[vid] = compressedVertexData[vid];
		}

		// DITCH THE MEDIAN OR AVERAGE: I WILL TRY TO SAVE THE MINIMUM AS I ONLY HAVE TO SAVE POSITIVE VALUES FOR THE DIFFERENCES THEN!
		cpu_compressed_vertex_data minimums{
			{UINT16_MAX, UINT16_MAX, UINT16_MAX}, {UINT16_MAX, UINT16_MAX}, {UINT16_MAX, UINT16_MAX}, UINT32_MAX
		};
		for (const auto& vdata : vertexDataForMeshlet) {
			const auto& cmlt = vdata.second;
			minimums.position = glm::min(minimums.position, cmlt.position);
			minimums.normal = glm::min(minimums.normal, cmlt.normal);
			minimums.texCoord = glm::min(minimums.texCoord, cmlt.texCoord);
			minimums.boneAttributes = glm::min(minimums.boneAttributes, cmlt.boneAttributes);
		}
		minimumVertexDataForMeshlets.push_back(minimums);

		cpu_compressed_vertex_data maxDifferences{
			{0,0,0}, {0,0}, {0,0}, 0
		};
		for (const auto& vdata : vertexDataForMeshlet) {
			const auto& cmlt = vdata.second;
			cpu_compressed_vertex_data mltDiff = {
				cmlt.position - minimums.position,
				cmlt.normal - minimums.normal,
				cmlt.texCoord - minimums.texCoord,
				cmlt.boneAttributes - minimums.boneAttributes
			};
			maxDifferences.position = glm::max(maxDifferences.position, mltDiff.position);
			maxDifferences.normal = glm::max(maxDifferences.normal, mltDiff.normal);
			maxDifferences.texCoord = glm::max(maxDifferences.texCoord, mltDiff.texCoord);
			maxDifferences.boneAttributes = glm::max(maxDifferences.boneAttributes, mltDiff.boneAttributes);
		}

		cpu_compressed_vertex_data bitCounts{
			glm::u16vec3(glm::ceil(glm::log2(glm::vec3(maxDifferences.position) + glm::vec3(1.0)))),
			glm::u16vec2(glm::ceil(glm::log2(glm::vec2(maxDifferences.normal) + glm::vec2(1.0)))),
			glm::u16vec2(glm::ceil(glm::ceil(glm::log2(glm::vec2(maxDifferences.texCoord) + glm::vec2(1.0))))),
			glm::u32vec1(glm::ceil(glm::log2(glm::vec1(maxDifferences.boneAttributes + 1)))).x
		};

		// Optional: Pad to 8 byte boundaries (just comment out if you dont want that) (but we probably need to as 8 bit storage is the minimum that well get on the gpu)
		bitCounts.position = glm::ceil(glm::vec3(bitCounts.position) / glm::vec3(32.0f)) * glm::vec3(32.0f);
		bitCounts.normal = glm::ceil(glm::vec2(bitCounts.normal) / glm::vec2(32.0f)) * glm::vec2(32.0f);
		bitCounts.texCoord = glm::ceil(glm::vec2(bitCounts.texCoord) / glm::vec2(32.0f)) * glm::vec2(32.0f);
		bitCounts.boneAttributes = (glm::ceil(glm::vec1(bitCounts.boneAttributes) / glm::vec1(32.0f)) * glm::vec1(32.0f)).x;

		bitCountsForMeshlets.push_back(bitCounts);
	}

	// NEXT CHECK: Lets check wether vertices are accessed in different meshlets and have different bone data.
	std::vector<meshlet_native> newMeshlets;
	{
		uint32_t splitCounter = 0; uint32_t nonSplitCounter = 0;
		std::copy(meshlets.begin(), meshlets.end(), std::back_inserter(newMeshlets));
		for (auto& pair : mVertexReferences) {
			if (pair.second.size() > 1) {
				bool compatible = true;
				for (int j = 0; j < pair.second.size(); j++) {
					const auto& firstMinValues = minimumVertexDataForMeshlets[pair.second[j]];
					const auto& firstBitCounts = bitCountsForMeshlets[pair.second[j]];
					for (int i = j + 1; i < pair.second.size(); i++) {
						const auto& refMinValues = minimumVertexDataForMeshlets[pair.second[i]];
						const auto& refBitCounts = bitCountsForMeshlets[pair.second[i]];
						if (refBitCounts.boneAttributes != firstBitCounts.boneAttributes) {
							compatible = false;
							break;
						}
					}
				}
				// TODO: It would be better to only split up the combinations that are actually not compatible
				// Right now if two out of three are not compatible we split up in 3

				if (!compatible) {
					//LOG_S(INFO) << "Vertex " + std::to_string(pair.first) + " is referenced by " + std::to_string(pair.second.size()) +
					//	" different meshlets, and the meshlet mins are not compatible. I'll try to split it up.";
					splitCounter++;
					// Split up the vertex
					const auto& vid = pair.first;
					const auto& meshletIds = pair.second;
					for (int i = 1; i < meshletIds.size(); i++) {
						const uint32_t newVid = static_cast<uint32_t>(compressedVertexData.size());
						compressedVertexData.push_back(compressedVertexData[vid]);
						// now change the reference in the meshlet
						auto& meshlet = newMeshlets[meshletIds[i]];
						uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
						unpackMeshIdxVcTc(meshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
						int changeCounter = 0;
						for (int j = 0; j < vertexCount; j++) {
							if (meshlet.mVertices[j] == vid) {
								meshlet.mVertices[j] = newVid;
								changeCounter++;
							}
						}
						if (changeCounter != 1) {
							// If changeCounter > 1, it might also be okay, but then we need to make sure that
							// mVertexReferences only contains each meshlet once.
							LOG_S(ERROR) << "Error: There should be exactly one change done on the meshlet vertices.";
						}
					}

				}
				else {
					nonSplitCounter++;
					//LOG_S(INFO) << "Vertex " + std::to_string(pair.first) + " is referenced by " + std::to_string(pair.second.size()) +
					//	" different meshlets, but the meshlet mins are compatible. I'll keep it as it is.";
				}
			}
		}
		LOG_S(INFO) << "Split " << splitCounter << " vertices, kept " << nonSplitCounter << " vertices.";
	}


	// Now build the temporary vertex data
	std::vector<cpu_temporary_vertex_data> mTemporaryVertexData;
	{
		mTemporaryVertexData.resize(compressedVertexData.size());
		for (int mid = 0; mid < newMeshlets.size(); mid++)
		{
			auto& currMeshlet = newMeshlets[mid];
			uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
			unpackMeshIdxVcTc(currMeshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);

			const auto minimums = minimumVertexDataForMeshlets[mid];
			const auto bitCounts = bitCountsForMeshlets[mid];

			for (int i = 0; i < vertexCount; i++) {
				const auto& vid = currMeshlet.mVertices[i];
				const auto& cmlt = compressedVertexData[vid];
				cpu_temporary_vertex_data mltDiff = {
					cmlt.position,
					cmlt.normal,
					cmlt.texCoord,
					cmlt.boneAttributes,
					bitCounts.boneAttributes / 8,
					cmlt.originalBoneAttributes
				};
				mTemporaryVertexData[vid] = mltDiff;
			}
		}
	}
	compressedVertexData.clear(); // should now not be used anymore

	// Optionally sort vertex data in the order of the first appearance in a meshlet
	if (mSortVertexDataInMeshletOrder) {
		std::vector<bool> vertexInNewList(mTemporaryVertexData.size(), false);
		std::map<uint32_t, uint32_t> oldVidToNewVid;
		std::vector<cpu_temporary_vertex_data> sortedVertexData;
		sortedVertexData.reserve(mTemporaryVertexData.size());
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
					sortedVertexData.push_back(mTemporaryVertexData[oldvid]);
				}
				currMeshlet.mVertices[i] = oldVidToNewVid[oldvid];
			}
		}
		mTemporaryVertexData = sortedVertexData;
	}

	// Get total final byte size of the vertex data
	uint32_t totalVertexBufferBytes = 0;
	for (const auto& vdata : mTemporaryVertexData) {
		// Calculate size with padding to 16 bit such that we can access the static data with 16-bit pointers
		totalVertexBufferBytes += vdata.getFinalByteSize(2);
	}

	// Fill the vertex and meshlet extension data aswell as set the vid to the byte offset
	{
		m8BitVertexData.reserve(totalVertexBufferBytes);
		std::map<uint32_t, uint32_t> oldVidToNewVid;
		for (int vid = 0; vid < mTemporaryVertexData.size(); vid++) {
			const auto& vdata = mTemporaryVertexData[vid];
			oldVidToNewVid[vid] = static_cast<uint32_t>(m8BitVertexData.size());
			// Store with padding to 16 bit such that we can access the static data with 16-bit pointers
			vdata.addToDataVector(m8BitVertexData, 2);
		}

		// Change vid in meshlets and fill extended meshlet buffer data
		mMeshletExtensionData.reserve(newMeshlets.size());
		for (int mid = 0; mid < newMeshlets.size(); mid++)
		{
			auto& currMeshlet = newMeshlets[mid];
			uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
			unpackMeshIdxVcTc(currMeshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);

			const auto firstOldVid = currMeshlet.mVertices[0];
			const auto bitCounts = bitCountsForMeshlets[mid];

			for (int i = 0; i < vertexCount; i++) {
				const auto& vid = currMeshlet.mVertices[i];
				// Safety check that the boneCounts of all vertices are the same as the meshlet (should be the case by design)
				if (mTemporaryVertexData[vid].boneAttributesResolution != bitCounts.boneAttributes / 8) {
					LOG_S(FATAL) << "Error: The boneAttributesResolution of the vertex is not the same as the meshlet.";
				}
				//assert(mTemporaryVertexData[vid].boneAttributesResolution == bitCounts.boneAttributes / 8);
				currMeshlet.mVertices[i] = oldVidToNewVid[vid];
			}

			// Save the mins and bitcounts in the meshlet array
			auto& meshletExtension = mMeshletExtensionData.emplace_back(meshlet_extension{});
			if (bitCounts.boneAttributes == 0) {
				auto originalBoneData = mTemporaryVertexData[firstOldVid].originalBoneAttributes;
				meshletExtension.tupleIndex = originalBoneData.x;
				meshletExtension.w2 = originalBoneData.y;
				meshletExtension.w3 = originalBoneData.z;
				meshletExtension.w4 = originalBoneData.w;
			}
			else {
				meshletExtension.tupleIndex = std::numeric_limits<uint16_t>::max();
				meshletExtension.w2 = 0;
				meshletExtension.w3 = 0;
				meshletExtension.w4 = 0;
			}
		}
	}

	// IMPORTANT: AT THIS POINT WE CAN ONLY ACCES THE VERTICES IN THE 8 BIT BUFFER
	mTemporaryVertexData.clear();

	// Overwrite the meshlets
	meshletBuilder->overwriteMeshlets(newMeshlets);

	m8BitVertexBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(m8BitVertexData)
	);
	avk::context().record_and_submit_with_fence({ m8BitVertexBuffer->fill(m8BitVertexData.data(), 0) }, *queue)->wait_until_signalled();
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 0, m8BitVertexBuffer));

	mBoneLUTBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mBoneLUTData)
	);
	avk::context().record_and_submit_with_fence({ mBoneLUTBuffer->fill(mBoneLUTData.data(), 0) }, *queue)->wait_until_signalled();
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 1, mBoneLUTBuffer));

	mMeshletExtensionBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mMeshletExtensionData)
	);
	avk::context().record_and_submit_with_fence({ mMeshletExtensionBuffer->fill(mMeshletExtensionData.data(), 0) }, *queue)->wait_until_signalled();
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 2, mMeshletExtensionBuffer));

	// report to props:
	mShared->mPropertyManager->get("lut_count")->setUint(mBoneLUTData.size());
	mShared->mPropertyManager->get("lut_size")->setUint(mBoneLUTData.size() * sizeof(glm::u16vec4));
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(uint8_t) * m8BitVertexData.size());
	mShared->mPropertyManager->get("emb_size")->setUint(sizeof(meshlet_extension) * mMeshletExtensionData.size());
}

void PDifferenceCodec32::doDestroy()
{
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	m8BitVertexData.clear();
	m8BitVertexBuffer = avk::buffer();
	mMeshletExtensionData.clear();
	mMeshletExtensionBuffer = avk::buffer();
}

void PDifferenceCodec32::hud_config(bool& config_has_changed)
{
	ImGui::Checkbox("LUT with Luid-Reuse", &mWithReuse);
	ImGui::Checkbox("Sort Vertex Data in Meshlet Order", &mSortVertexDataInMeshletOrder);
}
