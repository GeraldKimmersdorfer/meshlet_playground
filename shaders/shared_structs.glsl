struct meshlet
{
	uint    mVertices[64];
	uint8_t mIndices[378]; // 126 triangles * 3 indices
	uint8_t mVertexCount;
	uint8_t mTriangleCount;
};

struct extended_meshlet
{
	uint mMeshIndex;
	meshlet mGeometry;
};

struct mesh_data {
	mat4 mTransformationMatrix;
	uint mVertexOffset;		// Offset to first item in Positions Texel-Buffer
	uint mIndexOffset;		// Offset to first item in Indices Texel-Buffer
	uint mIndexCount;		// Amount if indices
	uint mMaterialIndex;	// index of material for mesh
	bool mAnimated;	// Index offset inside bone matrix buffer, -1 if not animated
	int p1;int p2;int p3;
};

struct vertex_data {
	vec4 mPositionTxX;
	vec4 mNormalTxY;
	uvec4 mBoneIndices;
	vec4 mBoneWeights;
};

struct bone_data {
	mat4 transform;
};

struct camera_data
{
    mat4 mViewProjMatrix;
};