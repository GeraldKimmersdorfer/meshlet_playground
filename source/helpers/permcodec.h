/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <glm/glm.hpp>
#include "../thirdparty/permutation_coding.h"

class PermutationCodec {


public:

	static uint32_t encode(glm::vec4 weights, uint32_t payload, blend_attribute_codec_t codec);

	static void test();

	static blend_attribute_codec_t codecDefault;
	static blend_attribute_codec_t codec14bit; // 14 bit weights, 18 bit payload
	static blend_attribute_codec_t codec16bit; // 16 bit weights, 16 bit payload


private:



};