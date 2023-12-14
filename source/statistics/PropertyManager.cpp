#include "PropertyManager.h"

#include <cassert>

void PropertyManager::add_property(std::shared_ptr<PropertyInterface> property)
{
	assert(get(property->getName()) == nullptr); // Property with same name already existing
	m_propertys_in_order.push_back(property);
	m_propertys_map[property->getName()] = property;
}

std::shared_ptr<PropertyInterface> PropertyManager::get(const std::string& name)
{
	auto it = m_propertys_map.find(name);
	if (it != m_propertys_map.end()) return it->second;
	return nullptr;
}

PropertyManager::PropertyManager()
{

}
