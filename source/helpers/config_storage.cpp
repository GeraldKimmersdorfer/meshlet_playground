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

#include "config_storage.h"
#include <fstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

 // Function to save configurations to a file
void saveConfigurationsToFile(const std::map<std::string, config_data>& configurations, const std::string& filename) {
	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file for writing: " + filename);
	}

	// Write the number of configurations
	size_t numConfigurations = configurations.size();
	file.write(reinterpret_cast<const char*>(&numConfigurations), sizeof(size_t));

	for (const auto& config : configurations) {
		// Write the configuration name length and name
		size_t nameLength = config.first.size();
		file.write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
		file.write(config.first.data(), nameLength);

		// Write the entire config_data struct
		file.write(reinterpret_cast<const char*>(&config.second), sizeof(config_data));
	}

	file.close();
}

// Function to read configurations from a file
void readConfigurationsFromFile(std::map<std::string, config_data>& configurations, const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		return; // ignore if not existant
		//throw std::runtime_error("Could not open file for reading: " + filename);
	}

	// Read the number of configurations
	size_t numConfigurations;
	file.read(reinterpret_cast<char*>(&numConfigurations), sizeof(size_t));

	configurations.clear();

	for (size_t i = 0; i < numConfigurations; ++i) {
		// Read the configuration name length and name
		size_t nameLength;
		file.read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
		std::string name(nameLength, '\0');
		file.read(&name[0], nameLength);

		// Read the entire config_data struct
		config_data config;
		file.read(reinterpret_cast<char*>(&config), sizeof(config_data));

		// Insert into the map
		configurations[name] = config;
	}

	file.close();
}
