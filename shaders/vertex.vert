#version 460
#extension GL_GOOGLE_include_directive	 : require
#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_shader_8bit_storage    : require	// necessary because of shared_structs.glsl
#extension GL_EXT_nonuniform_qualifier   : require
#extension GL_EXT_scalar_block_layout 	 : require

#include "include/mcc.glsl"

#define MCC_VERTEX_GATHER_TYPE _PULL // possible values: _PULL,_PUSH
#define MCC_VERTEX_COMPRESSION _NOCOMP 	// possible values: _NOCOMP, _LUT

#include "include/glsl_helpers.glsl"
#include "include/shared_structs.glsl"
#include "include/vertex_reconstruction.glsl"

#if MCC_VERTEX_GATHER_TYPE == _PUSH
layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uvec4 inBoneIndices;
layout (location = 4) in vec4 inBoneWeights;
#endif

layout(set = 0, binding = 1) uniform CameraBuffer { camera_data camera; };
layout(set = 0, binding = 2) uniform ConfigurationBuffer { config_data config; };
layout(set = 2, binding = 0) buffer BoneTransformBuffer { bone_data bones[]; };
layout(set = 4, binding = 1) buffer MeshBuffer { mesh_data meshes[]; };

layout (location = 0) out PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoord;
	flat int materialIndex;
	vec3 color;
} v_out;

layout(push_constant) uniform PushConstants {
	copy_push_data copy;
};

// NOTE: This function is by far more performant than the ones in glsl_helper
void boneTransform(in vec4 boneWeights, in uvec4 boneIndices, inout vec4 posMshSp, inout vec3 nrmMshSp) {
	mat4 skinMatrix = mat4(0.0);
	for (uint i = 0; i < 4; i++) {
		if (boneWeights[i] > BONE_WEIGHT_EPSILON) {
			skinMatrix += boneWeights[i] * bones[boneIndices[i]].transform;
		}  else break; // (if boneWeights are sorted)
	}
	posMshSp = skinMatrix * posMshSp;
	nrmMshSp = normalize(mat3(skinMatrix) * nrmMshSp);
}

void main() {

#if MCC_VERTEX_GATHER_TYPE == _PULL
	vertex_data vertex = getVertexData(gl_VertexIndex, 0);
#elif MCC_VERTEX_GATHER_TYPE == _PUSH
	vertex_data vertex = vertex_data(inPosition, inNormal, inTexCoord, inBoneIndices, inBoneWeights);
#endif

	uint meshIndex 			  = gl_InstanceIndex;
	mat4 transformationMatrix = meshes[meshIndex].mTransformationMatrix;
	bool isAnimated 		  = meshes[meshIndex].mAnimated;
	uint materialIndex        = meshes[meshIndex].mMaterialIndex;

	vec3 posLocal = fma(vertex.mPosition, vec3(meshes[meshIndex].mPositionNormalizationInvScale), vec3(meshes[meshIndex].mPositionNormalizationInvTranslation));

	vec4 posMshSp = vec4(posLocal, 1.0);
	vec3 nrmMshSp = vertex.mNormal;

	if (isAnimated) {
		boneTransform(vertex.mBoneWeights, vertex.mBoneIndices, posMshSp, nrmMshSp);
	}

	// Standard transformation:
	vec4 posWS = transformationMatrix * posMshSp + copy.mOffset;
	vec4 posCS = camera.mViewProjMatrix * posWS;

	gl_Position = posCS;

	v_out.positionWS = posWS.xyz;
	v_out.normalWS = mat3(transformationMatrix) * nrmMshSp;
	v_out.texCoord = vertex.mTexCoord;
	v_out.materialIndex = int(materialIndex);
	v_out.color = color_from_id_hash(gl_InstanceIndex); 
}

