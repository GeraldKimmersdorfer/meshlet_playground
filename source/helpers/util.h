/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>

enum WeightOrder {
	WEIGHT_NO_ORDER,
	WEIGHT_ASC_ORDER,
	WEIGHT_DESC_ORDER
};

std::vector<glm::vec4> generateRandomWeights(int n, WeightOrder order = WEIGHT_NO_ORDER, bool includeExtremeCases = false);

void setClipboardText(const std::string& text);