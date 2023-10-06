#pragma once

#include <vector>
#include "../shared_structs.h"

void createBoneIndexLUT(bool withShuffling, const std::vector<vertex_data>& vertexData,
	std::vector<glm::uvec4>& lut, std::vector<uint16_t>* vertexLUIndexTable = nullptr, std::vector<uint8_t>* vertexLUPermutation = nullptr);

template <class T>
void swap_ip(T& vec, int i1, int i2) {
	auto tmp = vec[i1];
	vec[i1] = vec[i2];
	vec[i2] = tmp;
}

template <class T>
T applyPermutation(const T& src, uint8_t permutationKey) {
	T o = src;
	switch (permutationKey) {
	case 0: break;
	case 1: swap_ip(o, 2, 3); break;
	case 2: swap_ip(o, 1, 2); break;
	case 3: swap_ip(o, 1, 2); swap_ip(o, 2, 3); break;
	case 4: swap_ip(o, 1, 3); break;
	case 5: swap_ip(o, 1, 3); swap_ip(o, 2, 3); break;
	case 6: swap_ip(o, 0, 1); break;
	case 7: swap_ip(o, 0, 1); swap_ip(o, 2, 3); break;
	case 8: swap_ip(o, 0, 1); swap_ip(o, 1, 2); break;
	case 9: swap_ip(o, 0, 1); swap_ip(o, 1, 2); swap_ip(o, 2, 3); break;
	case 10: swap_ip(o, 0, 1); swap_ip(o, 1, 3); break;
	case 11: swap_ip(o, 0, 1); swap_ip(o, 1, 3); swap_ip(o, 2, 3); break;
	case 12: swap_ip(o, 0, 2); break;
	case 13: swap_ip(o, 0, 2); swap_ip(o, 2, 3); break;
	case 14: swap_ip(o, 0, 1); swap_ip(o, 0, 2); break;
	case 15: swap_ip(o, 0, 1); swap_ip(o, 0, 2); swap_ip(o, 2, 3); break;
	case 16: swap_ip(o, 0, 2); swap_ip(o, 1, 3); break;
	case 17: swap_ip(o, 0, 3); swap_ip(o, 0, 1); swap_ip(o, 0, 2); break;
	case 18: swap_ip(o, 0, 3); break;
	case 19: swap_ip(o, 2, 3); swap_ip(o, 0, 2); break;
	case 20: swap_ip(o, 0, 3); swap_ip(o, 1, 2); break;
	case 21: swap_ip(o, 0, 1); swap_ip(o, 0, 3); swap_ip(o, 1, 2); break;
	case 22: swap_ip(o, 0, 1); swap_ip(o, 0, 3); break;
	case 23: swap_ip(o, 0, 1); swap_ip(o, 0, 3); swap_ip(o, 2, 3); break;
	}
	return o;
}