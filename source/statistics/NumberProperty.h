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

#include <string>
#include <mutex>
#include <type_traits>
#include "PropertyInterface.h"
#include "propertyFormatters.h"

template <typename T, typename AvgType = std::nullptr_t>
class NumberProperty :public PropertyInterface {

	using FormatterFunc = std::function<std::string(const T&)>;

	static_assert(
		std::is_same<T, float>::value ||
		std::is_same<T, unsigned int>::value,
		"NumberProperty only supports float and unsigned int"
		);

public:
	NumberProperty(const std::string& name, FormatterFunc formatter = genericSiFormatter)
		: PropertyInterface(name), m_formatter(formatter)
	{}


	void setValue(const T& newValue) {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		m_property = newValue;
	}

	T getValue() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		return m_property;
	}

	std::string getValueAsFormattedString() override {
		return m_formatter(m_property);
	}

	std::string getValueAsString() override {
		std::ostringstream ss;
		ss << std::scientific << m_property;
		return ss.str();
	}

	void setFloat(float newValue) override {
		setValue(newValue);
	}

	void setUint(unsigned int newValue) override {
		setValue(newValue);
	}

private:

	T m_property = T();

	FormatterFunc m_formatter;

	// Mutex for thread safety
	std::mutex mtx_property;
};