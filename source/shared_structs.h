#pragma once

#include <glm/glm.hpp>

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;
static constexpr size_t sNumPackedIndices = 95; //avk::div_ceil(sNumIndices, 4);

struct mesh_data {
	glm::mat4 mTransformationMatrix;
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

struct config_data {
	uint32_t mOverlayMeshlets = true;
	uint32_t mMeshletsCount = 0;
	glm::uvec2 padding;
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