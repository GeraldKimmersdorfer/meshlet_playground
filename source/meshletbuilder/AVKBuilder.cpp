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

#include "AVKBuilder.h"
#include "../SharedData.h"
#include "../helpers/packing.h"

void AVKBuilder::doGenerate()
{
	uint32_t aMaxVertices = sNumVertices;
	uint32_t aMaxIndices = sNumIndices - ((sNumIndices / 3) % 4) * 3;

	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	std::vector<meshlet_native> allMeshlets;
	for (size_t midx = 0; midx < mShared->mMeshData.size(); midx++) {

		auto meshletSelection = avk::make_models_and_mesh_indices_selection(mShared->mModel, midx);
		auto cpuMeshlets = avk::divide_into_meshlets(meshletSelection);

		for (auto& cpuMshlt : cpuMeshlets)
		{
			auto& genMeshlet = allMeshlets.emplace_back();
			genMeshlet.mMeshIdxVcTc = packMeshIdxVcTc(static_cast<uint32_t>(midx), cpuMshlt.mVertexCount, cpuMshlt.mIndexCount / 3);
			std::ranges::copy(cpuMshlt.mVertices, genMeshlet.mVertices);
			std::ranges::copy(cpuMshlt.mIndices, reinterpret_cast<uint8_t*>(genMeshlet.mIndicesPacked));
		}
	}
	mMeshletsNative = std::move(allMeshlets);
}
