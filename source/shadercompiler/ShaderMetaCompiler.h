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
#include "ShaderMetaConstant.h"

#define METASHADER_DIRECTORY "assets/shaders/"
#define MAX_UFID_LENGTH 30
#define SPIRV_OUTPUT_DIRECTORY "shaders/"
#define SPIRV_COMPILER_PATH L"assets/tools/glslangValidator.exe"
#define SPIRV_ADDITIONAL_FLAGS L"--target-env vulkan1.2 "
#define SPIRV_COMPILER_MAX_TIMEOUT 1000

class ShaderMetaCompiler {

public:
	static std::string precompile(const std::string& fileName, const std::vector<ShaderMetaConstant>& constantsOverrideMap);

private:

};