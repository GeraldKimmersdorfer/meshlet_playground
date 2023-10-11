
#include "BoneLUTDependentBuilder.h"
#include <span>
#include "../../meshoptimizer/src/meshoptimizer.h"
#include "../helpers/packing_helper.h"
#include "../vertexcompressor/lut_helper.h"
#include "../SharedData.h"

void BoneLUTDependentBuilder::doGenerate()
{
	uint32_t aMaxVertices = sNumVertices;
	uint32_t aMaxIndices = sNumIndices - ((sNumIndices / 3) % 4) * 3;

	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	std::vector<meshlet_native> allMeshlets;
	auto vertexPriorityCalculation = [this](uint32_t vIDSource, uint32_t vIDNew) {
		return 0.5;
		};
	
	// Calculate LUT indices:
	std::vector<glm::u16vec4> lut; std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(true, true, mShared->mVertexData, lut, &vertexLUIndexTable, &vertexLUPermutation);
	std::vector<uint32_t> vertexLUIndexTable32;
	vertexLUIndexTable32.reserve(vertexLUIndexTable.size());
	for (const auto& value : vertexLUIndexTable) vertexLUIndexTable32.push_back(static_cast<uint32_t>(value));

	for (uint32_t midx = 0; midx < mShared->mMeshData.size(); midx++) {
		auto& mesh = mShared->mMeshData[midx];
		std::span<uint32_t> indices{ &(mShared->mIndices[mesh.mIndexOffset]), mesh.mIndexCount };
		std::span<vertex_data> vertices{ &(mShared->mVertexData[mesh.mVertexOffset]), mesh.mVertexCount };

		// get the maximum number of meshlets that could be generated
		size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), aMaxVertices, max_triangles);
		std::vector<meshopt_Meshlet> meshlets(max_meshlets);
		std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
		std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

		// let meshoptimizer build the meshlets for us
		size_t meshlet_count = meshopt_buildMeshlets_feedback(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
			indices.data(), indices.size(), &vertices[0].mPositionTxX[0], vertices.size(), sizeof(vertex_data),
			aMaxVertices, max_triangles, cone_weight, &vertexLUIndexTable32[0], 1);

		std::vector<meshlet_native> generatedMeshlets(meshlet_count);
		for (int mltx = 0; mltx < meshlet_count; mltx++) {
			auto& m = meshlets[mltx];
			auto& gm = generatedMeshlets[mltx];
			gm.mMeshIdxVcTc = packMeshIdxVcTc(midx, m.vertex_count, m.triangle_count);
			std::ranges::copy(meshlet_vertices.begin() + m.vertex_offset,
				meshlet_vertices.begin() + m.vertex_offset + m.vertex_count,
				gm.mVertices);
			memcpy(gm.mIndicesPacked, &meshlet_triangles[m.triangle_offset], m.triangle_count * 3);
		}
		allMeshlets.insert(allMeshlets.end(), generatedMeshlets.begin(), generatedMeshlets.end());
	}
	mMeshletsNative = std::move(allMeshlets);
	generateRedirectedMeshletsFromNative();
}
