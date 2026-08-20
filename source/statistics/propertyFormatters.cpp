/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "propertyFormatters.h"

 // Define the lambda for float
const std::function<std::string(const float&)> floatSiFormatter = [](const float& value) -> std::string {
	return siFormatter<float, 2, 0.5f, 1000>(value);
	};

// Define the lambda for unsigned int
const std::function<std::string(const unsigned int&)> uintSiFormatter = [](const unsigned int& value) -> std::string {
	return siFormatter<unsigned int, 2, 1, 1000>(value);
	};