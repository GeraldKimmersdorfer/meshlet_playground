#version 460
#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_shader_8bit_storage    : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "shared_structs.glsl"

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 2) uniform ConfigurationBuffer { config_data config; };
layout(set = 1, binding = 0) buffer Material { MaterialGpuData materials[]; } matSsbo;


layout (location = 0) in PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoord;
	flat int materialIndex;
	vec3 color;
} v_in;

layout (location = 0) out vec4 fs_out;

void main() 
{
	int matIndex = v_in.materialIndex;

	int diffuseTexIndex = matSsbo.materials[matIndex].mDiffuseTexIndex;
    vec3 color = texture(textures[diffuseTexIndex], v_in.texCoord).rgb;
	
	float ambient = 0.1;
	vec3 diffuse = matSsbo.materials[matIndex].mDiffuseReflectivity.rgb;
	vec3 toLight = normalize(vec3(1.0, 1.0, 0.5));
	vec3 illum = vec3(ambient) + diffuse * max(0.0, dot(normalize(v_in.normalWS), toLight));
	color *= illum;
	
	if(config.mOverlayMeshlets) {
		color = mix(color, v_in.color, 0.5);
	}

	fs_out = vec4(color, 1.0);
}
