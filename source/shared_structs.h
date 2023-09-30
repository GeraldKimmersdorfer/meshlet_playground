#include <glm/glm.hpp>
#include <meshlet_helpers.hpp>

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;

struct mesh_data {
	glm::mat4 mTransformationMatrix;
	uint32_t mVertexOffset;			// Offset to first item in Positions Texel-Buffer
	uint32_t mVertexCount;
	uint32_t mIndexOffset;			// Offset to first item in Indices Texel-Buffer
	uint32_t mIndexCount;			// Amount if indices
	uint32_t mMaterialIndex;		// index of material for mesh
	int32_t mAnimated = false;		// Index offset inside bone matrix buffer, -1 if not animated
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

// WARNING: Only works without padding because of the values of sNumVertices and sNumIndices
// If those are changed, the alignment will most likely not properly work. The meshlet_redirect
// approach is just better!!!
struct meshlet_native
{
	uint32_t mMeshIndex;
	uint32_t mVertices[sNumVertices];
	uint8_t mIndices[sNumIndices];
	uint8_t mVertexCount;
	uint8_t mTriangleCount;
};

struct meshlet_redirect
{
	uint32_t mDataOffset;
	uint32_t mMeshIdxVcTc;	// see packMeshIdxVcTc
};