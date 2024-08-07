#pragma once

#include <glm/glm.hpp>

void test_oss();

uint16_t oss_compress(const glm::vec4& weights);

glm::vec4 oss_decompress(uint16_t compressed);