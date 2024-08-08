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
#include <sstream>
#include <mutex>
#include <queue>
#include <type_traits>
#include "PropertyInterface.h"
#include "propertyFormatters.h"


template <typename T>
class AverageNumberProperty :public PropertyInterface {

	using FormatterFunc = std::function<std::string(const T&)>;

	static_assert(
		std::is_same<T, float>::value ||
		std::is_same<T, unsigned int>::value,
		"AverageNumberProperty only supports float and unsigned int"
		);

public:
	AverageNumberProperty(const std::string& name,
		const unsigned int max_queue_length = 60,
		FormatterFunc formatter = genericSiFormatter<T>)
		: PropertyInterface(name), m_formatter(formatter)
	{
		m_elements.resize(max_queue_length, T());
	}


	void setValue(const T& newValue) {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block

		const auto& id = m_element_nextid;

		// Remove the old value from the total and total square
		m_total -= m_elements[id];
		m_total_square -= m_elements[id] * m_elements[id];

		// Add the new value to the total and total square
		m_elements[id] = newValue;
		m_total += newValue;
		m_total_square += newValue * newValue;
		m_current_valid_elements = std::min(m_current_valid_elements + 1, m_elements.size());

		m_avg = m_total / m_current_valid_elements;

		// Calculate the variance
		T variance = (m_total_square / m_current_valid_elements) - (m_avg * m_avg);
		m_sdev = std::sqrt(variance);

		m_element_nextid = nextElementId();
		m_lastvalue = newValue;
	}


	T getValue() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		return m_lastvalue;
	}

	T getAverage() {
		return m_avg;
	}

	std::string getValueAsFormattedString() override {
		std::stringstream ss;
		ss << m_formatter(m_avg) << " +- " << m_formatter(m_sdev) << " [" << m_current_valid_elements << "]";
		return ss.str();
	}

	std::string getValueAsString() override {
		std::ostringstream ss;
		ss << std::scientific << m_avg;
		return ss.str();
	}


	void setFloat(float newValue) override {
		setValue(newValue);
	}

	void setUint(unsigned int newValue) override {
		setValue(newValue);
	}

	void reset() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		m_total = T();
		m_total_square = T();
		m_current_valid_elements = 0;
		m_lastvalue = T();
		m_avg = T();
		m_sdev = T();
		m_element_nextid = 0;
		auto old_size = m_elements.size();
		m_elements.clear();
		m_elements.resize(old_size, T());
	}

private:
	T m_lastvalue = T();
	T m_avg = T();
	T m_sdev = T();

	std::vector<T> m_elements;
	size_t m_element_nextid = 0;
	size_t m_current_valid_elements = 0;

	T m_total = T();
	T m_total_square = T();

	FormatterFunc m_formatter;

	inline const size_t nextElementId() {
		return (m_element_nextid + 1) % m_elements.size();
	}

	inline const size_t prevElementId() {
		return (m_element_nextid + m_elements.size() - 1) % m_elements.size();
	}

	// Mutex for thread safety
	std::mutex mtx_property;
};