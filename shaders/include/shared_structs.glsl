#define S_NUM_VERTICES 64
#define S_NUM_INDICES 378
#define S_NUM_PACKED_INDICES 95 // avk::div_ceil(sNumIndices, 4)

struct meshlet_native {
	uint mMeshIdxVcTc;	// see packMeshIdxVcTc
	uint mVertices[S_NUM_VERTICES];
	uint mIndicesPacked[S_NUM_PACKED_INDICES];
};

struct meshlet_redirected {
	uint mDataOffset;
	uint mMeshIdxVcTc;
};

struct mesh_data {
	mat4 mTransformationMatrix;
	vec4 mPositionNormalizationInvScale;
	vec4 mPositionNormalizationInvTranslation;
	uint mVertexOffset;		// Offset to first item in Positions Texel-Buffer
	uint mVertexCount;
	uint mIndexOffset;		// Offset to first item in Indices Texel-Buffer
	uint mIndexCount;		// Amount if indices
	uint mMaterialIndex;	// index of material for mesh
	bool mAnimated;	// Index offset inside bone matrix buffer, -1 if not animated
	int p1;int p2;
};

struct vertex_data {
	vec3 mPosition;
	vec3 mNormal;
	vec2 mTexCoord;
	uvec4 mBoneIndices;
	vec4 mBoneWeights;
};

struct vertex_data_no_compression {
	vec4 mPositionTxX;
	vec4 mTxYNormal;
	uvec4 mBoneIndices;
	vec4 mBoneWeights;
};

struct vertex_data_bone_lookup {
	vec4 mPositionTxX;
	vec4 mTxYNormal;
	vec3 mBoneWeights;
	uint mBoneIndicesLUID;
};

struct vertex_data_meshlet_coding {
	uvec4 mPosition;
	vec4 mNormal;
	vec4 mTexCoord;
	vec4 mBoneWeights;
	uvec4 mBoneIndicesLUID;
};

struct bone_data {
	mat4 transform;
};

struct camera_data
{
    mat4 mViewProjMatrix;
};

struct config_data {
	bool mOverlayMeshlets;
	uint mMeshletsCount;
	uint p1;
	uint p2;
};

struct MaterialGpuData
{
	vec4 mDiffuseReflectivity;
	vec4 mAmbientReflectivity;
	vec4 mSpecularReflectivity;
	vec4 mEmissiveColor;
	vec4 mTransparentColor;
	vec4 mReflectiveColor;
	vec4 mAlbedo;

	float mOpacity;
	float mBumpScaling;
	float mShininess;
	float mShininessStrength;
	
	float mRefractionIndex;
	float mReflectivity;
	float mMetallic;
	float mSmoothness;
	
	float mSheen;
	float mThickness;
	float mRoughness;
	float mAnisotropy;
	
	vec4 mAnisotropyRotation;
	vec4 mCustomData;
	
	int mDiffuseTexIndex;
	int mSpecularTexIndex;
	int mAmbientTexIndex;
	int mEmissiveTexIndex;
	int mHeightTexIndex;
	int mNormalsTexIndex;
	int mShininessTexIndex;
	int mOpacityTexIndex;
	int mDisplacementTexIndex;
	int mReflectionTexIndex;
	int mLightmapTexIndex;
	int mExtraTexIndex;
	
	vec4 mDiffuseTexOffsetTiling;
	vec4 mSpecularTexOffsetTiling;
	vec4 mAmbientTexOffsetTiling;
	vec4 mEmissiveTexOffsetTiling;
	vec4 mHeightTexOffsetTiling;
	vec4 mNormalsTexOffsetTiling;
	vec4 mShininessTexOffsetTiling;
	vec4 mOpacityTexOffsetTiling;
	vec4 mDisplacementTexOffsetTiling;
	vec4 mReflectionTexOffsetTiling;
	vec4 mLightmapTexOffsetTiling;
	vec4 mExtraTexOffsetTiling;
};