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
#include <memory>
#include <vector>
#include <map>
#include "PropertyInterface.h"

class PropertyManager
{
public:
	// adds the property
	void add_property(std::shared_ptr<PropertyInterface> property);

	std::shared_ptr<PropertyInterface> getShared(const std::string& name);

	PropertyInterface* get(const std::string& name);

	const std::vector<std::shared_ptr<PropertyInterface>>& getAll();

	// Returns a const reference to the root properties
	const std::vector<std::shared_ptr<PropertyInterface>>& getRootProperties() const;

	PropertyManager();

	void refreshPropertyMap();

private:
	void addPropertyToMap(std::shared_ptr<PropertyInterface> property);

	// Contains the properties as list for the correct order
	std::vector<std::shared_ptr<PropertyInterface>> rootProperties;
	// Contains the properties as map for fast access by name
	std::map<std::string, std::shared_ptr<PropertyInterface>> m_propertys_map;
};
