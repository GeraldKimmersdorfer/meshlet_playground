#pragma once

#include <glm/glm.hpp>

#define MAX_INSTANCE_COUNT 500
#define BONE_WEIGHT_EPSILON 0.0000001

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;
static constexpr size_t sNumPackedIndices = 95; //avk::div_ceil(sNumIndices, 4);



struct mesh_data {
	glm::mat4 mTransformationMatrix;
	glm::vec4 mPositionNormalizationInvScale;
	glm::vec4 mPositionNormalizationInvTranslation;
	uint32_t mVertexOffset;			// Offset to first item in general Vertex-Buffer
	uint32_t mVertexCount;			// Amount of vertices in general Vertex-Buffer
	uint32_t mIndexOffset;			// Offset to first item in general Indices-Buffer
	uint32_t mIndexCount;			// Amount if indices in general Indices-Buffer
	uint32_t mMaterialIndex;		// Material index for given mesh 
	int32_t mAnimated = false;
	glm::vec2 padding;
};

struct vertex_data {
	glm::vec4 mPositionTxX;
	glm::vec4 mTxYNormal;
	glm::uvec4 mBoneIndices;
	glm::vec4 mBoneWeights;
};	// mind padding and alignment!

struct vertex_data_bone_lookup {
	glm::vec4 mPositionTxX;
	glm::vec4 mTxYNormal;
	glm::vec3 mBoneWeights;
	uint32_t mBoneIndicesLUID;
	glm::vec4 mBoneWeightsGT;
	glm::uvec4 mBoneIndicesGT;
};

// 20 byte
struct vertex_data_permutation_coding {
	glm::u32vec2 mPosition;	// each component 21 bit
	uint32_t mNormal;
	uint32_t mTexCoords;
	uint32_t mBoneData;
};

struct vertex_data_meshlet_coding {
	glm::uvec4 mPosition;
	glm::vec4 mNormal;
	glm::vec4 mTexCoord;
	glm::vec4 mBoneWeights;
	glm::uvec4 mBoneIndicesLUID;
};

/*
struct vertex_data_meshlet_coding {
	uint64_t mPosition;	// each component 21 bit
	uint32_t mNormal;
	uint32_t mTexCoord;
	// WEIGHTS = 25 bit, actually available for the weights (more than in permut coding paper)
	// MBILUID = 2 bit, id of the (up to 4) luids inside the meshlet_data (Meshlet Bone Index Lookup ID)
	// PERMUTATION = 5 bit, 0-23, defines how the bone indices need to be shuffled
	uint32_t mWeightsImbiluidPermutation;
};*/

struct config_data {
	uint32_t mOverlayMeshlets = true;
	uint32_t mMeshletsCount = 0;
	uint32_t mInstanceCount = 2;
	uint32_t padding;
	glm::vec4 mInstancingOffset = { 0.0f , 0.0f, 1.0f , 0.0f };
};

// NOTE: We end up with massive amounts
// of storage unused... The meshlet_redirect approach is just better!!!
struct meshlet_native
{
	uint32_t mMeshIdxVcTc;	// see packMeshIdxVcTc
	uint32_t mVertices[sNumVertices];
	uint32_t mIndicesPacked[sNumPackedIndices];
};

struct meshlet_redirect
{
	uint32_t mDataOffset;
	uint32_t mMeshIdxVcTc;	// see packMeshIdxVcTc
};