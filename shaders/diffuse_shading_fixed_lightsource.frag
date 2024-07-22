#version 460
#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_shader_8bit_storage    : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "include/shared_structs.glsl"

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
	flat vec3 colorFlat;
} v_in;

layout (location = 0) out vec4 fs_out;

void main() 
{
	int matIndex = v_in.materialIndex;

	int diffuseTexIndex = matSsbo.materials[matIndex].mDiffuseTexIndex;
    vec4 colora = texture(textures[diffuseTexIndex], v_in.texCoord);
	if (colora.a < 0.2) discard;
	vec3 color = colora.rgb;

	vec3 overlayColor = v_in.colorFlat;
	if(config.overlayIndex > 0) {
		if (config.overlayIndex == 2) overlayColor = vec3(0.0, 0.0, 0.0);
		if (config.overlayPreShading == 1) {
			color = mix(color, overlayColor, config.overlayStrength);
		}
	}
	
	float ambient = config.lightAmbientStrength;
	vec3 diffuse = matSsbo.materials[matIndex].mDiffuseReflectivity.rgb;
	vec3 toLight = normalize(vec3(1.0, 1.0, 0.5));
	vec3 illum = vec3(ambient) + config.lightDiffuseStrength * diffuse * max(0.0, dot(normalize(v_in.normalWS), toLight));
	color *= illum;
	
	if (config.overlayIndex > 0 && config.overlayPreShading == 0) {
		color = mix(color, overlayColor, config.overlayStrength);
	}

	fs_out = vec4(color, 1.0);
}
