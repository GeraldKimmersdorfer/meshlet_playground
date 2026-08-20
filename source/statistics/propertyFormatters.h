/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <cmath>
#include <functional>



template <typename T, int Precision = 2, T Threshold = 0.5, int Base = 1000>
std::string siFormatter(const T value, const std::string& unit = "") {
	static_assert(std::is_same<T, float>::value || std::is_same<T, unsigned int>::value,
		"siFormatter only supports float and unsigned int");

	std::stringstream ss;
	T absValue = value;

	if constexpr (std::is_same<T, float>::value) {
		absValue = value < 0 ? -value : value;
	}

	const char* prefixes[] = { "n", "u", "m", "", "k", "M", "G" };
	int prefixIndex = 3; // Start with no prefix

	// Determine the correct prefix
	if (absValue < Threshold) {
		while (absValue < Threshold && prefixIndex > 0) {
			absValue *= Base;
			--prefixIndex;
		}
	}
	else {
		while (absValue >= Base && prefixIndex < 6) {
			absValue /= Base;
			++prefixIndex;
		}
	}

	ss << std::fixed << std::setprecision(Precision) << absValue;

	std::string formattedValue = ss.str();

	// Remove trailing zeros and the decimal point if not necessary
	if (formattedValue.find('.') != std::string::npos) {
		// Erase trailing zeros
		formattedValue.erase(formattedValue.find_last_not_of('0') + 1);
		// Erase decimal point if it is the last character
		if (formattedValue.back() == '.') {
			formattedValue.pop_back();
		}
	}

	return formattedValue + prefixes[prefixIndex] + unit;
}

template <typename T>
std::function<std::string(const T&)> genericSiFormatter() {
	if constexpr (std::is_same_v<T, float>) {
		return [](const T& value) -> std::string {
			return siFormatter<float>(value);
			};
	}
	else if constexpr (std::is_same_v<T, unsigned int>) {
		return [](const T& value) -> std::string {
			return siFormatter<unsigned int>(value);
			};
	}
	else {
		static_assert(std::is_same<T, float>::value || std::is_same<T, unsigned int>::value,
			"genericSiFormatter only supports float and unsigned int");
	}
}