/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "util.h"

#include <algorithm>
#include <random>
#include <unordered_set>
#include <windows.h>

struct Vec4Hash {
	std::size_t operator()(const glm::vec4& vec) const {
		std::size_t h1 = std::hash<float>{}(vec.x);
		std::size_t h2 = std::hash<float>{}(vec.y);
		std::size_t h3 = std::hash<float>{}(vec.z);
		std::size_t h4 = std::hash<float>{}(vec.w);
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
	}
};

struct Vec4Equal {
	bool operator()(const glm::vec4& lhs, const glm::vec4& rhs) const {
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
	}
};

std::vector<glm::vec4> generateRandomWeights(int n, WeightOrder order, bool includeExtremeCases) {
	std::vector<glm::vec4> weights;
	std::unordered_set<glm::vec4, Vec4Hash, Vec4Equal> uniqueWeights;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	// Generate random weights
	for (int i = 0; i < n; ++i) {
		std::vector<float> components(4);
		float sum = 0.0f;
		for (int j = 0; j < 4; ++j) {
			components[j] = dis(gen);
			sum += components[j];
		}
		for (int j = 0; j < 4; ++j) {
			components[j] /= sum;
		}

		if (order == WEIGHT_ASC_ORDER) {
			std::sort(components.begin(), components.end());
		}
		else if (order == WEIGHT_DESC_ORDER) {
			std::sort(components.begin(), components.end(), std::greater<float>());
		}

		glm::vec4 vec(components[0], components[1], components[2], components[3]);
		if (uniqueWeights.insert(vec).second) {
			weights.push_back(vec);
		}
	}

	// Add extreme cases if the flag is set
	if (includeExtremeCases) {
		std::vector<glm::vec4> extremeCases = {
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.5f, 0.5f, 0.0f, 0.0f},
			{0.5f, 0.0f, 0.5f, 0.0f},
			{0.5f, 0.0f, 0.0f, 0.5f},
			{0.0f, 0.5f, 0.5f, 0.0f},
			{0.0f, 0.5f, 0.0f, 0.5f},
			{0.0f, 0.0f, 0.5f, 0.5f},
			{1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 0.0f},
			{1.0f / 3.0f, 1.0f / 3.0f, 0.0f, 1.0f / 3.0f},
			{1.0f / 3.0f, 0.0f, 1.0f / 3.0f, 1.0f / 3.0f},
			{0.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f},
			{0.25f, 0.25f, 0.25f, 0.25f}
		};

		for (auto& vec : extremeCases) {
			std::vector<float> components = { vec.x, vec.y, vec.z, vec.w };
			if (order == WEIGHT_ASC_ORDER) {
				std::sort(components.begin(), components.end());
			}
			else if (order == WEIGHT_DESC_ORDER) {
				std::sort(components.begin(), components.end(), std::greater<float>());
			}
			glm::vec4 sortedVec(components[0], components[1], components[2], components[3]);
			if (uniqueWeights.insert(sortedVec).second) {
				weights.push_back(sortedVec);
			}
		}
	}

	return weights;
}

void setClipboardText(const std::string& text)
{
	// Open the clipboard
	if (!OpenClipboard(nullptr)) {
		return;
	}

	// Empty the clipboard
	EmptyClipboard();

	// Allocate global memory for the text
	HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, text.size() + 1);
	if (!hGlob) {
		CloseClipboard();
		return;
	}

	// Copy the string into the allocated memory
	memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
	GlobalUnlock(hGlob);

	// Set the clipboard data
	SetClipboardData(CF_TEXT, hGlob);

	// Close the clipboard
	CloseClipboard();
}
