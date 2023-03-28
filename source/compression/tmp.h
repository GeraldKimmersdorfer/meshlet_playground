#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>

void compress(std::vector<glm::uvec4>& boneIndices, std::vector<glm::vec4>& boneWeights) {
	// =================================== STEP 1 ==================================================================================================
	// We want to compress the bone indices per vertex. To do so we'll create a lookup table and just save one id for each vertex.
	// Well pass the whole lookup table to the shader. This way we'll save some space as a lot of vertices usually are connected to the same bones.
	// =============================================================================================================================================
	// 

	// ToDo: Insert checks that 16-bit for indices aswell as for the lookup-id is actually enough.

	// Step 1a: Convert indices list to 16-bit. 32 bit is unnecessary
	std::vector<glm::u16vec4> tmpScaledBoneIndices;
	for (auto il : boneIndices) {
		tmpScaledBoneIndices.push_back(il);
	}

	std::vector<glm::u16vec4> tmpReducedBoneIndices;
	// Step 1b: Set Indices which are irrelevant to max 
	for (int i = 0; i < tmpScaledBoneIndices.size(); i++) {
		auto curWeights = boneWeights[i];
		for (int j = 0; j < 4; j++) {
			if (curWeights[j] == 0.0) {
				tmpScaledBoneIndices[i][j] = 65535;	// 65535 = 2 ^ 16 - 1
			}
		}
		tmpReducedBoneIndices.push_back(tmpScaledBoneIndices[i]);
	}

	// Step 1c: Sort the list lexigraphically
	// To do this quickly i'll use an ugly hack: I interpret the vectors as one 64bit unsigned integer and sort according to those. It
	// might be quick but not really scalable and it's also black objective c magic
	std::sort(tmpReducedBoneIndices.begin(), tmpReducedBoneIndices.end(),
		[](const glm::u16vec4& a, const glm::u16vec4& b) -> bool
		{
			return (*((glm::u64*)&(a.x))) < (*((glm::u64*)&(b.x)));
		});

	// Step 1d: Delete duplicates
	tmpReducedBoneIndices.erase(std::unique(tmpReducedBoneIndices.begin(), tmpReducedBoneIndices.end()), tmpReducedBoneIndices.end());

	// TODO: Step 1e and 1f can be considerable speed up by using a hash map for the index-list

	// Step 1e: Sort out even further as for example entries u16vec4(1, 2, 0, 65535) can be replaced
	// by u16vec4(1, 2, 0, 3) as the last index is irrelevant anyway
	// This step also deletes entries with just one bone index, as we will later write that index directly in
	// the lookup index data for each vertex. (as oposed to write an index to this list which would also just be one index...)
	for (int j = 0; j < tmpReducedBoneIndices.size(); j++) {
		auto& bi1 = tmpReducedBoneIndices[j];
		// check whether bi1 has an entry tats unimportant and on which position it is:
		int uip = 3;
		for (; uip >= 0; uip--) {
			if (bi1[uip] < 65535) break;
		}
		if (uip <= 0) {
			// This entry is either all 65535 (which shouldnt exist...) or just with one value. We don't need those in the list. (reason see above)
			tmpReducedBoneIndices.erase(tmpReducedBoneIndices.begin() + j);
			j--; // necessary as we delete the current entry...
			continue;
		}
		if (uip < 3) {
			// okay, it has unimportant entries starting at position uip
			// Lets see whether we can find an entry that has up until uip the same structure.
			// if yes we may delete this entry.
			for (auto& bi2 : tmpReducedBoneIndices) {
				if (bi1 == bi2) continue;
				int i = 0;
				for (; i <= uip; i++) {
					if (bi1[i] != bi2[i]) break;
				}
				if (i == uip + 1) {
					// bi2 replaces bi1, so we can delete bi2
					tmpReducedBoneIndices.erase(tmpReducedBoneIndices.begin() + j);
					j--; // necessary as we delete the current entry...
					break;
				}
			}
		}
	}

	//DEV: Output Index-Table
	//for (auto& tmp : tmpReducedBoneIndices) std::cout << glm::to_string(tmp) << std::endl;

	// Step 1f: Now lets go through all vertices and find the suitable id that corresponds to the lookup table
	std::vector<glm::u16> mBoneIndicesLookupID;
	for (auto il : tmpScaledBoneIndices) {
		if (il.y == 65535) {
			// In this case there is just one influencing bone therefore we'll directly save the bone id inside the boneIndexLookupId
			mBoneIndicesLookupID.push_back(il.x);
		}
		else {
			for (int i = 0; i < tmpReducedBoneIndices.size(); i++) {
				bool fitFound = true;
				for (int j = 0; j < 4; j++) {
					if (il[j] == 65535) break;
					if (tmpReducedBoneIndices[i][j] != il[j]) {
						fitFound = false;
						break;
					}
				}
				if (fitFound) {
					mBoneIndicesLookupID.push_back(i);
					//std::cout << glm::to_string(tmpReducedBoneIndices[i]) << " fits " << glm::to_string(il) << std::endl;
					break;
				}
			}
		}
	}

	// Calculate how much space we saved:
	float storageBefore = tmpScaledBoneIndices.size() * sizeof(glm::u16vec4);
	float storageAfterwards = mBoneIndicesLookupID.size() * sizeof(glm::u16) + tmpReducedBoneIndices.size() * sizeof(glm::u16vec4);
	std::cout << "Saved " << (1 - (storageAfterwards / storageBefore)) * 100.0f << "% (" << storageBefore - storageAfterwards << " bytes) by creating a lookup-table for the indices." << std::endl;
}