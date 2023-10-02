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