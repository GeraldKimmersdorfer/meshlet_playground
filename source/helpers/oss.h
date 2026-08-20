/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <glm/glm.hpp>

void test_oss();

uint16_t oss_compress(const glm::vec4& weights);

glm::vec4 oss_decompress(uint16_t compressed);