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

struct mesh_data
{
	mat4 mTransformationMatrix;
    uint mOffsetPositions;	// Offset to first item in Positions Texel-Buffer
	uint mOffsetTexCoords;	// Offset to first item in TexCoords Texel-Buffer
	uint mOffsetNormals;	// Offset to first item in Normals Texel-Buffer
	uint mOffsetIndices;	// Offset to first item in Indices Texel-Buffer (unused yet)
	uint mMaterialIndex;
};

struct camera_data
{
    mat4 mViewProjMatrix;
};