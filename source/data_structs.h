#pragma once
#include <auto_vk_toolkit.hpp>
#include "config.h"

struct alignas(16) push_constants
{
	VkBool32 mHighlightMeshlets;
};

/** Contains the necessary buffers for drawing everything */
struct data_for_draw_call
{
	avk::buffer mPositionsBuffer;
	avk::buffer mTexCoordsBuffer;
	avk::buffer mNormalsBuffer;
	avk::buffer mBoneIndicesBuffer;
	avk::buffer mBoneWeightsBuffer;

	glm::mat4 mModelMatrix;

	uint32_t mMaterialIndex;
	uint32_t mModelIndex;
};

/** Contains the data for each draw call */
struct loaded_data_for_draw_call
{
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec2> mTexCoords;
	std::vector<glm::vec3> mNormals;
	std::vector<uint32_t> mIndices;
	std::vector<glm::uvec4> mBoneIndices;
	std::vector<glm::vec4> mBoneWeights;

	glm::mat4 mModelMatrix;

	uint32_t mMaterialIndex;
	uint32_t mModelIndex;
};

struct additional_animated_model_data
{
	std::vector<glm::mat4> mBoneMatricesAni;
};

/** Helper struct for the animations. */
struct animated_model_data
{
	animated_model_data() = default;
	animated_model_data(animated_model_data&&) = default;
	animated_model_data& operator=(animated_model_data&&) = default;
	~animated_model_data() = default;
	// prevent copying of the data:
	animated_model_data(const animated_model_data&) = delete;
	animated_model_data& operator=(const animated_model_data&) = delete;

	std::string mModelName;
	avk::animation_clip_data mClip;
	uint32_t mNumBoneMatrices;
	size_t mBoneMatricesBufferIndex;
	avk::animation mAnimation;
	
	[[nodiscard]] double start_sec() const { return mClip.mStartTicks / mClip.mTicksPerSecond; }
	[[nodiscard]] double end_sec() const { return mClip.mEndTicks / mClip.mTicksPerSecond; }
	[[nodiscard]] double duration_sec() const { return end_sec() - start_sec(); }
	[[nodiscard]] double duration_ticks() const { return mClip.mEndTicks - mClip.mStartTicks; }
};

/** The meshlet we upload to the gpu with its additional data. */
struct alignas(16) meshlet
{
	glm::mat4 mTransformationMatrix;
	uint32_t mMaterialIndex;
	uint32_t mTexelBufferIndex;
	uint32_t mModelIndex;

	avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
};