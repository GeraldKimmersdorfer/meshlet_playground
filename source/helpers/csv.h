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
void writeAndOpenCSV(const std::map<K,T>& container) {
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
