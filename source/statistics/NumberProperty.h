#pragma once

#include <string>
#include <mutex>
#include <type_traits>
#include "PropertyInterface.h"

template <typename T, typename AvgType = std::nullptr_t>
class NumberProperty :public PropertyInterface {

	static_assert(
		std::is_same<T, float>::value ||
		std::is_same<T, unsigned int>::value,
		"NumberProperty only supports float and unsigned int"
		);

public:
	NumberProperty(const std::string& name, const std::string& unit = "") : PropertyInterface(name, unit) {};

	void setValue(const T& newValue) {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		m_property = newValue;
	}

	T getValue() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		return m_property;
	}

	std::string getValueAsString() {
		return std::to_string(getValue());
	}

	std::string getFormatedString() {
		return std::to_string(getValue()) + m_unit;
	}

	void setFloat(float newValue) override {
		setValue(newValue);
	}

	void setUint(unsigned int newValue) override {
		setValue(newValue);
	}

private:
	T m_property = T();

	// Mutex for thread safety
	std::mutex mtx_property;
};