#pragma once

#include <string>
#include "PropertyInterface.h"

class StringProperty : public PropertyInterface {
public:
	StringProperty(const std::string& name, const std::string& value = "")
		: PropertyInterface(name), m_value(value) {
		generateFormattedName();
	}

	std::string getValueAsFormattedString() override {
		return m_value;
	}

	std::string getValueAsString() override {
		return m_value;
	}

	void setString(const std::string& newValue) override {
		m_value = newValue;
	}

	std::string getString() const {
		return m_value;
	}

private:
	std::string m_value;
};
