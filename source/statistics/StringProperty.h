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
