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