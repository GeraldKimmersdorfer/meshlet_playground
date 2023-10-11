#include "lut_helper.h"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <set>
#include <iostream>

#include "../helpers/csv_helpers.h"

// Define a custom hash function for glm::uvec4
struct UVec4Hash {
	std::size_t operator()(const glm::u16vec4& v) const {
		// You can use glm::hash or your own custom hash logic here
		return std::hash<unsigned short>()(v.x) ^ std::hash<unsigned short>()(v.y) ^ std::hash<unsigned short>()(v.z) ^ std::hash<unsigned short>()(v.w);
	}
};

// Define a custom equality function for glm::uvec4
struct UVec4Equal {
	bool operator()(const glm::uvec4& a, const glm::uvec4& b) const {
		return a == b;
	}
};

using LuidChangeMap = std::map<uint16_t, uint16_t>;
using IndexLookupHashMap = std::unordered_map<glm::u16vec4, uint16_t, UVec4Hash, UVec4Equal>;

const std::map<uint32_t, uint8_t> permutationKeyLookup = {
 { 66051u, 0 },		// 0, 1, 2, 3
 { 66306u, 1 },		// 0, 1, 3, 2
 { 131331u, 2 },	// 0, 2, 1, 3
 { 131841u, 3 },	// 0, 2, 3, 1
 { 197121u, 4 },	// 0, 3, 2, 1
 { 196866u, 5 },	// 0, 3, 1, 2
 { 16777731u, 6 },	// 1, 0, 2, 3
 { 16777986u, 7 },	// 1, 0, 3, 2
 { 16908291u, 8 },	// 1, 2, 0, 3
 { 16909056u, 9 },	// 1, 2, 3, 0
 { 16974336u, 10 }, // 1, 3, 2, 0
 { 16973826u, 11 },	// 1, 3, 0, 2
 { 33619971u, 12 }, // 2, 1, 0, 3
 { 33620736u, 13 },	// 2, 1, 3, 0
 { 33554691u, 14 },	// 2, 0, 1, 3
 { 33555201u, 15 },	// 2, 0, 3, 1
 { 33751041u, 16 }, // 2, 3, 0, 1
 { 33751296u, 17 }, // 2, 3, 1, 0
 { 50397696u, 18 }, // 3, 1, 2, 0
 { 50397186u, 19 }, // 3, 1, 0, 2
 { 50462976u, 20 }, // 3, 2, 1, 0
 { 50462721u, 21 }, // 3, 2, 0, 1
 { 50332161u, 22 }, // 3, 0, 2, 1
 { 50331906u, 23 }  // 3, 0, 1, 2
};

const std::map<uint8_t, uint8_t> inverseKeyLookup = {
	{0, 0},
	{1, 1},
	{2, 2},
	{3, 5},
	{4, 4},
	{5, 3},
	{6, 6},
	{7, 7},
	{8, 14},
	{9, 23},
	{10, 22},
	{11, 15},
	{12, 12},
	{13, 19},
	{14, 8},
	{15, 11},
	{16, 16},
	{17, 21},
	{18, 18},
	{19, 13},
	{20, 20},
	{21, 17},
	{22, 10},
	{23, 9}
};

uint8_t getPermutationKey(uint32_t* arr) {
	uint32_t hash = (arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | (arr[3]);
	auto it = permutationKeyLookup.find(hash);
	assert(it != permutationKeyLookup.end());	// Please only feed the function with permutations of { 0, 1, 2, 3 }
	return it->second;
}

// returns the resulting permutation key if p1 and then p2 is executed
uint8_t combinePermutations(uint8_t p1, uint8_t p2) {
	if (p1 == 0) return p2;
	if (p2 == 0) return p1;
	glm::uvec4 base = { 0,1,2,3 };
	base = applyPermutation(base, p1);
	base = applyPermutation(base, p2);
	return getPermutationKey((uint32_t*)(void*)&base[0]);
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

LuidChangeMap mergeChangeMaps(const LuidChangeMap& m1, const LuidChangeMap& m2) {
	LuidChangeMap result;
	std::set<uint16_t> markForIgnoreInM2;
	for (const auto& pair : m1) {
		auto it = m2.find(pair.second);
		if (it != m2.end()) {
			// The changed value, should be changed again according to M2, mark that the entry in M2 was already handled
			result[pair.first] = it->second;
			markForIgnoreInM2.insert(it->first);
		}
		else {
			result.insert(pair);
		}
	}
	for (const auto& pair : m2) {
		// Now add all elements in m2 that were not changed in m1
		if (!markForIgnoreInM2.contains(pair.first))
			result.insert(pair);
	}
	return std::move(result);
}

void applyChangeMap(std::vector<uint16_t>& luids, const LuidChangeMap& m) {
	for (uint32_t vid = 0; vid < luids.size(); vid++) {
		auto it = m.find(luids[vid]);
		if (it != m.end()) {
			luids[vid] = it->second;
		}
	}
}

// Only debug function
void getInverseKeys()
{
	glm::uvec4 base = { 0, 1, 2, 3 };
	for (int permut = 0; permut < 24; permut++) {
		auto reference = applyPermutation(base, permut);
		for (int invPermut = 0; invPermut < 24; invPermut++) {
			if (applyPermutation(reference, invPermut) == base) {
				std::cout << "{" << permut << ", " << invPermut << "}," << std::endl;
				break;
			}
		}
	}
}

static bool inline cmpIndexVectorSort(const glm::uvec4& a, const glm::uvec4& b) {
	// Different solution would be to use uint64 and magic casting, probably faster but not easily extendable
	for (int i = 0; i < 4; i++) {
		if (a[i] > b[i]) return true;
		else if (a[i] < b[i]) return false;
	}
	return false;
}

static bool inline cmpIndexVectorTupleForSort(const std::pair<glm::u16vec4, uint16_t>& p1, const std::pair<glm::u16vec4, uint16_t>& p2) {
	return cmpIndexVectorSort(p1.first, p2.first);
}

bool cmpIndexVectorEqual(const glm::uvec4& a, const glm::uvec4& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// Returns true with respect to doesn't matter values (UINT16_MAX). e.g. a={0,2,3,UINT16_MAX}; b={0,2,3,4} returns true
static inline bool cmpIndexVectorGoodEnough(const glm::uvec4& a, const glm::uvec4& b) {
	for (int i = 0; i < 4; i++) {
		if (a[i] != UINT16_MAX && b[i] != UINT16_MAX && a[i] != b[i]) return false;
	}
	return true;
}

static inline void cmp(glm::u16vec4& a, uint32_t lhs, uint32_t rhs) {
	int swap = (a[lhs] > a[rhs]);
	uint16_t a_lhs = a[lhs];
	a[lhs] = swap ? a[rhs] : a_lhs;
	a[rhs] = swap ? a_lhs : a[rhs];
}

// Sorts given Vector4 type in a descending fashion
static inline void sortVec4Inplace(glm::u16vec4& vec) {
	cmp(vec, 0, 1);  cmp(vec, 2, 3);  cmp(vec, 0, 2);  cmp(vec, 1, 3);  cmp(vec, 1, 2);
}

// NOTE: CHECK FOR UINT16_MAX probably UNNECESSARY SINCE WE ONLY COMPARE WITH EQUAL COUNT ANYWAY
static inline float getU16Vec4Avg(const glm::u16vec4& vec) {
	float tmp = 0.0F; float cnt = 0.0F;
	if (vec.x != UINT16_MAX) { tmp += vec.x; cnt += 1.0F; }
	if (vec.y != UINT16_MAX) { tmp += vec.y; cnt += 1.0F; }
	if (vec.z != UINT16_MAX) { tmp += vec.z; cnt += 1.0F; }
	if (vec.w != UINT16_MAX) { tmp += vec.w; cnt += 1.0F; }
	return (cnt > 0.0F) ? (tmp / cnt) : 0.0F;
}

static inline float getU16Median(const glm::u16vec4& vec) {
	uint16_t values[4]; int cnt = 0;
	for (int i = 0; i < 4; i++) if (vec[i] != UINT16_MAX) values[cnt++] = vec[i];
	return (cnt > 0) ? (float)values[cnt / 2] : 0.0F;
}

static inline int getSharedIDCount(const glm::u16vec4& a, const glm::u16vec4& b) {
	int tmp = 0;
	for (int x = 0; x < 4; x++) {
		for (int y = x; y < 4; y++) {
			if (a[x] != UINT16_MAX && b[y] != UINT16_MAX && a[x] == a[y]) tmp++;
		}
	}
	return tmp;
}

static inline void removeFromUintVector(std::vector<uint16_t>& v, uint16_t val) {
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == val) {
			v.erase(v.begin() + i);
			return;
		}
	}
}

void test()
{
	getInverseKeys();
}

void createBoneIndexLUT(bool withShuffling, bool withMerging, const std::vector<vertex_data>& vertexData, std::vector<glm::u16vec4>& lut, std::vector<uint16_t>* vertexLUIndexTable, std::vector<uint8_t>* vertexLUPermutation)
{
	assert(vertexLUIndexTable);	// Dont' allow no target for new VertexLUIndices

	uint32_t vertex_count = static_cast<uint32_t>(vertexData.size());

	// STEP 0: Create vector with all reduced bone index vectors
	std::vector<glm::u16vec4> adoptedBoneIndexVectors;	// for unused bones (weigth < epsilon) -> UINT16_MAX
	std::vector<uint16_t> luids(vertex_count);	// lookup index per vertex

	adoptedBoneIndexVectors.reserve(vertex_count);

	for (auto& vert : vertexData) {
		auto& tmp = adoptedBoneIndexVectors.emplace_back(vert.mBoneIndices);
		for (glm::length_t i = 0; i < 4; i++) {
			if (vert.mBoneWeights[i] <= 0.000001f) tmp[i] = UINT16_MAX;
		}
	}

	// STEP 1: Delete duplicates by using a hashmap
	IndexLookupHashMap reducedIndices;
	for (uint32_t vi = 0; vi < adoptedBoneIndexVectors.size(); vi++) {
		auto it = reducedIndices.find(adoptedBoneIndexVectors[vi]);
		if (it == reducedIndices.end()) {
			uint16_t newLuid = reducedIndices.size();
			reducedIndices[adoptedBoneIndexVectors[vi]] = newLuid;
			luids[vi] = newLuid;
		}
		else {
			luids[vi] = it->second;
		}
	}

	//writeAndOpenCSV(luids);

	// We have to protocol all changes to the luid numbers in this map, such that we efficiently can change them for all the vertices at the end
	LuidChangeMap luidChanges;

	// STEP 2: If sorting on, sort the index-vectors and again use an unordered_set to eliminate duplicates:
	if (withShuffling) {
		IndexLookupHashMap sortedIndices;
		for (const auto& pair : reducedIndices) {
			glm::u16vec4 sortedIndexVector = pair.first;
			sortVec4Inplace(sortedIndexVector);
			auto it = sortedIndices.find(sortedIndexVector);
			if (it == sortedIndices.end()) {
				uint16_t newLuid = sortedIndices.size();
				sortedIndices[sortedIndexVector] = newLuid;
				luidChanges[pair.second] = newLuid;
			}
			else {
				luidChanges[pair.second] = it->second;
			}
		}
		reducedIndices = std::move(sortedIndices);
	}

	// STEP 3: Order indices and delete ones that can be used by others (optimization also done in paper permutation coding. see there for more details)
	std::vector<std::pair<glm::u16vec4, uint16_t>> reducedIndicesWithID;
	reducedIndicesWithID.reserve(reducedIndices.size());
	for (const auto& pair : reducedIndices) reducedIndicesWithID.push_back(pair);
	std::sort(reducedIndicesWithID.begin(), reducedIndicesWithID.end(), cmpIndexVectorTupleForSort);
	LuidChangeMap luidChanges2;

	// NOTE: Now it can (and will) happen that a luid is pointing to a luid thats actually not in the list anymore, because
	// it was deleted in a successing step. Therefore we have to go temporarly save all the luids for this block and if a new better option
	// is found we have to change all of the luid transfers in the change map within the block. (sounds more complicated than it is)
	// SECOND NOTE: We dont need currentBlockLUIds if we have a seperate loop inside the loop for the block. (would probably be better/faster)
	std::vector<uint16_t> currentBlockLUIds;
	// Now only keep the last of each set, as this one should be the one that encompasses all of them
	for (uint16_t i = 0; i < reducedIndicesWithID.size() - 1; i++) {
		// Check the one after me, if it can be used in my stead then delete me
		if (cmpIndexVectorGoodEnough(reducedIndicesWithID[i].first, reducedIndicesWithID[i + 1].first)) {
			auto newLuid = reducedIndicesWithID[i + 1].second;
			for (const auto luid : currentBlockLUIds) luidChanges2[luid] = newLuid;
			luidChanges2[reducedIndicesWithID[i].second] = newLuid;
			currentBlockLUIds.push_back(reducedIndicesWithID[i].second);
			reducedIndicesWithID.erase(reducedIndicesWithID.begin() + i);
			i--;
		}
		else {
			currentBlockLUIds.clear();
		}
	}

	// Reset indices: (such that the index and the index inside the pair is actually the same)
	LuidChangeMap luidChanges3;
	for (uint16_t newLuid = 0; newLuid < reducedIndicesWithID.size(); newLuid++) {
		luidChanges3[reducedIndicesWithID[newLuid].second] = newLuid;
		reducedIndicesWithID[newLuid].second = newLuid;
	}

	// STEP 4: Merge Change Maps and apply to luids
	luidChanges = mergeChangeMaps(luidChanges, mergeChangeMaps(luidChanges2, luidChanges3));
	applyChangeMap(luids, luidChanges);	// Do we need that here? YES because of the basePermutations
	luidChanges.clear();

	// STEP 5: if withMerging is active lets try to combine the remaining entries such that we have as little "dont care" values as necessary
	// The offset can be saved in the resulting permutation for the vertex weights
	std::map<uint16_t, uint8_t> basePermutations;
	if (withShuffling && withMerging) {
		
		std::vector<uint16_t> bins[3]; // helper arrays. bins[0] contains all ids to entries with 1 empty field, bin[1] with 2 empty fields, bin[2] with 3 empty fields
		for (const auto& pair : reducedIndicesWithID) {
			int freeSlots = 0;
			for (int i = 0; i < 4; i++) if (pair.first[i] == UINT16_MAX) freeSlots++;
			if (freeSlots > 0) bins[freeSlots-1].push_back(pair.second);
		}

		// Lets do some kind of greedy approach: First fill the "big" ones meaning with three empty fields and the biggest one to fill (one empty field) and so on
		for (int currentGroup = 2; currentGroup > 0; currentGroup--) {
			while (!bins[currentGroup].empty()) {
				auto itmToFillID = bins[currentGroup].back();
				bins[currentGroup].pop_back();
				auto& itmToFill = reducedIndicesWithID[itmToFillID];
				float avgValueItmToFill = getU16Median(itmToFill.first);

				// Lets look for a partner starting with the smallest ones (would fill the most)
				for (int partnerGroup = 2 - currentGroup; partnerGroup <= currentGroup; partnerGroup++) {
					if (bins[partnerGroup].empty()) continue;

					// Search for the partner that shares the most indices. (MAYBE this one is actually neighbouring) <- just an assumption would be easier to just merge whatever possible.
					// If none found or multiple select the one that on median (or average then replace getU16Median with getU16Avg) comes closest to the current one 
					uint16_t bestPartner = UINT16_MAX;
					int bestPartnerSharedIDsCount = 0;
					float bestPartnerClosestAvgDistance = FLT_MAX;
					for (auto possiblePartner : bins[partnerGroup]) {
						const auto& partnerItm = reducedIndicesWithID[possiblePartner];
						auto sharedCount = getSharedIDCount(itmToFill.first, partnerItm.first);
						auto distance = glm::abs(avgValueItmToFill - getU16Median(partnerItm.first));
						if (sharedCount > bestPartnerSharedIDsCount || (sharedCount == bestPartnerSharedIDsCount && distance < bestPartnerClosestAvgDistance)) {
							bestPartner = possiblePartner;
							bestPartnerSharedIDsCount = sharedCount;
							bestPartnerClosestAvgDistance = distance;
						}
					}

					// By now we definitely found a partner, so merge by putting item inside partner (as partner has more valid ids and is MAYBE more important)
					// Theres only a few possible permutations that are possible given the source group and destination group
					uint8_t permutation;
					glm::u8vec4 newVec = reducedIndicesWithID[bestPartner].first;
					if (currentGroup == 2) {
						if (partnerGroup == 0) {			// itm: 10 X X X ; partner: 0 1 2 X ; res: 0 1 2 10
							permutation = 18;				// 0 1 2 10 -> 10 X X X
							newVec.w = itmToFill.first.x;
							removeFromUintVector(bins[partnerGroup], bestPartner);	// delete partner from bin (could be faster with map)
						}
						else if (partnerGroup == 1) {		// itm: 10 X X X ; partner: 0 1 X X ; res: 0 1 10 X
							permutation = 12;				// 0 1 10 X -> 10 X X X
							newVec.z = itmToFill.first.x;
							removeFromUintVector(bins[partnerGroup], bestPartner);
							bins[0].push_back(bestPartner);
						}
						else if (partnerGroup == 2) {		// itm: 10 X X X ; partner: 0 X X X ; res: 0 10 X X // NOTE: NEVER EXPLICIT TESTED BECAUSE NOT IN TESTDATA
							permutation = 6;				// 0 10 X X -> 10 X X X
							newVec.y = itmToFill.first.x;
							removeFromUintVector(bins[partnerGroup], bestPartner);
							bins[1].push_back(bestPartner);
						}
					}
					else if (currentGroup == 1) {
						if (partnerGroup == 1) {			// itm: 10 11 X X ; partner: 0 1 X X ; res: 0 1 10 11
							permutation = 16;				// 0 1 10 11 -> 10 11 X X
							newVec.z = itmToFill.first.x;
							newVec.w = itmToFill.first.y;
							removeFromUintVector(bins[partnerGroup], bestPartner);
						}
						else if (partnerGroup == 2) {		// itm: 10 11 X X ; partner: 0 X X X ; res: 0 10 11 X // NOTE: NEVER EXPLICIT TESTED BECAUSE NOT IN TESTDATA
							permutation = 8;				// 0 10 11 X -> 10 11 X X
							newVec.y = itmToFill.first.x;
							newVec.z = itmToFill.first.y;
							removeFromUintVector(bins[partnerGroup], bestPartner);
							bins[0].push_back(bestPartner);
						}
					}
					reducedIndicesWithID[bestPartner].first = newVec;
					luidChanges[itmToFillID] = bestPartner;
					itmToFill.second = UINT16_MAX; // mark for deletion
					basePermutations[itmToFillID] = permutation;
					break;

				} // end for (int partnerGroup = 2 - currentGroup; partnerGroup <= currentGroup; partnerGroup++) 
			} // end while
		} // end for (int currentGroup = 2; currentGroup > 0; currentGroup--)

		// Lets remove all marked items from the list and reset indices again
		for (int i = 0; i < reducedIndicesWithID.size(); i++) {
			if (reducedIndicesWithID[i].second == UINT16_MAX) {
				reducedIndicesWithID.erase(reducedIndicesWithID.begin() + i);
				i--;
			}
		}
		luidChanges2.clear();
		for (uint16_t newLuid = 0; newLuid < reducedIndicesWithID.size(); newLuid++) {
			luidChanges2[reducedIndicesWithID[newLuid].second] = newLuid;
			reducedIndicesWithID[newLuid].second = newLuid;
		}

		// merge luid changes:
		luidChanges = mergeChangeMaps(luidChanges, luidChanges2);
	} // end if

	// STEP 6: IF SHUFFELING IS ON LETS CALCULATE THE PERMUTATION PER VERTEX
	if (withShuffling) {
		if (!vertexLUPermutation) throw std::runtime_error("If shuffling is active, a target vertexLUPermutation buffer/vector has to be given.");
		vertexLUPermutation->clear();
		vertexLUPermutation->resize(vertex_count);
		if (basePermutations.size() > 0) {
			// In this case stuff was merged. luidChanges needs to be aplied one more time and we might have to respect basePermutations
			for (size_t vid = 0; vid < vertex_count; vid++) {
				auto luid = luids[vid];

				uint8_t basePermut = 0;
				auto basePermutIt = basePermutations.find(luid);
				if (basePermutIt != basePermutations.end()) {
					basePermut = basePermutIt->second;
				}

				// apply luid change, if any:
				auto luidChangeIt = luidChanges.find(luid);
				if (luidChangeIt != luidChanges.end()) {
					luids[vid] = luidChangeIt->second;
					luid = luids[vid];
				}

				auto bIndices = adoptedBoneIndexVectors[vid];
				uint8_t permutation;
				bIndices = sortUvec4(bIndices, permutation);
				permutation = combinePermutations(permutation, basePermut);
				(*vertexLUPermutation)[vid] = permutation;
				std::cout << glm::to_string(vertexData[vid].mBoneIndices) << " and " << glm::to_string(vertexData[vid].mBoneWeights) << " with " << glm::to_string(applyPermutationInverse(reducedIndicesWithID[luid].first, permutation)) << std::endl;
				
				//DEBUG CHECK:
				auto original = adoptedBoneIndexVectors[vid];
				auto fromlut = reducedIndicesWithID[luid].first;
				//auto fromlutWithPermutation = applyPermutation
				if (vid % 100 == 0) {

					std::cout << "Pause" << std::endl;
				}
			}
		}
		else {
			
			// In this case merging is either not activated or nothing was to merge. LUID is already applied. We just have to get Permutation
			for (size_t vid = 0; vid < vertex_count; vid++) {
				auto luid = luids[vid];
				auto bIndices = adoptedBoneIndexVectors[luid];
				uint8_t permutation;
				bIndices = sortUvec4(bIndices, permutation);
				(*vertexLUPermutation)[vid] = permutation;
			}
		}

	}

	// STEP 7: Write out vertexLUIndexTable. The content is inside reducedIndicesWithID
	(*vertexLUIndexTable) = std::move(luids);

	// STEP 8: Write out LUT
	lut.clear();
	lut.reserve(reducedIndicesWithID.size());
	for (const auto& pair : reducedIndicesWithID) lut.push_back(pair.first);
}
