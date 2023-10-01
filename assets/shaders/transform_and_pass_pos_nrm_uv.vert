#version 460
#extension GL_EXT_shader_8bit_storage    : require	// necessary because of shared_structs.glsl
#extension GL_GOOGLE_include_directive	 : require

#include "shared_structs.glsl"
#include "glsl_helpers.glsl"

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uvec4 inBoneIndices;
layout (location = 4) in vec4 inBoneWeights;

layout(set = 0, binding = 1) uniform CameraBuffer { camera_data camera; };
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

void main() {
	mesh_data mesh = meshes[gl_InstanceIndex];
	mat4 transformationMatrix = mesh.mTransformationMatrix;
	vec4 posMshSp = vec4(inPosition, 1.0);
	vec3 nrmMshSp = inNormal;
	if (mesh.mAnimated) {
		vec4 boneWeights = inBoneWeights;
		uvec4 boneIndices = inBoneIndices;
		posMshSp = bone_transform(
			bones[boneIndices[0]].transform, bones[boneIndices[1]].transform, 
			bones[boneIndices[2]].transform, bones[boneIndices[3]].transform,
			boneWeights, posMshSp
		);
		nrmMshSp = bone_transform(
			bones[boneIndices[0]].transform, bones[boneIndices[1]].transform,
			bones[boneIndices[2]].transform, bones[boneIndices[3]].transform,
			boneWeights, nrmMshSp
		);
	}
	vec4 posWS = transformationMatrix * posMshSp;
	vec4 posCS = camera.mViewProjMatrix * posWS;

	gl_Position = posCS;

	v_out.positionWS = posWS.xyz;
	v_out.normalWS = mat3(transformationMatrix) * nrmMshSp;
	v_out.texCoord = inTexCoord;
	v_out.materialIndex = int(mesh.mMaterialIndex);
	v_out.color = color_from_id_hash(gl_InstanceIndex, vec3(1.0, 0.5, 0.5)); 
}

