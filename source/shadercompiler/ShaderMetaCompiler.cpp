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

#include "ShaderMetaCompiler.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <windows.h>
#include <codecvt>
#include <unordered_map>
#include "ShaderFile.h"
#include <map>
#include <cassert>

std::map<std::string, std::unique_ptr<ShaderFile>> shaderFileBuffer;


std::string ShaderMetaCompiler::precompile(const std::string& fileName, const std::vector<ShaderMetaConstant>& constantsOverrideMap)
{
	std::string fileNameSrc = METASHADER_DIRECTORY;
	fileNameSrc.append(fileName);

	auto p = std::filesystem::path(fileNameSrc);
	if (!std::filesystem::exists(p)) {
		// NOTE: Please make sure that the working directory is set correctly! ($(OutputPath))
		std::cerr << "Shader-File " << p << " not found." << std::endl;
		assert(false);
	}

	if (!shaderFileBuffer.contains(fileName)) {
		shaderFileBuffer.insert({ fileName, std::make_unique<ShaderFile>(fileNameSrc) });
	}
	auto& file = shaderFileBuffer[fileName];
	file->reload();

	auto compiledShaderFile = file->compile(constantsOverrideMap);
	return compiledShaderFile;
}


