
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