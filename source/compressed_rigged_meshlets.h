#pragma once
#include <auto_vk_toolkit.hpp>
#include "data_structs.h"


class compressed_rigged_meshlets : public avk::invokee
{

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<std::tuple<animated_model_data, additional_animated_model_data>> mAnimatedModels;

	std::vector<avk::buffer> mViewProjBuffers;
	avk::buffer mMaterialBuffer;
	avk::buffer mMeshletsBuffer;
	std::array<std::vector<avk::buffer>, cConcurrentFrames> mBoneMatricesBuffersAni;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipeline;
	avk::quake_camera mQuakeCam;
	size_t mNumMeshletWorkgroups;

	std::vector<avk::buffer_view> mPositionBuffers;
	std::vector<avk::buffer_view> mTexCoordsBuffers;
	std::vector<avk::buffer_view> mNormalBuffers;
	std::vector<avk::buffer_view> mBoneWeightsBuffers;
	std::vector<avk::buffer_view> mBoneIndicesBuffers;

	bool mHighlightMeshlets;

public: // v== avk::invokee overrides which will be invoked by the framework ==v

	compressed_rigged_meshlets(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	/** Creates buffers for all the drawcalls.
	 *  Called after everything has been loaded and split into meshlets properly.
	 *  @param dataForDrawCall		The loaded data for the drawcalls.
	 *	@param drawCallsTarget		The target vector for the draw call data.
	 */
	void add_draw_calls(std::vector<loaded_data_for_draw_call>& dataForDrawCall, std::vector<data_for_draw_call>& drawCallsTarget);

	void initialize() override;

	void update() override;

	void render() override;



}; // compressed_rigged_meshlets

