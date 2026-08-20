/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "permcodec.h"
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <algorithm>

#include "util.h"

blend_attribute_codec_t PermutationCodec::codecDefault = {
		.entry_count = 3,
		.weight_value_count = 18,
		.extra_value_counts = {1,1,2}
};

// Generated with optimal_coding.py with print_codec_table([262144], [3], [32])
blend_attribute_codec_t PermutationCodec::codec14bit = { 3, 36, {1, 1, 2}, 87382 };

// Generated with optimal_coding.py with print_codec_table([65536], [3], [32])
blend_attribute_codec_t PermutationCodec::codec16bit = { 3, 58, {1, 1, 2}, 21846 };

uint32_t PermutationCodec::encode(glm::vec4 weights, uint32_t payload, blend_attribute_codec_t codec)
{
	float* weightParse = (float*)(void*)&weights[0];
	std::sort(weightParse, weightParse + 4);

	return compress_blend_attributes(weightParse, payload, codec);
}

void PermutationCodec::test()
{
	auto weights = generateRandomWeights(1000000, WEIGHT_ASC_ORDER, true);

	blend_attribute_codec_t codecSettings = codec16bit;

	uint64_t max = 0; uint64_t min = 0xFFFFFFFFFFFFFFFF;
	float error_sum = 0;
	float error_counts = 0;
	for (auto& w : weights)
	{
		uint32_t payload = 65535;// 0xFFFF;
		auto encoded = compress_blend_attributes(&w.x, payload, codecSettings);
		max = std::max(max, encoded);
		min = std::min(min, encoded);

		glm::vec4 decoded;
		int valid;
		decompress_blend_attributes(&decoded.x, &valid, encoded, codecSettings);
		//std::cout << "input: " << glm::to_string(w) << " output: " << glm::to_string(decoded) << std::endl;
		error_sum += glm::length(decoded - w);
		error_counts += 1.0f;
	}
	int bit_depth = 0;
	if (max > 0) {
		bit_depth = static_cast<int>(std::ceil(std::log2(max + 1)));
		// Round up to the nearest multiple of 8
		//bit_depth = ((bit_depth + 7) / 8) * 8;
	}

	std::cout << "min: " << min << " max: " << max << " (" << bit_depth << " bits)" << std::endl;
	std::cout << "average error: " << error_sum / error_counts << std::endl;
}
