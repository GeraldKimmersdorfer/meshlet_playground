#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Property.h"

class PropertyManager
{

public:
	// adds the property
	void add_property(std::shared_ptr<PropertyInterface> property);

	std::shared_ptr<PropertyInterface> get(const std::string& name);

	const std::vector<std::shared_ptr<PropertyInterface>>& getAll() {
		return m_propertys_in_order;
	}

	PropertyManager();

private:
	// Contains the properties as list for the correct order
	std::vector<std::shared_ptr<PropertyInterface>> m_propertys_in_order;
	// Contains the properties as map for fast access by name
	std::map<std::string, std::shared_ptr<PropertyInterface>> m_propertys_map;

};

