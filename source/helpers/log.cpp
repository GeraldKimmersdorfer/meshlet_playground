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

#include "log.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>

 // Define ASCII color codes
const char* ASCII_COLOR_RESET = "\033[0m";
const char* ASCII_COLOR_CYAN = "\033[36m";
const char* ASCII_COLOR_BLUE = "\033[34m";
const char* ASCII_COLOR_YELLOW = "\033[33m";
const char* ASCII_COLOR_RED = "\033[31m";
const char* ASCII_COLOR_GRAY = "\033[90m";

// Custom log format function
void custom_log_format(void* user_data, const loguru::Message& message) {
	const char* typeStr = nullptr;
	const char* typeColorCode = nullptr;
	const char* messageColorCode = nullptr;
	std::ostream* stream = nullptr;

	switch (message.verbosity) {
	case loguru::Verbosity_INFO:
		typeStr = "Info    ";
		typeColorCode = ASCII_COLOR_CYAN;
		messageColorCode = ASCII_COLOR_GRAY;
		stream = &std::cout;
		break;
	case loguru::Verbosity_WARNING:
		typeStr = "Warning ";
		typeColorCode = ASCII_COLOR_YELLOW;
		stream = &std::cout;
		break;
	case loguru::Verbosity_ERROR:
		typeStr = "Critical";
		typeColorCode = ASCII_COLOR_RED;
		stream = &std::cerr;
		break;
	case loguru::Verbosity_FATAL:
		typeStr = "Fatal   ";
		typeColorCode = ASCII_COLOR_RED;
		stream = &std::cerr;
		break;
	default:
		typeStr = "Unknown ";
		typeColorCode = ASCII_COLOR_GRAY;
		stream = &std::cerr;
		break;
	}

	// Get current time
	auto t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);

	// Extract relative file path
	const char* filename = message.filename;
	const char* last_slash = strrchr(filename, '/');
	if (!last_slash) {
		last_slash = strrchr(filename, '\\');
	}
	if (last_slash) {
		filename = last_slash + 1;
	}

	// Format the log message
	(*stream) << typeColorCode
		<< std::put_time(&tm, "%H:%M:%S") << " | "
		<< typeColorCode << typeStr << " | "
		<< std::setw(40) << std::left << filename << " | "
		<< std::setw(5) << message.line << " | " << (messageColorCode ? messageColorCode : "")
		<< message.message << ASCII_COLOR_RESET << std::endl;

	if (message.verbosity == loguru::Verbosity_FATAL) {
		abort();
	}

	stream->flush();
}

void printAsciiLogo() {
	std::cout << R"(   __  __        _    _     _     ___ _                                     _ 
  |  \/  |___ __| |_ | |___| |_  | _ \ |__ _ _  _ __ _ _ _ ___ _  _ _ _  __| |
  | |\/| / -_|_-< ' \| / -_)  _| |  _/ / _` | || / _` | '_/ _ \ || | ' \/ _` |
  |_|  |_\___/__/_||_|_\___|\__| |_| |_\__,_|\_, \__, |_| \___/\_,_|_||_\__,_|
__________________________________________   |__/|___/ _________________________

)";
}

void initLogging(int argc, char* argv[])
{
	printAsciiLogo();
	// Set the maximum verbosity for console output
	loguru::g_stderr_verbosity = loguru::Verbosity_OFF; // Disable default stderr logging

	// Add custom log callback
	loguru::add_callback("custom_formatter", custom_log_format, nullptr, loguru::Verbosity_MAX);

	// Initialize Loguru
	loguru::init(argc, argv);
}
