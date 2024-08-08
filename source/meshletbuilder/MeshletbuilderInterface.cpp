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

#include "MeshletbuilderInterface.h"
#include "../SharedData.h"
#include "../helpers/packing.h"

void MeshletbuilderInterface::generate()
{
	LOG_S(INFO) << "Generating meshlets for " << mName;
	if (mShared->mIndices.size() != mGeneratedOnIndexCount) {
		doGenerate();
		addVertexOffsetToMeshlets();
		generateRedirectedMeshletsFromNative();
		mGeneratedOnIndexCount = mShared->mIndices.size();
	}
}

void MeshletbuilderInterface::destroy()
{
	mMeshletsNative.clear();
	mMeshletsRedirect.clear();
	mRedirectPackedIndexData.clear();
	mGeneratedOnIndexCount = 0;	// make sure it gets rebuild on next generate
	LOG_S(INFO) << "Meshletbuilder " << mName << " destroyed";
}

const std::pair<std::vector<meshlet_redirect>, std::vector<uint32_t>> MeshletbuilderInterface::getMeshletsRedirect()
{
	return std::make_pair(mMeshletsRedirect, mRedirectPackedIndexData);
}

const std::vector<meshlet_native>& MeshletbuilderInterface::getMeshletsNative()
{
	return mMeshletsNative;
}

void MeshletbuilderInterface::overwriteMeshlets(const std::vector<meshlet_native>& meshletsNative)
{
	mMeshletsNative = meshletsNative;
	generateRedirectedMeshletsFromNative();
}

void MeshletbuilderInterface::reportBufferSizes()
{
	mShared->mPropertyManager->get("meshlets")->setUint(mMeshletsNative.size());
	mShared->mPropertyManager->get("mb_size")->setUint(mMeshletsNative.size() * sizeof(meshlet_native));
	mShared->mPropertyManager->get("mb_redirect_size")->setUint(mMeshletsRedirect.size() * sizeof(meshlet_redirect) + mRedirectPackedIndexData.size() * sizeof(uint32_t));
}

void MeshletbuilderInterface::generateRedirectedMeshletsFromNative()
{
	std::vector<meshlet_redirect> newMeshletData;
	std::vector<uint32_t> newIndexData;
	for (auto& nml : mMeshletsNative) {
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(nml.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
		// add all vertex-data

		auto rml = newMeshletData.emplace_back(meshlet_redirect{
			.mDataOffset = static_cast<uint32_t>(newIndexData.size()),
			.mMeshIdxVcTc = nml.mMeshIdxVcTc
			});

		newIndexData.insert(newIndexData.end(), &nml.mVertices[0], &nml.mVertices[(int)vertexCount]);
		newIndexData.insert(newIndexData.end(), &nml.mIndicesPacked[0], &nml.mIndicesPacked[(triangleCount * 3 + 3) / 4]);
	}
	mRedirectPackedIndexData = std::move(newIndexData);
	mMeshletsRedirect = std::move(newMeshletData);

	reportBufferSizes();
}

void MeshletbuilderInterface::addVertexOffsetToMeshlets()
{
	for (auto& m : mMeshletsNative) {
		uint32_t meshIndex; uint8_t vertexCount; uint8_t triangleCount;
		unpackMeshIdxVcTc(m.mMeshIdxVcTc, meshIndex, vertexCount, triangleCount);
		for (int i = 0; i < vertexCount; i++) {
			m.mVertices[i] += mShared->mExtendedMeshData[meshIndex].vertexOffset;
		}
	}
}
