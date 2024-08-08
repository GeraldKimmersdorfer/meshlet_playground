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