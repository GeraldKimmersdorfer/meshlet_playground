
#include "MeshoptimizerBuilder.h"
#include <span>

std::vector<meshlet_data> MeshoptimizerBuilder::generate(std::vector<vertex_data>& vertexData, std::vector<uint32_t>& indexData, std::vector<mesh_data>& meshData, uint32_t aMaxVertices, uint32_t aMaxIndices)
{
	/*
	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	for (uint32_t midx = 0; midx < meshData.size(); midx++) {
		auto& mesh = meshData[midx];
		std::span<uint32_t> indices{ &indexData[mesh.mIndexOffset], mesh.mIndexCount };
		std::span<vertex_data> vertices{ &vertexData[mesh.mVertexOffset], mesh.mVertexCount };

		// get the maximum number of meshlets that could be generated
		size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), aMaxVertices, max_triangles);
		std::vector<meshopt_Meshlet> meshlets(max_meshlets);
		std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
		std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

		// let meshoptimizer build the meshlets for us
		size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
			indices.data(), indices.size(), &vertices[0].mPositionTxX[0], vertices.size(), sizeof(meshlet_data),
			aMaxVertices, max_triangles, cone_weight);

		std::vector<meshlet_data> generatedMeshlets(meshlet_count);
		for (int mltx = 0; mltx < meshlet_count; mltx++) {
			auto& m = meshlets[mltx];
			auto& gm = generatedMeshlets[mltx];
			gm.mMeshIndex = midx;
			for (int i = 0; i < m.vertex_count; i++) {
				gm.m
			}
			gm.mVertexCount = m.vertex_count;
			gm.mPrimitiveCount = m.triangle_count;
			std::ranges::copy(meshlet_vertices.begin() + m.vertex_offset,
				meshlet_vertices.begin() + m.vertex_offset + m.vertex_count,
				gm.mVertices);
			std::ranges::copy(meshlet_triangles.begin() + m.triangle_offset,
				meshlet_triangles.begin() + m.triangle_offset + gm.mIndexCount,
				gm.mIndices.begin());
		}

	}*/
	return std::vector<meshlet_data>();
}
