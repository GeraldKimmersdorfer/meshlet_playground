#include "ShaderMetaCompiler.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <windows.h>
#include <codecvt>

// Convert an UTF8 string to a wide Unicode String
std::wstring to_wstring(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

bool spirv_compile(const std::string& src, const std::string& dest) {
	// Define the command and executable path
	LPCTSTR executablePath = L"assets/tools/glslangValidator.exe";
	std::wstring args;
	args += std::wstring(executablePath) + L" ";	// Note: executablePath has to be arg[0]
	args += L"--target-env vulkan1.2 ";
	args += L"-o " + to_wstring(dest) + L" ";
	args += to_wstring(src) + L" ";

	// Initialize the startup info and process info structures
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	ZeroMemory(&processInfo, sizeof(processInfo));

	// Create the process
	if (CreateProcess(
		executablePath,    // Application name
		&args[0],               // Command line
		NULL,               // Process handle not inheritable
		NULL,               // Thread handle not inheritable
		FALSE,              // Set handle inheritance to FALSE
		0,                  // No creation flags
		NULL,               // Use parent's environment block
		NULL,               // Use parent's starting directory
		&startupInfo,       // Pointer to STARTUPINFO structure
		&processInfo        // Pointer to PROCESS_INFORMATION structure
	)) {
		// Wait for the process to complete
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		// Close process and thread handles
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else {
		std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

std::string ShaderMetaCompiler::precompile(const std::string& fileName, const std::vector<std::pair<std::string, std::string>>& constantsOverrideMap)
{
	auto [name, extension] = splitFilename(fileName);

	std::string fileNameSrc = "assets/shaders/";
	fileNameSrc.append(fileName);

	std::string fileNameTmp = fileNameSrc;
	fileNameTmp.append(".tmp.");
	fileNameTmp.append(extension);

	std::stringstream fileNameOut;
	fileNameOut << "shaders/" << name;
	for (const auto& kv : constantsOverrideMap) {
		fileNameOut << "_" << kv.second;
	}
	fileNameOut << "." << extension << ".spv";

	// BUILD regex objects:
	std::vector<std::pair<std::regex, std::string>> regexList;
	for (const auto& kv : constantsOverrideMap) {
		std::stringstream pattern;
		pattern << "#define " << kv.first << " .*";
		std::stringstream replacement;
		replacement << "#define " << kv.first << " " << kv.second;
		regexList.push_back(std::make_pair(std::regex{ pattern.str() }, replacement.str()));
	}

	std::filesystem::remove(fileNameTmp);
	std::ofstream fileOutput(fileNameTmp);
	if (!fileOutput) {
		std::cerr << "Couldn't open file " << fileNameTmp << " as preprocessing output." << std::endl;
		return "";
	}
	std::ifstream fileInput(fileNameSrc);
	if (fileInput.is_open()) {
		std::string line;
		while (std::getline(fileInput, line)) {
			for (auto& rp : regexList) {
				line = std::regex_replace(line, rp.first, rp.second);
			}
			fileOutput << line << std::endl;
		}
		fileInput.close();
	}
	else {
		std::cerr << "Couldn't open file " << fileNameSrc << " for preprocessing." << std::endl;
		return "";
	}
	fileOutput.close();

	spirv_compile(fileNameTmp, fileNameOut.str());

	std::filesystem::remove(fileNameTmp);

	return fileNameOut.str();
}

std::pair<std::string, std::string> ShaderMetaCompiler::splitFilename(const std::string& filePath)
{
	size_t lastSlash = filePath.find_last_of("\\/");
	std::string fileNameWithExtension = filePath.substr(lastSlash + 1);
	size_t lastDot = fileNameWithExtension.find_last_of(".");
	if (lastDot != std::string::npos) {
		std::string fileName = fileNameWithExtension.substr(0, lastDot);
		std::string extension = fileNameWithExtension.substr(lastDot + 1);
		return std::make_pair(fileName, extension);
	}
	return std::make_pair(fileNameWithExtension, "");
}
