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

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;

struct alignas(16) push_constants
{
	vk::Bool32 mHighlightMeshlets;
	int32_t    mVisibleMeshletIndexFrom;
	int32_t    mVisibleMeshletIndexTo;
};

struct mesh_data {
	glm::mat4 mTransformationMatrix;
	uint32_t mVertexOffset;			// Offset to first item in Positions Texel-Buffer
	uint32_t mIndexOffset;			// Offset to first item in Indices Texel-Buffer
	uint32_t mIndexCount;			// Amount if indices
	uint32_t mMaterialIndex;		// index of material for mesh
	int32_t mAnimated = false;		// Index offset inside bone matrix buffer, -1 if not animated
	glm::vec3 padding;
};

struct animation_data {
	std::string mName;
	double mDurationTicks;
	double mDurationSeconds;
	unsigned int mChannelCount;
	double mTicksPerSecond;
	avk::animation_clip_data mClip;
	avk::animation mAnimation;
};

struct vertex_data {
	glm::vec4 mPositionTxX;
	glm::vec4 mNormalTxY;
	glm::uvec4 mBoneIndices;
	glm::vec4 mBoneWeights;
};	// mind padding and alignment!

/** The meshlet we upload to the gpu with its additional data. */
struct alignas(16) meshlet
{
	uint32_t mMeshIndex;
	avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
};

/// <summary>
/// All pipelines will have access to this data
/// </summary>
class SharedData
{
public:

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

	// TODO: PUT IN A CONFIG UBO


};