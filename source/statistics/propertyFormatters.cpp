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

#include "propertyFormatters.h"

 // Define the lambda for float
const std::function<std::string(const float&)> floatSiFormatter = [](const float& value) -> std::string {
	return siFormatter<float, 2, 0.5f, 1000>(value);
	};

// Define the lambda for unsigned int
const std::function<std::string(const unsigned int&)> uintSiFormatter = [](const unsigned int& value) -> std::string {
	return siFormatter<unsigned int, 2, 1, 1000>(value);
	};