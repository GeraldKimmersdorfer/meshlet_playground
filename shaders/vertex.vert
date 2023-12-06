#version 460
#extension GL_EXT_shader_8bit_storage    : require	// necessary because of shared_structs.glsl
#extension GL_GOOGLE_include_directive	 : require
#include "include/mcc.glsl"

#define MCC_VERTEX_GATHER_TYPE _PULL // possible values: _PULL,_PUSH

#include "include/shared_structs.glsl"
#include "include/glsl_helpers.glsl"

#if MCC_VERTEX_GATHER_TYPE == _PUSH
layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uvec4 inBoneIndices;
layout (location = 4) in vec4 inBoneWeights;
#endif

layout(set = 0, binding = 1) uniform CameraBuffer { camera_data camera; };
layout(set = 2, binding = 0) buffer BoneTransformBuffer { bone_data bones[]; };
layout(set = 4, binding = 1) buffer MeshBuffer { mesh_data meshes[]; };
#if MCC_VERTEX_GATHER_TYPE == _PULL
layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data_no_compression vertices[]; };
#endif

layout (location = 0) out PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoord;
	flat int materialIndex;
	vec3 color;
} v_out;

// NOTE: This function is by far more performant than the ones in glsl_helper
void boneTransform(in vec4 boneWeights, in uvec4 boneIndices, inout vec4 posMshSp, inout vec3 nrmMshSp) {
	mat4 skinMatrix = mat4(0.0);
	for (uint i = 0; i < 4; i++) {
		if (boneWeights[i] > BONE_WEIGHT_EPSILON) {
			skinMatrix += boneWeights[i] * bones[boneIndices[i]].transform;
		} // else break (if boneWeights are sorted)
	}
	posMshSp = skinMatrix * posMshSp;
	nrmMshSp = normalize(mat3(skinMatrix) * nrmMshSp);
}

void main() {
	mesh_data mesh = meshes[gl_InstanceIndex];
#if MCC_VERTEX_GATHER_TYPE == _PULL
	vertex_data_no_compression vertex = vertices[gl_VertexIndex];
#endif

	mat4 transformationMatrix = mesh.mTransformationMatrix;
#if MCC_VERTEX_GATHER_TYPE == _PULL
	vec4 posMshSp = vec4(vertex.mPositionTxX.xyz, 1.0);
	vec3 nrmMshSp = vertex.mTxYNormal.yzw;
	vec2 texCoord = vec2(vertex.mPositionTxX.w, vertex.mTxYNormal.x);
#elif MCC_VERTEX_GATHER_TYPE == _PUSH
	vec4 posMshSp = vec4(inPosition, 1.0);
	vec3 nrmMshSp = inNormal;
	vec2 texCoord = inTexCoord;
#endif
	vec3 posLocal = posMshSp.xyz * vec3(mesh.mPositionNormalizationInvScale) + vec3(mesh.mPositionNormalizationInvTranslation);

	posMshSp = vec4(posLocal, 1.0);

	if (mesh.mAnimated) {
#if MCC_VERTEX_GATHER_TYPE == _PULL
		vec4 boneWeights = vertex.mBoneWeights;
		uvec4 boneIndices = vertex.mBoneIndices;
#elif MCC_VERTEX_GATHER_TYPE == _PUSH
		vec4 boneWeights = inBoneWeights;
		uvec4 boneIndices = inBoneIndices;
#endif
		boneTransform(boneWeights, boneIndices, posMshSp, nrmMshSp);
	}

	vec4 posWS = transformationMatrix * posMshSp;
	vec4 posCS = camera.mViewProjMatrix * posWS;

	gl_Position = posCS;

	v_out.positionWS = posWS.xyz;
	v_out.normalWS = mat3(transformationMatrix) * nrmMshSp;
	v_out.texCoord = texCoord;
	v_out.materialIndex = int(mesh.mMaterialIndex);
	v_out.color = color_from_id_hash(gl_InstanceIndex); 
}

