#pragma once

#include <vector>
#include <iostream>
#include "../shared_structs.h"

void test();

void createBoneIndexLUT(bool withShuffling, bool withMerging, const std::vector<vertex_data>& vertexData,
	std::vector<glm::u16vec4>& lut, std::vector<uint16_t>* vertexLUIndexTable = nullptr, std::vector<uint8_t>* vertexLUPermutation = nullptr);

template <class T>
static inline T applyPermutation(const T& src, uint8_t permutationKey) {
	assert(permutationKey >= 0 && permutationKey <= 23);
	switch (permutationKey) {
	case 0: return T(src[0], src[1], src[2], src[3]);
	case 1: return T(src[0], src[1], src[3], src[2]);
	case 2: return T(src[0], src[2], src[1], src[3]);
	case 3: return T(src[0], src[2], src[3], src[1]);
	case 4: return T(src[0], src[3], src[2], src[1]);
	case 5: return T(src[0], src[3], src[1], src[2]);
	case 6: return T(src[1], src[0], src[2], src[3]);
	case 7: return T(src[1], src[0], src[3], src[2]);
	case 8: return T(src[1], src[2], src[0], src[3]);
	case 9: return T(src[1], src[2], src[3], src[0]);
	case 10: return T(src[1], src[3], src[2], src[0]);
	case 11: return T(src[1], src[3], src[0], src[2]);
	case 12: return T(src[2], src[1], src[0], src[3]);
	case 13: return T(src[2], src[1], src[3], src[0]);
	case 14: return T(src[2], src[0], src[1], src[3]);
	case 15: return T(src[2], src[0], src[3], src[1]);
	case 16: return T(src[2], src[3], src[0], src[1]);
	case 17: return T(src[2], src[3], src[1], src[0]);
	case 18: return T(src[3], src[1], src[2], src[0]);
	case 19: return T(src[3], src[1], src[0], src[2]);
	case 20: return T(src[3], src[2], src[1], src[0]);
	case 21: return T(src[3], src[2], src[0], src[1]);
	case 22: return T(src[3], src[0], src[2], src[1]);
	case 23: return T(src[3], src[0], src[1], src[2]);
	}
	std::cerr << "Permutation Key has to be inside bounds [0,23]" << std::endl;
	return T(src);
}

template <class T>
static inline T applyPermutationInverse(const T& src, uint8_t permutationKey) {
	switch (permutationKey) {
	case 0: return T(src[0], src[1], src[2], src[3]);
	case 1: return T(src[0], src[1], src[3], src[2]);
	case 2: return T(src[0], src[2], src[1], src[3]);
	case 3: return T(src[0], src[3], src[1], src[2]);
	case 4: return T(src[0], src[3], src[2], src[1]);
	case 5: return T(src[0], src[2], src[3], src[1]);
	case 6: return T(src[1], src[0], src[2], src[3]);
	case 7: return T(src[1], src[0], src[3], src[2]);
	case 8: return T(src[2], src[0], src[1], src[3]);
	case 9: return T(src[3], src[0], src[1], src[2]);
	case 10: return T(src[3], src[0], src[2], src[1]);
	case 11: return T(src[2], src[0], src[3], src[1]);
	case 12: return T(src[2], src[1], src[0], src[3]);
	case 13: return T(src[3], src[1], src[0], src[2]);
	case 14: return T(src[1], src[2], src[0], src[3]);
	case 15: return T(src[1], src[3], src[0], src[2]);
	case 16: return T(src[2], src[3], src[0], src[1]);
	case 17: return T(src[3], src[2], src[0], src[1]);
	case 18: return T(src[3], src[1], src[2], src[0]);
	case 19: return T(src[2], src[1], src[3], src[0]);
	case 20: return T(src[3], src[2], src[1], src[0]);
	case 21: return T(src[2], src[3], src[1], src[0]);
	case 22: return T(src[1], src[3], src[2], src[0]);
	case 23: return T(src[1], src[2], src[3], src[0]);
	}
	std::cerr << "Permutation Key has to be inside bounds [0,23]" << std::endl;
	return T(src);
}