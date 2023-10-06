#include "MeshletbuilderInterface.h"
#include "../SharedData.h"
#include "../packing_helper.h"

void MeshletbuilderInterface::generate()
{
	if (mShared->mIndices.size() != mGeneratedOnIndexCount) {
		doGenerate();
		mGeneratedOnIndexCount = mShared->mIndices.size();
	}
}

void MeshletbuilderInterface::destroy()
{
	mMeshletsNative.clear();
	mMeshletsRedirect.clear();
	mRedirectPackedIndexData.clear();
	mGeneratedOnIndexCount = 0;	// make sure it gets rebuild on next generate
}

const std::pair<std::vector<meshlet_redirect>, std::vector<uint32_t>> MeshletbuilderInterface::getMeshletsRedirect()
{
	return std::make_pair(mMeshletsRedirect, mRedirectPackedIndexData);
}

const std::vector<meshlet_native>& MeshletbuilderInterface::getMeshletsNative()
{
	return mMeshletsNative;
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
}
