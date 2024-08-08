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