#include "SharedData.h"

#include <imgui.h>

//#include <vk_convenience_functions.hpp>
//#include <material_image_helpers.hpp>

void SharedData::attachSharedPipelineConfiguration(avk::graphics_pipeline_config* pipeConfig, std::vector<avk::binding_data>* staticDescriptors)
{
	pipeConfig->mFrontFaceWindingOrder = avk::cfg::front_face::define_front_faces_to_be_counter_clockwise();
	pipeConfig->mCullingMode = mCullingMode;
	if (mDrawingMode == avk::cfg::polygon_drawing_mode::fill) {
		pipeConfig->mPolygonDrawingModeAndConfig = avk::cfg::polygon_drawing::config_for_filling();
	}
	else if (mDrawingMode == avk::cfg::polygon_drawing_mode::line) {
		pipeConfig->mPolygonDrawingModeAndConfig = avk::cfg::polygon_drawing::config_for_lines();
	}
	else {
		pipeConfig->mPolygonDrawingModeAndConfig = avk::cfg::polygon_drawing::config_for_points();
	}
	pipeConfig->mViewportDepthConfig.push_back(avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)));
	auto rp = avk::context().create_renderpass({
		avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
		avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
		}, avk::context().main_window()->renderpass_reference().subpass_dependencies());
	pipeConfig->mRenderPassSubpass = std::move(std::make_tuple(std::move(rp), 0));
	// DYNAMIC BINDING DATA
	pipeConfig->mResourceBindings.push_back(std::move(avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal))));
	pipeConfig->mResourceBindings.push_back(std::move(avk::descriptor_binding(0, 1, mViewProjBuffers[0])));
	pipeConfig->mResourceBindings.push_back(std::move(avk::descriptor_binding(2, 0, mBoneTransformBuffers[0])));
	// STATIC BINDING DATA (HAVE TO BE ADDED BY CALLING ENTITY)
	staticDescriptors->push_back(std::move(avk::descriptor_binding(0, 2, mConfigurationBuffer)));
	staticDescriptors->push_back(std::move(avk::descriptor_binding(1, 0, mMaterialsBuffer)));
	staticDescriptors->push_back(std::move(avk::descriptor_binding(4, 1, mMeshesBuffer)));
}

std::vector<avk::binding_data> SharedData::getDynamicDescriptorBindings(int64_t inFlightIndex)
{
	return {
		std::move(avk::descriptor_binding(0, 1, mViewProjBuffers[inFlightIndex])),
		std::move(avk::descriptor_binding(2, 0, mBoneTransformBuffers[inFlightIndex]))
	};
}

void SharedData::hudSharedConfiguration(bool& config_has_changed)
{
	ImGui::Combo("Rasterizer Culling", (int*)(void*)&mCullingMode, "Disabled\0Cull Front Faces\0Cull Back Faces\0Cull Both\0");
	ImGui::Combo("Polygon Draw Mode", (int*)(void*)&mDrawingMode, "Solid\0Wireframe\0Points\0");
}
