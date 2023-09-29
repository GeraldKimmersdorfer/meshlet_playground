#include <glm/glm.hpp>
#include <meshlet_helpers.hpp>

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;

struct meshlet_data
{
	uint32_t mMeshIndex;
	uint32_t mVertices[sNumVertices];
	uint8_t mIndices[sNumIndices];
	uint8_t mVertexCount;
	uint8_t mPrimitiveCount;
};

struct meshlet_data_index_buffer
{
	uint32_t mMeshIndex;		// The index of the mesh this meshlet belongs to (to gather transform, material, etc.)
	uint32_t mIndexOffset;		// Index offset into the meshlet sorted index buffer (Indices are aligned inside this buffer from [mIndexOffset -> mIndexOffset + mPrimitiveCount/3u]
	uint8_t mVertexCount;		// The amount of vertices that belong to this meshlet (nvidia recommendation: < 64)
	uint8_t mPrimitiveCount;	// The amount of triangles that belong to this meshlet (nvidia recommendation: < 126)
};

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
	uint32_t mMeshletsFrom = 0;
	uint32_t mMeshletsTo = 0;
	uint32_t padding;
};

/** The meshlet we upload to the gpu with its additional data. */
struct alignas(16) meshlet
{
	uint32_t mMeshIndex;
	avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
};

struct alignas(16) meshlet_redirect
{
	uint32_t mMeshIndex;
	avk::meshlet_redirected_gpu_data mGeometry;
};