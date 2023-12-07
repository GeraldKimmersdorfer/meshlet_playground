#pragma once

#include <glm/glm.hpp>
#include "../thirdparty/permutation_coding.h"

class PermutationCodec {


public:

	static uint32_t encode(glm::vec4 weights, uint32_t payload);

	static void test();


private:

	static blend_attribute_codec_t mCodecSettings;

};