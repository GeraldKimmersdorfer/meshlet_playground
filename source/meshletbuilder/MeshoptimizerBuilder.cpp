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

#include "MeshoptimizerBuilder.h"
#include <span>
#include "../../meshoptimizer/src/meshoptimizer.h"
#include "../helpers/packing.h"

#include "../SharedData.h"

void MeshoptimizerBuilder::doGenerate()
{
	uint32_t aMaxVertices = sNumVertices;
	uint32_t aMaxIndices = sNumIndices - ((sNumIndices / 3) % 4) * 3;

	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	std::vector<meshlet_native> allMeshlets;
	for (uint32_t midx = 0; midx < mShared->mMeshData.size(); midx++) {
		auto& emesh = mShared->mExtendedMeshData[midx];
		std::span<uint32_t> indices{ &(mShared->mIndices[emesh.indexOffset]), emesh.indexCount };
		std::span<vertex_data> vertices{ &(mShared->mVertexData[emesh.vertexOffset]), emesh.vertexCount };

		// get the maximum number of meshlets that could be generated
		size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), aMaxVertices, max_triangles);
		std::vector<meshopt_Meshlet> meshlets(max_meshlets);
		std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
		std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

		// let meshoptimizer build the meshlets for us
		size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
			indices.data(), indices.size(), &vertices[0].mPositionTxX[0], vertices.size(), sizeof(vertex_data),
			aMaxVertices, max_triangles, cone_weight);

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
}
