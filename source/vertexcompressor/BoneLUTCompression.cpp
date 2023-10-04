#include "BoneLUTCompression.h"

#include <glm/gtx/hash.hpp>
#include <set>

void BoneLUTCompression::doCompress(avk::queue* queue)
{
	createLUT();
	/*
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device,
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mShared->mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mShared->mVertexData.data(), 0) }, *queue)->wait_until_signalled();
	mAdditionalDescriptorBindings.push_back(avk::descriptor_binding(3, 0, mShared->mVertexBuffer));*/
}

void BoneLUTCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void BoneLUTCompression::createLUT()
{
	// STEP 1: Create vector of all indices:
	std::unordered_set<glm::uvec4> allPossibleIndices;
	for (auto& vert : mShared->mVertexData) {
		glm::uvec4 oldIndex = vert.mBoneIndices;
		glm::uvec4 newIndex;
		for (glm::length_t i = 0; i < 4; i++) {
			if (vert.mBoneWeights[i] <= 0.0001f) oldIndex[i] = UINT32_MAX;
			newIndex[3 - i] = oldIndex[i];
		}
		allPossibleIndices.insert(newIndex);
	}
	
	// STEP 2: Order the indices: (Different solution would be to use uint128 and magic casting, probably faster)
	auto cmp = [](const glm::uvec4& a, const glm::uvec4& b) {
		for (int i = 3; i >= 0; i--) {
			if (a[i] > b[i]) return true;
			else if (a[i] < b[i]) return false;
		}
		return true;
	};
	std::set<glm::uvec4, decltype(cmp)> allPossibleIndicesSorted;
	allPossibleIndicesSorted.insert(allPossibleIndices.begin(), allPossibleIndices.end());
	
	// STEP 3: Now only keep the last of each set, as this one should be the one that encompasses all of them



	// STEP 2: Delete all duplicates (where 
	std::cout << "Hallo";
}
