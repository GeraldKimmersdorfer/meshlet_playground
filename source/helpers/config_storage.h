/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <string>
#include <map>
#include "../shared_structs.h" // Include the header where config_data is defined

 // Function declarations with default filename
void saveConfigurationsToFile(const std::map<std::string, config_data>& configurations, const std::string& filename = "config_storage.dat");
void readConfigurationsFromFile(std::map<std::string, config_data>& configurations, const std::string& filename = "config_storage.dat");
