#version 460
#extension GL_EXT_shader_8bit_storage    : require	// necessary because of shared_structs.glsl
#extension GL_GOOGLE_include_directive	 : require

#include "shared_structs.glsl"
#include "glsl_helpers.glsl"

layout(set = 0, binding = 1) uniform CameraBuffer { camera_data camera; };
layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data vertices[]; };
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
	int vi = gl_VertexIndex;
	vertex_data vertex = vertices[vi];
	vec4 posMshSp = vec4(vertex.mPositionTxX.xyz, 1.0);
	vec3 nrmMshSp = vertex.mNormalTxY.xyz;
	vec4 posWS = transformationMatrix * posMshSp;
	vec4 posCS = camera.mViewProjMatrix * posWS;

	gl_Position = posCS;

	v_out.positionWS = posWS.xyz;
	v_out.normalWS = mat3(transformationMatrix) * nrmMshSp;
	v_out.texCoord = vec2(vertex.mPositionTxX.w, vertex.mNormalTxY.w);
	v_out.materialIndex = int(mesh.mMaterialIndex);
	v_out.color = color_from_id_hash(gl_InstanceIndex, vec3(1.0, 0.5, 0.5)); 
}

