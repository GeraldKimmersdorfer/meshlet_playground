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
#include <vector>
#include <set>
#include <filesystem>
#include <chrono>
#include "ShaderMetaConstant.h"

class ShaderFile {

	struct ShaderMetaConstantStorage {
		std::string mName;
		std::string mDefaultValue;
		std::string mWholeMatch;
	};

public:

	ShaderFile(const std::string& path);

	// If the file signature changes we reload the content of the original
	// shader file and clear all valid shader files
	void reload();

	// Compiles the shader by first replacing the constants and returns the
	// path to the spirv file. If already compiled and basefile didn't change
	// it will not recompile!
	std::string compile(const std::vector<ShaderMetaConstant>& constants);

private:
	std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> mDependencies; // A vector with all include files and their respective last change time.
	std::filesystem::path mPath;

	std::string mBaseCode = "";

	std::vector<ShaderMetaConstantStorage> mAllConstants;
	std::set<std::string> mValidShaderVariants;

	void extractConstants();

	/// <summary>
	/// Starting with the given file this function searches for all includes and adds them to the dependency list
	/// </summary>
	void loadDependencies(const std::filesystem::path& path);
	std::filesystem::path getTemporaryPathName(const std::string& ufid);
	std::filesystem::path getSpvPathName(const std::string& ufid);


};