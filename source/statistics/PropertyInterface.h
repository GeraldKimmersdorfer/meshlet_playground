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
#include <vector>
#include <memory>

class PropertyManager;  // Forward declaration

class PropertyInterface {
protected:
	std::string m_name;
	std::string m_formatted_name;
	std::vector<std::shared_ptr<PropertyInterface>> m_children;
	PropertyManager* m_manager;

	void generateFormattedName();

public:
	PropertyInterface(const std::string& name);

	virtual std::string getValueAsFormattedString() = 0;
	virtual std::string getValueAsString() = 0;

	virtual void setFloat(float newValue) { throw std::exception("Float-Setter not implemented for this object."); }
	virtual void setUint(unsigned int newValue) { throw std::exception("Uint-Setter not implemented for this object."); }
	virtual void setString(const std::string& newValue) { throw std::exception("String-Setter not implemented for this object."); }

	const std::string& getName() const { return m_name; }
	const std::string& getFormattedName() const { return m_formatted_name; }

	const std::vector<std::shared_ptr<PropertyInterface>>& getChildren() const { return m_children; }

	void addChild(std::shared_ptr<PropertyInterface> child);

	void setManager(PropertyManager* manager);
};

class PropertyGroup : public PropertyInterface {
public:
	PropertyGroup(const std::string& name) : PropertyInterface(name) {}

	std::string getValueAsFormattedString() override {
		return "";
	}

	std::string getValueAsString() override {
		return "";
	}
};
