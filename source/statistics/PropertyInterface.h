#pragma once

#include <string>

class PropertyInterface {
protected:
	std::string m_name;
	std::string m_unit;

public:
	PropertyInterface(const std::string& name, const std::string& unit = "") : m_name(name), m_unit(unit) {};

	virtual std::string getFormatedString() = 0;

	virtual std::string getValueAsString() = 0;

	virtual void setFloat(float newValue) { throw std::exception("Setter not implemented for this object."); }
	virtual void setUint(unsigned int newValue) { throw std::exception("Setter not implemented for this object."); }

	const std::string& getName() { return m_name; }
};