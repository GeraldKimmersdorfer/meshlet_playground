#pragma once

#include "imgui.h"
#include "imgui_manager.hpp"
#include "invokee.hpp"

#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include "vk_convenience_functions.hpp"
#include "../meshoptimizer/src/meshoptimizer.h"
#include "../ImGuiFileDialog/ImGuiFileDialog.h"
#include <glm/glm.hpp>
#include <avk/avk.hpp>
#include "configure_and_compose.hpp"
#include "material_image_helpers.hpp"
#include <auto_vk_toolkit.hpp>
#include <meshlet_helpers.hpp>
#include "shared_structs.h"

struct animation_data {
	std::string mName;
	double mDurationTicks;
	double mDurationSeconds;
	unsigned int mChannelCount;
	double mTicksPerSecond;
	avk::animation_clip_data mClip;
	avk::animation mAnimation;
};


/// <summary>
/// All pipelines will have access to this data
/// </summary>
class SharedData
{
public:

	config_data mConfig;

	avk::model mModel;

	std::vector<mesh_data> mMeshData;
	std::vector<vertex_data> mVertexData;
	std::vector<uint32_t> mIndices;

	std::vector<avk::buffer> mViewProjBuffers;
	std::vector<avk::buffer> mBoneTransformBuffers;
	avk::buffer mMaterialsBuffer;
	avk::buffer mMeshesBuffer;
	avk::buffer mVertexBuffer;
	avk::buffer mIndexBuffer;
	avk::buffer mConfigurationBuffer;

	std::vector<avk::image_sampler> mImageSamplers;

	// VK PROPERTIES:
	vk::PhysicalDeviceProperties2 mProps2;
	vk::PhysicalDeviceMeshShaderPropertiesEXT mPropsMeshShader;
	vk::PhysicalDeviceMeshShaderPropertiesNV mPropsMeshShaderNV;
	vk::PhysicalDeviceSubgroupProperties mPropsSubgroup;
	bool mNvPipelineSupport = false;

	// VK FEATURES:
	vk::PhysicalDeviceFeatures2 mFeatures2;
	vk::PhysicalDeviceMeshShaderFeaturesEXT mFeaturesMeshShader;

	// Updater:
	avk::updater* mSharedUpdater;

	virtual void uploadConfig() = 0;

};