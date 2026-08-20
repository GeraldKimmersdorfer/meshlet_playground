/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "PropertyInterface.h"
#include "PropertyManager.h"
#include <cctype>

PropertyInterface::PropertyInterface(const std::string& name) : m_name(name), m_manager(nullptr) {
	generateFormattedName();
}

void PropertyInterface::generateFormattedName() {
	m_formatted_name = m_name;
	for (size_t i = 0; i < m_formatted_name.size(); ++i) {
		if (m_formatted_name[i] == '_') {
			m_formatted_name[i] = ' ';
		}
	}
	if (!m_formatted_name.empty()) {
		m_formatted_name[0] = std::toupper(m_formatted_name[0]);
	}
}

void PropertyInterface::addChild(std::shared_ptr<PropertyInterface> child) {
	m_children.push_back(child);
	if (m_manager) {
		child->setManager(m_manager);
		m_manager->refreshPropertyMap();
	}
}

void PropertyInterface::setManager(PropertyManager* manager) {
	m_manager = manager;
	for (auto& child : m_children) {
		child->setManager(manager);
	}
}
