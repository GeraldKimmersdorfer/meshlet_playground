#include "lut_helper.h"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <iostream>


const std::map<uint32_t, uint8_t> permutationKeyLookup = {
 { 66051u, 0 },
 { 66306u, 1 },
 { 131331u, 2 },
 { 131841u, 3 },
 { 197121u, 4 },
 { 196866u, 5 },
 { 16777731u, 6 },
 { 16777986u, 7 },
 { 16908291u, 8 },
 { 16909056u, 9 },
 { 16974336u, 10 },
 { 16973826u, 11 },
 { 33619971u, 12 },
 { 33620736u, 13 },
 { 33554691u, 14 },
 { 33555201u, 15 },
 { 33751041u, 16 },
 { 33751296u, 17 },
 { 50397696u, 18 },
 { 50397186u, 19 },
 { 50462976u, 20 },
 { 50462721u, 21 },
 { 50332161u, 22 },
 { 50331906u, 23 }
};

uint8_t getPermutationKey(uint32_t* arr) {
	uint32_t hash = (arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | (arr[3]);
	auto it = permutationKeyLookup.find(hash);
	assert(it != permutationKeyLookup.end());	// Please only feed the function with permutations of { 0, 1, 2, 3 }
	return it->second;
}



glm::uvec4 sortUvec4(const glm::uvec4& src, uint8_t& permutation) {
	uint32_t* vec = (uint32_t*)(void*)&src[0];
	uint32_t permutArray[] = { 0, 1, 2, 3 };
	std::sort(permutArray, permutArray + 4, [vec](const uint32_t& a, const uint32_t& b) {
		return vec[a] < vec[b];
		});
	permutation = getPermutationKey(permutArray);
	return applyPermutation(src, permutation);
}


bool cmpIndexVectorSort(const glm::uvec4& a, const glm::uvec4& b) {
	for (int i = 0; i < 4; i++) {
		if (a[i] > b[i]) return true;
		else if (a[i] < b[i]) return false;
	}
	return false;
}

bool cmpIndexVectorEqual(const glm::uvec4& a, const glm::uvec4& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// Returns true with respect to doesn't matter values (UINT32_MAX). e.g. a={0,2,3,UINT32_MAX}; b={0,2,3,4} returns true
bool cmpIndexVectorGoodEnough(const glm::uvec4& a, const glm::uvec4& b) {
	for (int i = 0; i < 4; i++) {
		if (a[i] != UINT32_MAX && b[i] != UINT32_MAX && a[i] != b[i]) return false;
	}
	return true;
}

void createBoneIndexLUT(bool withShuffling, const std::vector<vertex_data>& vertexData, std::vector<glm::uvec4>& lut, std::vector<uint16_t>* vertexLUIndexTable, std::vector<uint8_t>* vertexLUPermutation)
{
	assert(vertexLUIndexTable);	// Dont' allow no target for new VertexLUIndices

	// STEP 1: Create vector of all indices:
	std::unordered_set<glm::uvec4> allPossibleIndices;
	for (auto& vert : vertexData) {
		glm::uvec4 oldIndex = vert.mBoneIndices;
		for (glm::length_t i = 0; i < 4; i++) {
			if (vert.mBoneWeights[i] <= 0.000001f) oldIndex[i] = UINT32_MAX;
		}
		if (withShuffling) {	// when shuffling on, sort the vector elements first
			unsigned int* vec = (unsigned int*)(void*)&oldIndex[0];
			std::sort(vec, vec + 4);
		}
		allPossibleIndices.insert(oldIndex);
	}

	// STEP 2: Order the indices: (Different solution would be to use uint128 and magic casting, probably faster but uint128 no standard)
	std::copy(allPossibleIndices.begin(), allPossibleIndices.end(), std::back_inserter(lut));
	std::sort(lut.begin(), lut.end(), cmpIndexVectorSort);

	// STEP 3: Now only keep the last of each set, as this one should be the one that encompasses all of them
	for (int i = 0; i < lut.size() - 1; i++) {
		// Check the one after me, if it can be used in my stead then delete me
		if (cmpIndexVectorGoodEnough(lut[i], lut[i + 1])) {
			lut.erase(lut.begin() + i);
			i--;
		}
	}

	// STEP 4: Assign corresponding lu indices to all vertices:
	vertexLUIndexTable->resize(vertexData.size());
	if (vertexLUPermutation) vertexLUPermutation->resize(vertexData.size());

	// NOTE: I WOULD ASSUME THIS IS TERRIBLY SLOW
	for (size_t vID = 0; vID < vertexData.size(); vID++) {
		glm::uvec4 bIndices = vertexData[vID].mBoneIndices;
		for (glm::length_t i = 0; i < 4; i++) if (vertexData[vID].mBoneWeights[i] <= 0.000001f) bIndices[i] = UINT32_MAX;
		if (withShuffling) {
			uint8_t permutation;
			bIndices = sortUvec4(bIndices, permutation);
			if (vertexLUPermutation) (*vertexLUPermutation)[vID] = permutation;
		}
		//std::vector<glm::uvec4> foundPartner = {};
		for (uint16_t luindex = 0; luindex < lut.size(); luindex++) {
			if (cmpIndexVectorGoodEnough(bIndices, lut[luindex])) {
				//foundPartner.push_back(lut[luindex]);
				(*vertexLUIndexTable)[vID] = luindex;
				break;
			}
		}
		/*
		if (foundPartner.size() == 0) {
			std::cerr << "Found 0 partner for index" << std::endl;
		}
		else if (foundPartner.size() >= 1) {
			std::cerr << "Multiple: " << glm::to_string(bIndices);
			for (int i = 0; i < foundPartner.size(); i++)
				std::cerr << glm::to_string(foundPartner[i]) << "; ";
		}*/
	}
}
