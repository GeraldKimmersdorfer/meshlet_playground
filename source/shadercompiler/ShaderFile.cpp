#include "ShaderFile.h"

#include <fstream>
#include <regex>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <Windows.h>
#include <cassert>
#include "ShaderMetaCompiler.h"

const std::regex rConstantsSearch(R"(\n\s*(#define\s+MCC_([\w\d_]+)\s+([\w\d]+)))");

// Convert an UTF8 string to a wide Unicode String
std::wstring to_wstring(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::pair<std::string, std::string> splitFilename(const std::string& filePath)
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

void replaceAll(std::string& inputStr, const std::string& searchStr, const std::string& replaceStr, int maxReplacements = -1) {
	size_t pos = inputStr.find(searchStr);
	int i = 0;
	while (pos != std::string::npos) {
		inputStr.replace(pos, searchStr.length(), replaceStr);
		if (++i == maxReplacements) break;
		pos = inputStr.find(searchStr, pos + replaceStr.length());
	}
}

void spirv_compile(const std::filesystem::path& src, const std::filesystem::path& dest) {
	// Define the command and executable path
	LPCTSTR executablePath = SPIRV_COMPILER_PATH;
	std::wstring args;
	args += std::wstring(executablePath) + L" ";	// Note: executablePath has to be arg[0]
	args += SPIRV_ADDITIONAL_FLAGS;
	args += L"-o " + dest.wstring() + L" ";
	args += src.wstring() + L" ";

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
		DWORD result = WaitForSingleObject(processInfo.hProcess, SPIRV_COMPILER_MAX_TIMEOUT);

		// Close process and thread handles
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		if (result == WAIT_TIMEOUT) throw std::runtime_error("Spirv-Compilation of " + src.string() + " suceeded allowed execution time!");
		else if (result != WAIT_OBJECT_0) throw std::runtime_error("WaitForSingleObject failed for shader " + src.string()); // + ": " + ((std::wstring*)(void*)GetLastError()));
		if (!std::filesystem::exists(dest)) throw std::runtime_error("Shader Compilation failed. Check log for details.");
	}
	else throw std::runtime_error("CreateProcess failed");// << GetLastError() << std::endl)
}

ShaderFile::ShaderFile(const std::string& path)
	:mPath(path)
{
}

void ShaderFile::reload()
{
	auto p = std::filesystem::path(mPath);
	std::filesystem::file_time_type ftime = std::filesystem::last_write_time(p);
	if (mBaseCode.empty() || ftime > mLastModifiedTime) {
		mValidShaderVariants.clear();

		// LOAD BASE CODE:
		std::ifstream t(mPath);
		if (t.is_open()) {
			std::stringstream buffer;
			buffer << t.rdbuf();
			mBaseCode = buffer.str();

			// GET ALL POSSIBLE CONSTANTS:
			extractConstants();

		}
		else {
			std::cerr << "Couldnt open file " << mPath << " for reading." << std::endl;
		}

		mLastModifiedTime = ftime;
	}
}

std::string getUniqueFileIdentifier(const std::map<std::string, std::string> replacementValues) {
	std::stringstream string_identifier_builder;
	int i = 0;
	for (auto& rv : replacementValues) {
		string_identifier_builder << (i > 0 ? "_" : "") << rv.second;
		i++;
	}
	std::string string_identifier = string_identifier_builder.str();

	if (string_identifier.length() > MAX_UFID_LENGTH) {
		auto hash = std::hash<std::string>{}(string_identifier);
		string_identifier = std::to_string(hash);
	}
	return string_identifier;
}



std::string ShaderFile::compile(const std::vector<ShaderMetaConstant>& constants)
{
	std::map<std::string, std::string> replacementValues;
	for (auto& smcs : mAllConstants) {
		bool smcs_found = false;
		for (auto& smc : constants) {
			if (smc.mName == smcs.mName) {
				replacementValues[smcs.mName] = smc.mNewValue;
				smcs_found = true;
				break;
			}
		}
		if (!smcs_found) {
			std::cout << "Constant " << smcs.mName << " is not set for current shader compilation. Default value " << smcs.mDefaultValue << " will be used." << std::endl;
			replacementValues[smcs.mName] = smcs.mDefaultValue;
		}
	}

	bool constantNotFound = false;
	for (auto& smc : constants) {
		if (!replacementValues.contains(smc.mName)) {
			std::cerr << "Given Constant " << smc.mName << " was not found in shader file " << mPath << std::endl;
			constantNotFound = true;
		}
	}
	if (constantNotFound) return "";

	std::string ufid = getUniqueFileIdentifier(replacementValues);
	
	auto spvFile = getSpvPathName(ufid);
	if (mValidShaderVariants.contains(ufid) && std::filesystem::exists(spvFile)) {
		return spvFile.string();	// use already compiled version
	}
	else {
		// Create temporary file
		auto tmpFile = getTemporaryPathName(ufid);
		std::string mAlternatedCode = mBaseCode;
		for (auto& smcs : mAllConstants) {
			if (replacementValues.contains(smcs.mName) && replacementValues[smcs.mName] != smcs.mDefaultValue) {
				std::string newWholeMatch = smcs.mWholeMatch;
				replaceAll(newWholeMatch, " " + smcs.mDefaultValue, " " + replacementValues[smcs.mName]);
				replaceAll(mAlternatedCode, smcs.mWholeMatch, newWholeMatch, 1);
			}
		}
		std::ofstream outputStream(tmpFile);
		if (!outputStream.is_open()) {
			std::cerr << "Couldnt open " << tmpFile << " for writing." << std::endl;
			return "";
		}
		outputStream << mAlternatedCode;
		outputStream.close();

		std::filesystem::remove(spvFile);	// remove old file

		// Run glslangValidator
		spirv_compile(tmpFile, spvFile);

		// Delete temporary file
		std::filesystem::remove(tmpFile);
		// and save in valid array:
		mValidShaderVariants.insert(ufid);
		return spvFile.string();
	}
}

void ShaderFile::extractConstants()
{
	mAllConstants.clear();

    // Create a regex iterator
    std::sregex_iterator it(mBaseCode.begin(), mBaseCode.end(), rConstantsSearch);
    std::sregex_iterator end;

    // Iterate over matches and their groups
    while (it != end) {
        std::smatch match = *it;

		mAllConstants.push_back({
			match[2].str(),
			match[3].str(),
			match[1].str()
			});

        ++it;
    }
}

std::filesystem::path ShaderFile::getTemporaryPathName(const std::string& ufid)
{
	std::wstringstream filename;
	filename << mPath.stem().c_str() << L"_" << to_wstring(ufid) << L".tmp" << mPath.extension().c_str();
	std::filesystem::path tmp(METASHADER_DIRECTORY);
	tmp /= filename.str();
	return tmp;
}

std::filesystem::path ShaderFile::getSpvPathName(const std::string& ufid)
{
	std::wstringstream filename;
	filename << mPath.stem().c_str() << L"_" << to_wstring(ufid) << mPath.extension().c_str() << L".spv";
	std::filesystem::path tmp(SPIRV_OUTPUT_DIRECTORY);
	tmp /= filename.str();
	return tmp;
}
