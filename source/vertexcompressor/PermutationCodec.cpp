#include "PermutationCodec.h"
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <algorithm>

blend_attribute_codec_t PermutationCodec::mCodecSettings = {
		.entry_count = 3,
		.weight_value_count = 18,
		.extra_value_counts = {1,1,2}
};

uint32_t PermutationCodec::encode(glm::vec4 weights, uint32_t payload)
{
	float* weightParse = (float*)(void*)&weights[0];
	std::sort(weightParse, weightParse + 4);

	return compress_blend_attributes(weightParse, payload, mCodecSettings);
}

float glm_sum(const glm::vec3& vec) {
	return vec.x + vec.y + vec.z;
}

glm::vec4 generateRandomWeightVector() {
	glm::vec3 rand = glm::linearRand(glm::vec3(0.0), glm::vec3(1.0));

	while (glm_sum(rand) > 1.0f) {
		rand -= glm::vec3(0.01);
		rand = glm::clamp(rand, glm::vec3(0.0), glm::vec3(1.0));
	}
	glm::vec4 weights = glm::vec4(rand, 1.0 - glm_sum(rand));
	float* weightParse = (float*)(void*)&weights[0];
	std::sort(weightParse, weightParse + 4);
	return weights;
}

void PermutationCodec::test()
{
	auto codec = blend_attribute_codec_t{
	.entry_count = 3,
	.weight_value_count = 18,
	.extra_value_counts = {1,1,2}
	};
	for (uint32_t payload = 0; payload <= 0x001FFFFF; payload++) {
		auto weights = generateRandomWeightVector();
		auto code = compress_blend_attributes((float*)(void*)&weights[0], payload, codec);
		int isValid; float weightsDecode[4]; uint32_t tupleDecode;
		tupleDecode = decompress_blend_attributes(weightsDecode, &isValid, code, codec);
		if (tupleDecode != payload) {
			std::cerr << "Payload " << payload << " Decode-Error (" << tupleDecode << ")" << std::endl;
		}
		if (code > UINT32_MAX) {
			std::cerr << "Payload " << payload << " can not be saved in uint32_t (Code: " << code << ")" << std::endl;
		}
	}
}


