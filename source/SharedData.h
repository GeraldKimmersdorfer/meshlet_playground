#pragma once

#include "invokee.hpp"
#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "vk_convenience_functions.hpp"
#include <glm/glm.hpp>
#include <avk/avk.hpp>
#include "configure_and_compose.hpp"
#include "material_image_helpers.hpp"
#include <auto_vk_toolkit.hpp>
#include <meshlet_helpers.hpp>
#include "shared_structs.h"

class MeshletbuilderInterface;
class VertexCompressionInterface;

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
	avk::updater* mSharedUpdater; // Not necessary anymore
	avk::descriptor_cache mDescriptorCache;

	virtual void uploadConfig() = 0;

	virtual MeshletbuilderInterface* getCurrentMeshletBuilder() = 0;
	virtual VertexCompressionInterface* getCurrentVertexCompressor() = 0;

	void attachSharedPipelineConfiguration(avk::graphics_pipeline_config* pipeConfig, std::vector<avk::binding_data>* staticDescriptors);
	std::vector<avk::binding_data> getDynamicDescriptorBindings(int64_t inFlightIndex);

protected:
	void hudSharedConfiguration(bool& config_has_changed);

private:
	avk::cfg::culling_mode mCullingMode = avk::cfg::culling_mode::cull_back_faces;
	avk::cfg::polygon_drawing_mode mDrawingMode = avk::cfg::polygon_drawing_mode::fill;

};