#include "csv.h"

#include <fstream>
#include <Windows.h>
#include <filesystem>

namespace fs = std::filesystem;

uint32_t file_counter = 0;

void createDirectoryIfNotExists(const fs::path& directoryPath) {
    if (!fs::exists(directoryPath)) {
        fs::create_directory(directoryPath);
    }
    else if (!fs::is_directory(directoryPath)) {
        throw std::runtime_error("A file with the same name as temporary directory exists");
    }
}

std::filesystem::path generateFilePath() {
    fs::path tempfile = TEMP_FILE_FOLDER;
    createDirectoryIfNotExists(tempfile);
    return tempfile / (std::to_string(file_counter++) + ".csv");
}

void writeAndOpenCSV(std::vector<std::vector<std::string>> data)
{
    auto fileName = generateFilePath();
	std::ofstream outputFile(fileName);
    if (!outputFile.is_open()) throw std::runtime_error("Failed to open csv for writing!");

    for (int row = 0; row < data.size(); row++) {
        if (row > 0) outputFile << std::endl;
        for (int column = 0; column < data[row].size(); column++) {
            if (column > 0) outputFile << CSV_SEPARATOR;
            outputFile << data[row][column];
        }
    }
    outputFile.close();

    HINSTANCE result = ShellExecute(NULL, L"open", CSV_VIEWER_EXE, fileName.wstring().c_str(), NULL, SW_SHOWNORMAL);
    if (!(reinterpret_cast<int>(result) > 32)) throw std::runtime_error("Failed to open the file using shell execute.");
}
