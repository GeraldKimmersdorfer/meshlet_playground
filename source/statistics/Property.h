#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <thread>
#include <mutex>
#include <queue>

#include <type_traits>

class PropertyInterface {
protected:
	std::string m_name;

public:
	PropertyInterface(const std::string& name) : m_name(name) {};

	virtual std::string getValueAsString() = 0;
	virtual const std::string& getName() = 0;

	const std::string& getName() { return m_name; }
};

template<typename T>
std::string typeToString(const T& data) { return std::to_string(data); }
//std::string typeToString(const int& data) {		return std::to_string(data); }
//std::string typeToString(const float& data) {	return replace(std::to_string(data), '.', ',');	}
//std::string typeToString(const double& data) {  return replace(std::to_string(data), '.', ','); }


template <typename T, typename AvgType = std::nullptr_t>
class NumberProperty :public PropertyInterface {

	static_assert(
		std::is_same<T, int>::value ||
		std::is_same<T, double>::value ||
		std::is_same<T, float>::value ||
		std::is_same<T, uint32_t>::value ||
		std::is_same<T, uint64_t>::value,
		"NumberProperty only supports int, double, float, uint32_t, and uint64_t"
		);

public:
	NumberProperty(const std::string& name, const T& initialValue) : PropertyInterface(name), m_property(initialValue) {};

	void setValue(const T& newValue) {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		m_property = newValue;
		if constexpr (!std::is_same<AvgType, std::nullptr_t>::value) {
			m_total += m_property;
			m_avgelements.push(m_property);
			if (m_avgelements.size() > 60) {
				m_total -= m_avgelements.front();
				m_avgelements.pop();
			}
		}

	}

	T getValue() {
		std::lock_guard<std::mutex> lock(mtx_property); // Keeps the lock until the end of the block
		return m_property;
	}

	double getAvgValue() {
		m_total / (double)m_avgelements.size();
	}

	std::string getValueAsString() {
		return typeToString(getValue());
	}

	

private:
	T m_property;

	std::queue<T> m_avgelements;
	AvgType m_total = AvgType();

	// Mutex for thread safety
	std::mutex mtx_property;
};