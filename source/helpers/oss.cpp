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

#include "oss.h"

#include "../thirdparty/vbac_compression.h"
#include "../helpers/util.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>

void test_oss()
{
	auto weights = generateRandomWeights(50, WEIGHT_DESC_ORDER, true);

	for (auto& w : weights)
	{
		uint64_t result;
		auto info = vbac_oss_compress(&w.x, 1, 16, &result);

		// Make sure that result is inside 16 bits
		bool fits = result < (1ull << 16);
		if (!fits) {
			std::cout << "result does not fit in 16 bits" << std::endl;
		}

		glm::vec4 decoded;
		vbac_oss_decompress(&result, 1, info, &decoded.x);

		std::cout << "input:   " << glm::to_string(w) << std::endl;
		std::cout << "decoded: " << glm::to_string(decoded) << " [" << result << "]" << std::endl;
		std::cout << info.N << " " << info.MI4 << " " << info.scale << std::endl;
	}

}

vbac_oss_info oss_conf_16bit(104, 65231, 0.0048543689320388345);

uint16_t oss_compress(const glm::vec4& weights)
{
	// make sure in desc order
	assert(weights.x >= weights.y && weights.y >= weights.z && weights.z >= weights.w);
	uint64_t result;
	auto info = vbac_oss_compress(&weights.x, 1, 16, &result);
	assert(info.N == oss_conf_16bit.N && info.MI4 == oss_conf_16bit.MI4 && info.scale == oss_conf_16bit.scale);
	return static_cast<uint16_t>(result);
}

glm::vec4 oss_decompress(uint16_t compressed)
{
	glm::vec4 decoded;
	uint64_t compressed64 = compressed;
	vbac_oss_decompress(&compressed64, 1, oss_conf_16bit, &decoded.x);
	return decoded;
}
