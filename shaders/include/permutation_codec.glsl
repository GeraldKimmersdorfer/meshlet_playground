#define ENTRY_COUNT 3
#include "blend_attribute_compression.glsl"


const blend_attribute_codec_t codec_14bit = {36, {1, 1, 2}, 87382};
const blend_attribute_codec_t codec_16bit = {58, {1, 1, 2}, 21846};

vec4 decompress_pc_16x16(in uint code, out uint payload) {
	bool valid; // not really necessary...
	uvec2 code64 = uvec2(code, 0);
	float out_weights[ENTRY_COUNT + 1];
    payload = decompress_blend_attributes(out_weights, valid, code64, codec_16bit);
	return vec4(out_weights[3], out_weights[2], out_weights[1], out_weights[0]);
}

vec4 decompress_pc_14x18(in uint code, out uint payload) {
	bool valid; // not really necessary...
	uvec2 code64 = uvec2(code, 0);
	float out_weights[ENTRY_COUNT + 1];
    payload = decompress_blend_attributes(out_weights, valid, code64, codec_14bit);
	return vec4(out_weights[3], out_weights[2], out_weights[1], out_weights[0]);
}