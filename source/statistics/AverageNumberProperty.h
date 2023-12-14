#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <queue>
#include <type_traits>
#include "PropertyInterface.h"


template <typename T, typename AvgType = T>
class AverageNumberProperty :public PropertyInterface {

	static_assert(
		std::is_same<T, float>::value ||
		std::is_same<T, unsigned int>::value,
		"AverageNumberProperty only supports float and unsigned int"
		);

public:
	AverageNumberProperty(const std::string& name, const std::string& unit = "", const unsigned int max_queue_length = 60) 
		:PropertyInterface(name, unit), m_max_queue_length(max_queue_length) 
	{};

	void setValue(const T& newValue) {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		m_property = newValue;
		
		m_total += m_property;
		m_avgelements.push(m_property);
		if (m_avgelements.size() > m_max_queue_length) {
			m_total -= m_avgelements.front();
			m_avgelements.pop();
		}
		m_avg = m_total / m_avgelements.size();
	}

	T getValue() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		return m_property;
	}

	AvgType getAverage() {
		return m_avg;
	}

	std::string getValueAsString() {
		return std::to_string(getValue());
	}

	std::string getFormatedString() {
		std::stringstream ss;
		ss << std::to_string(getValue()) << m_unit << " (" << std::to_string(m_avg) << m_unit << ")";
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
	AvgType m_avg = AvgType();

	std::queue<T> m_avgelements;
	unsigned int m_max_queue_length;
	AvgType m_total = AvgType();
	
	// Mutex for thread safety
	std::mutex mtx_property;
};