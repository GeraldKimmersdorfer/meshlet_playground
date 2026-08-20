/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "PropertyManager.h"
#include <cassert>
#include <stack>

void PropertyManager::add_property(std::shared_ptr<PropertyInterface> property)
{
	assert(get(property->getName()) == nullptr); // Property with same name already existing
	rootProperties.push_back(property);
	property->setManager(this);
	refreshPropertyMap();
}

std::shared_ptr<PropertyInterface> PropertyManager::getShared(const std::string& name)
{
	const auto it = m_propertys_map.find(name);
	if (it != m_propertys_map.end()) return it->second;
	return nullptr;
}

PropertyInterface* PropertyManager::get(const std::string& name)
{
	const auto it = m_propertys_map.find(name);
	if (it != m_propertys_map.end()) return it->second.get();
	return nullptr;
}

const std::vector<std::shared_ptr<PropertyInterface>>& PropertyManager::getAll() {
	static std::vector<std::shared_ptr<PropertyInterface>> all_properties;
	all_properties.clear();

	std::stack<std::shared_ptr<PropertyInterface>> stack;
	for (const auto& root : rootProperties) {
		stack.push(root);
	}

	while (!stack.empty()) {
		auto current = stack.top();
		stack.pop();
		all_properties.push_back(current);
		for (const auto& child : current->getChildren()) {
			stack.push(child);
		}
	}

	return all_properties;
}

const std::vector<std::shared_ptr<PropertyInterface>>& PropertyManager::getRootProperties() const {
	return rootProperties;
}

PropertyManager::PropertyManager()
{
}

void PropertyManager::refreshPropertyMap()
{
	m_propertys_map.clear();
	for (const auto& property : rootProperties) {
		addPropertyToMap(property);
	}
}

void PropertyManager::addPropertyToMap(std::shared_ptr<PropertyInterface> property)
{
	std::stack<std::shared_ptr<PropertyInterface>> stack;
	stack.push(property);
	while (!stack.empty()) {
		auto current = stack.top();
		stack.pop();
		m_propertys_map[current->getName()] = current;
		for (const auto& child : current->getChildren()) {
			stack.push(child);
		}
	}
}
