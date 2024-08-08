/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
 
#define S_NUM_VERTICES 64
#define S_NUM_INDICES 378
#define S_NUM_PACKED_INDICES 95 // avk::div_ceil(sNumIndices, 4)

#define BONE_WEIGHT_EPSILON 0.0000001

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
	mat4 transformationMatrix;
	vec4 positionScale;
	vec4 positionTranslation;
	vec4 texCoordsTranslationScale;
	uint materialIndex;	// index of material for mesh
	bool animated;	// Index offset inside bone matrix buffer, -1 if not animated
	int p1;int p2;
};

struct vertex_data {
	vec3 mPosition;
	vec3 mNormal;
	vec2 mTexCoord;
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
	uvec2 mPosition;	// each component 21 bit
	uint mNormal;
	uint mTexCoords;
	// WEIGHTS = 25 bit, actually available for the weights (more than in permut coding paper)
	// MBILUID = 2 bit, id of the (up to 4) luids inside the meshlet_data (Meshlet Bone Index Lookup ID)
	// PERMUTATION = 5 bit, 0-23, defines how the bone indices need to be shuffled
	uint mWeightsImbiluidPermutation;
	uint padding;	// even in scalar layouts we need 64 bit padding (8 byte)
};

struct vertex_data_permutation_coding {
	uvec2 mPosition;	// each component 21 bit
	uint mNormal;
	uint mTexCoords;
	uint mBoneData;
	uint padding;
};

struct bone_data {
	mat4 transform;
};

struct camera_data
{
    mat4 mViewProjMatrix;
};



struct copy_push_data {
	uvec4 mID;
	vec4 mOffset;
};

struct config_data {
	uint overlayIndex;
	uint mMeshletsCount;
	uint mCopyCount;
	uint padding;
	vec4 mCopyOffset;
	float overlayStrength;
	uint overlayPreShading;
	float lightAmbientStrength;
	float lightDiffuseStrength;
	vec4 hashColorTint;
	uint highlightedMeshletIndex;
	bool discardAllFragments;
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