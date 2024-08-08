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

#include <avk/avk.hpp>

namespace avk {

	template <typename... Ts>
	graphics_pipeline_config create_graphics_pipeline_config(Ts... args)
	{
		// 1. GATHER CONFIG
		std::vector<avk::attachment> renderPassAttachments;
		std::function<void(graphics_pipeline_t&)> alterConfigFunction;
		graphics_pipeline_config config;
		add_config(config, renderPassAttachments, alterConfigFunction, std::move(args)...);
		return config;
	}

	template<typename T>
	std::vector<T> mergeVectors(const std::vector<T>& first) {
		return first;
	}

	template<typename T, typename... Args>
	std::vector<T> mergeVectors(const std::vector<T>& first, const Args&... rest) {
		std::vector<T> merged = first;
		const std::vector<T> others = mergeVectors(rest...);
		merged.insert(merged.end(), others.begin(), others.end());
		return merged;
	}

}