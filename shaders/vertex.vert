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
	flat vec3 colorFlat;
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
	mat4 transformationMatrix = meshes[meshIndex].transformationMatrix;
	bool isAnimated 		  = meshes[meshIndex].animated;
	uint materialIndex        = meshes[meshIndex].materialIndex;

	// Scale position and texcoord from [0,1] to the actual bounds
	const vec3 posLocal = fma(vertex.mPosition, vec3(meshes[meshIndex].positionScale), vec3(meshes[meshIndex].positionTranslation));
	const vec2 texCoord = fma(vertex.mTexCoord, meshes[meshIndex].texCoordsTranslationScale.zw, meshes[meshIndex].texCoordsTranslationScale.xy);

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
	v_out.texCoord = texCoord;
	v_out.materialIndex = int(materialIndex);

	switch (config.overlayIndex) {
		case 100: v_out.color = color_from_id_hash(gl_VertexIndex, config.hashColorTint.rgb); break;
		case 101: v_out.color = color_from_id_hash(meshIndex, config.hashColorTint.rgb); break;
		case 102: v_out.color = vec3(1.0, 0.0, 0.0); break; // for meshlet pipeline
		case 103: v_out.color = color_from_id_hash(global_tuple_index, config.hashColorTint.rgb); break;
		case 104: v_out.color = sortVec4HighLow(vertex.mBoneWeights).rgb * vec3(1.0, 2.0, 3.0); break;
		case 105: v_out.color = vec3(1.0, 0.0, 0.0); break; // for meshlet pipeline
		case 200: v_out.color = vec3(1.0, 0.0, 0.0); break; // for meshlet pipeline
	}
	v_out.colorFlat = v_out.color; 
}

