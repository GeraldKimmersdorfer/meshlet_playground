#include "DynamicMeshletVertexCodec.h"
#include "../helpers/lut_helper.h"
#include <imgui.h>
#include "../helpers/packing_helper.h"
#include "PermutationCodec.h"
#include "../meshletbuilder/MeshletbuilderInterface.h"
#include <glm/gtx/string_cast.hpp>

void DynamicMeshletVertexCodec::doCompress(avk::queue* queue)
{
	// Step 1: Create LUT Table
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(true, true, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	// Step 2: Create Compressed VertexTable:
	mCompressedVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		auto& newVert = mCompressedVertexData.emplace_back(cpu_compressed_vertex_data{});
		newVert.mPosition = glm::u16vec3(
			static_cast<uint16_t>(vert.mPositionTxX[0] * 0xFFFF),
			static_cast<uint16_t>(vert.mPositionTxX[1] * 0xFFFF),
			static_cast<uint16_t>(vert.mPositionTxX[2] * 0xFFFF)
		);
		newVert.mNormal = compressNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
		newVert.mTexCoord = compressTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
		newVert.mBoneIndexLuid = vertexLUIndexTable[vid];
		newVert.mBoneWeightsAndPermutation = PermutationCodec::encode(vert.mBoneWeights, vertexLUPermutation[vid]);	// TODO: permutation is only 5 bit. Increase resolution of permutation coding
	}
	auto meshletBuilder = mShared->getCurrentMeshletBuilder();
	const auto& meshlets = meshletBuilder->getMeshletsNative();
#if _DEBUG
	// NEXT CHECK: Lets check wether vertices are accessed in different meshlets! With our encoding scheme that can't be the case
	// because the vertex data will only contain the offset to the median value of the meshlet. If thats the case, we'll have
	// to copy the vertex for both meshlets. But first lets see if thats actually necessary.
	std::map<uint32_t, uint32_t> mVertexReferences;	// containes for each vertex by how many different meshlets its referenced
	for (uint32_t mltid = 0; mltid < meshlets.size(); mltid++) {
		auto& meshlet = meshlets[mltid];
		std::map<uint32_t, uint32_t> mVertexReferencesForMeshlet;
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(meshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
		auto& mesh = mShared->mMeshData[meshIndex];
		for (uint32_t mltvid = 0; mltvid < vertexCount; mltvid++) {
			auto gvid = meshlet.mVertices[mltvid] + mesh.mVertexOffset;
			auto it = mVertexReferencesForMeshlet.find(gvid);
			if (it == mVertexReferencesForMeshlet.end()) {
				mVertexReferencesForMeshlet[gvid] = 1;
			}
			else {
				it->second++;
			}
		}

		// Check wether same vertex is in meshlet twice (shouldnt happen) and copy into big meshletReferenceMap
		for (auto& pair : mVertexReferencesForMeshlet) {
			if (pair.second > 1) throw std::runtime_error("Vertex " + std::to_string(pair.first) + " is referenced " + std::to_string(pair.second) + 
				" times inside the same meshlet. That shouldnt happen!");
			auto gvid = pair.first;
			auto it = mVertexReferences.find(gvid);
			if (it == mVertexReferences.end()) {
				mVertexReferences[gvid] = pair.second;
			}
			else {
				it->second += pair.second;
			}
		}
	}
	// NOTE: This triggers, which means there are vertices referenced by the same meshlet. That means along the way we need to change the meshlet building
	// function in a way that those get copied! BUT since right now i just want to evaluate the potential its fine...
	for (auto& pair : mVertexReferences) {
		if (pair.second > 1) {
			std::cerr << "Vertex " + std::to_string(pair.first) + " is referenced by " + std::to_string(pair.second) +
				" different meshlets. It has to be split up which is not implemented." << std::endl;
		}
	}
#endif
	glm::u64vec4 bitSavedTotal{ 0l };
	long totalVertexCount{ 0 };
	uint32_t uncompressedBitCount{ sizeof(cpu_compressed_vertex_data) * 8 };

	for (uint32_t mltid = 0; mltid < meshlets.size(); mltid++) {
		auto& meshlet = meshlets[mltid];
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(meshlet.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
		auto& mesh = mShared->mMeshData[meshIndex];

		// GATHER COMPRESSED VERTEX DATA FOR THIS MESHLET
		std::vector<cpu_compressed_vertex_data> vertexDataForMeshlet;
		vertexDataForMeshlet.reserve(vertexCount);
		for (uint32_t mltvid = 0; mltvid < vertexCount; mltvid++) {
			auto gvid = meshlet.mVertices[mltvid] + mesh.mVertexOffset;
			vertexDataForMeshlet.push_back(mCompressedVertexData[gvid]);
		}
		
		// GET MEDIAN VALUES OF EACH COMPONENT. THAT MEANS WE HAVE TO SORT THE vertexDataForMeshlet A FEW TIMES...
		// NOTE: Theres probably a better way. Also looking at the average might be interesting. I just 
		/*
		cpu_compressed_vertex_data medians;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mPosition.x < b.mPosition.x; });
		medians.mPosition.x = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mPosition.x;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mPosition.y < b.mPosition.y; });
		medians.mPosition.y = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mPosition.y;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mPosition.z < b.mPosition.z; });
		medians.mPosition.z = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mPosition.z;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mNormal.x < b.mNormal.x; });
		medians.mNormal.x = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mNormal.x;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mNormal.y < b.mNormal.y; });
		medians.mNormal.y = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mNormal.y;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mTexCoord.x < b.mTexCoord.x; });
		medians.mTexCoord.x = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mTexCoord.x;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mTexCoord.y < b.mTexCoord.y; });
		medians.mTexCoord.y = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mTexCoord.y;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mBoneIndexLuid < b.mBoneIndexLuid; });
		medians.mBoneIndexLuid = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mBoneIndexLuid;
		std::sort(vertexDataForMeshlet.begin(), vertexDataForMeshlet.end(), [](const cpu_compressed_vertex_data& a, const cpu_compressed_vertex_data& b) { return a.mBoneWeightsAndPermutation < b.mBoneWeightsAndPermutation; });
		medians.mBoneWeightsAndPermutation = vertexDataForMeshlet[vertexDataForMeshlet.size() / 2].mBoneWeightsAndPermutation;*/

		// DITCH THE MEDIAN OR AVERAGE: I WILL TRY TO SAVE THE MINIMUM AS I ONLY HAVE TO SAVE POSITIVE VALUES FOR THE DIFFERENCES THEN!
		cpu_compressed_vertex_data minimums{
			{UINT16_MAX, UINT16_MAX, UINT16_MAX}, {UINT16_MAX, UINT16_MAX}, {UINT16_MAX, UINT16_MAX}, UINT16_MAX, UINT32_MAX
		};
		for (const auto& cmlt : vertexDataForMeshlet) {
			minimums.mPosition = glm::min(minimums.mPosition, cmlt.mPosition);
			minimums.mNormal = glm::min(minimums.mNormal, cmlt.mNormal);
			minimums.mTexCoord = glm::min(minimums.mTexCoord, cmlt.mTexCoord);
			minimums.mBoneIndexLuid = glm::min(minimums.mBoneIndexLuid, cmlt.mBoneIndexLuid);
			minimums.mBoneWeightsAndPermutation = glm::min(minimums.mBoneWeightsAndPermutation, cmlt.mBoneWeightsAndPermutation);
		}

		cpu_compressed_vertex_data maxDifferences{
			{0,0,0}, {0,0}, {0,0}, 0, 0
		};
		for (const auto& cmlt : vertexDataForMeshlet) {
			cpu_compressed_vertex_data mltDiff = {
				cmlt.mPosition - minimums.mPosition,
				cmlt.mNormal - minimums.mNormal,
				cmlt.mTexCoord - minimums.mTexCoord,
				cmlt.mBoneIndexLuid - minimums.mBoneIndexLuid,
				cmlt.mBoneWeightsAndPermutation - minimums.mBoneWeightsAndPermutation
			};
			maxDifferences.mPosition = glm::max(maxDifferences.mPosition, mltDiff.mPosition);
			maxDifferences.mNormal = glm::max(maxDifferences.mNormal, mltDiff.mNormal);
			maxDifferences.mTexCoord = glm::max(maxDifferences.mTexCoord, mltDiff.mTexCoord);
			maxDifferences.mBoneIndexLuid = glm::max(maxDifferences.mBoneIndexLuid, mltDiff.mBoneIndexLuid);
			maxDifferences.mBoneWeightsAndPermutation = glm::max(maxDifferences.mBoneWeightsAndPermutation, mltDiff.mBoneWeightsAndPermutation);
		}

		cpu_compressed_vertex_data bitCounts{
			glm::u16vec3(glm::ceil(glm::log2(glm::vec3(maxDifferences.mPosition) + glm::vec3(1.0)))),
			glm::u16vec2(glm::ceil(glm::log2(glm::vec2(maxDifferences.mNormal) + glm::vec2(1.0)))),
			glm::u16vec2(glm::ceil(glm::ceil(glm::log2(glm::vec2(maxDifferences.mTexCoord) + glm::vec2(1.0))))),
			glm::u16vec1(glm::ceil(glm::log2(glm::vec1(maxDifferences.mBoneIndexLuid + 1)))).x,
			glm::u32vec1(glm::ceil(glm::log2(glm::vec1(maxDifferences.mBoneWeightsAndPermutation + 1)))).x
		};
		glm::u16vec4 sumBitCount(bitCounts.mPosition.x + bitCounts.mPosition.y + bitCounts.mPosition.z + bitCounts.mNormal.x + bitCounts.mNormal.y + bitCounts.mTexCoord.x + bitCounts.mTexCoord.y + bitCounts.mBoneIndexLuid + bitCounts.mBoneWeightsAndPermutation);
		sumBitCount = glm::ceil(glm::vec4(sumBitCount) / glm::vec4(1.0f, 8.0f, 16.0f, 32.0f)) * glm::vec4(1.0f, 8.0f, 16.0f, 32.0f);
		glm::u16vec4 bitSaved = glm::u16vec4(uncompressedBitCount) - sumBitCount;

		/*
		std::cout << "==== MAX DIFFERENCES FOR MESHLET " << std::to_string(mltid) << " ====" << std::endl;
		std::cout << "Position:                  " << glm::to_string(maxDifferences.mPosition) << " " << glm::to_string(bitCounts.mPosition) << std::endl;
		std::cout << "Normal:                    " << glm::to_string(maxDifferences.mNormal) << " " << glm::to_string(bitCounts.mNormal) << std::endl;
		std::cout << "TexCoord:                  " << glm::to_string(maxDifferences.mTexCoord) << " " << glm::to_string(bitCounts.mTexCoord) << std::endl;
		std::cout << "BoneIndexLuid:             " << std::to_string(maxDifferences.mBoneIndexLuid) << " " << std::to_string(bitCounts.mBoneIndexLuid) << std::endl;
		std::cout << "BoneWeightsAndPermutation: " << std::to_string(maxDifferences.mBoneWeightsAndPermutation) << " " << std::to_string(bitCounts.mBoneWeightsAndPermutation) << std::endl;
		std::cout << "------------------------------------------------------" << std::endl;
		std::cout << "BitCounts:(u1,u8,u16,u32)  " << glm::to_string(sumBitCount) << std::endl;
		std::cout << "BitSaved: (u1,u8,u16,u32)  " << glm::to_string(bitSaved) << std::endl;
		std::cout << "======================================================" << std::endl;*/

		bitSavedTotal += glm::u16vec4(vertexCount) * bitSaved;
		totalVertexCount += vertexCount;

		// Lets say we need 32 bit per meshlet to save the vertex data structure:
		bitSavedTotal -= glm::u16vec4(32);

		// Additionally we need to save the reference data (minimum) for the meshlet:
		bitSavedTotal -= glm::u16vec4(uncompressedBitCount);
	}


	std::cout << "BitSavedTotal: (u1,u8,u16,u32) " << glm::to_string(bitSavedTotal) << std::endl;


	long bitsUncompressed = uncompressedBitCount * totalVertexCount ;
	glm::u64vec4 bitsTotalUncompressed{ static_cast<uint64_t>(bitsUncompressed) };
	glm::u64vec4 bitsTotalCompressed = bitsTotalUncompressed - bitSavedTotal;
	glm::vec4 compression = glm::vec4(bitsTotalCompressed) / glm::vec4(bitsTotalUncompressed);
	std::cout << "BitCompressionTotal:           " << glm::to_string(compression) << std::endl;


	/*
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
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 1, mBoneLUTBuffer));*/
}

void DynamicMeshletVertexCodec::doDestroy()
{
	mCompressedVertexData.clear();
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void DynamicMeshletVertexCodec::hud_config(bool& config_has_changed)
{
	//ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
}
