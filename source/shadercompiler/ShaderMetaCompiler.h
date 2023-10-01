#pragma once

#include <string>
#include <vector>

class ShaderMetaCompiler {

public:
	static std::string precompile(const std::string& fileName, const std::vector<std::pair<std::string, std::string>>& constantsOverrideMap);

private:
	static std::pair<std::string, std::string> splitFilename(const std::string& filePath);


};