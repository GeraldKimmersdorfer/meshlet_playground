
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

}