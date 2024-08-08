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

enum MCC_MESHLET_EXTENSION { _EXT, _NV };
enum MCC_MESHLET_TYPE { _NATIVE, _REDIR };
enum MCC_VERTEX_GATHER_TYPE { _PUSH, _PULL };

template <typename T>
std::string MCC_to_string(T value) {
	if constexpr (std::is_same_v<T, MCC_MESHLET_EXTENSION>) {
		switch (value) {
		case _EXT: return "_EXT";
		case _NV: return "_NV";
		}
	}
	else if constexpr (std::is_same_v<T, MCC_MESHLET_TYPE>) {
		switch (value) {
		case _NATIVE: return "_NATIVE";
		case _REDIR: return "_REDIR";
		}
	}
	else if constexpr (std::is_same_v<T, MCC_VERTEX_GATHER_TYPE>) {
		switch (value) {
		case _PUSH: return "_PUSH";
		case _PULL: return "_PULL";
		}
	}
	assert(true);	// Please define all MCCs here
	return "Undefined";
}