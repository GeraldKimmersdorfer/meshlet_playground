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

std::map<std::string, std::unique_ptr<ShaderFile>> shaderFileBuffer;


std::string ShaderMetaCompiler::precompile(const std::string& fileName, const std::vector<ShaderMetaConstant>& constantsOverrideMap)
{
	std::string fileNameSrc = METASHADER_DIRECTORY;
	fileNameSrc.append(fileName);

	if (!shaderFileBuffer.contains(fileName)) {
		shaderFileBuffer.insert({ fileName, std::make_unique<ShaderFile>(fileNameSrc) });
	}
	auto& file = shaderFileBuffer[fileName];
	file->reload();

	auto compiledShaderFile = file->compile(constantsOverrideMap);
	return compiledShaderFile;
}


