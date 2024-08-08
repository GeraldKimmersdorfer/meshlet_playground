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
