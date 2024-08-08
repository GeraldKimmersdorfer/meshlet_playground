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

#define CSV_VIEWER_EXE L"assets\\tools\\CSVFileView.exe"
#define TEMP_FILE_FOLDER L"temp"
#define CSV_SEPARATOR ','

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <map>
#include <unordered_map>

namespace std {
	inline string to_string(const glm::u16vec4& _Val) {
		return to_string(_Val.x) + CSV_SEPARATOR + to_string(_Val.y) + CSV_SEPARATOR + to_string(_Val.z) + CSV_SEPARATOR + to_string(_Val.w);
	}
	template <typename K, typename T>
	inline string to_string(const std::pair<K, T>& _Val) {
		return to_string(_Val.first) + CSV_SEPARATOR + to_string(_Val.second);
	}
}

void writeAndOpenCSV(std::vector<std::vector<std::string>> data);

template <typename T>
void writeAndOpenCSV(const std::vector<T>& container) {
	std::vector<std::vector<std::string>> data;
	for (const auto& element : container) {
		data.push_back({ std::to_string(element) });
	}
	writeAndOpenCSV(data);
}

template <typename K, typename T>
void writeAndOpenCSV(const std::map<K, T>& container) {
	std::vector<std::vector<std::string>> data;
	for (const auto& element : container) {
		data.push_back({ std::to_string(element.first), std::to_string(element.second) });
	}
	writeAndOpenCSV(data);
}

template <typename K, typename T>
void writeAndOpenCSV(const std::unordered_map<K, T>& container) {
	std::vector<std::vector<std::string>> data;
	for (const auto& element : container) {
		data.push_back({ std::to_string(element.first), std::to_string(element.second) });
	}
	writeAndOpenCSV(data);
}
