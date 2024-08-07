#pragma once

#include <vector>
#include <glm/glm.hpp>

enum WeightOrder {
	WEIGHT_NO_ORDER,
	WEIGHT_ASC_ORDER,
	WEIGHT_DESC_ORDER
};

std::vector<glm::vec4> generateRandomWeights(int n, WeightOrder order = WEIGHT_NO_ORDER, bool includeExtremeCases = false);
