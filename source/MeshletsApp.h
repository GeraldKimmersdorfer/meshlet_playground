#pragma once

#include "imgui.h"
#include "imgui_manager.hpp"
#include "invokee.hpp"

#include "SharedData.h"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include <functional>

#include "pipelines/PipelineInterface.h"
#include "vertexcompressor/VertexCompressionInterface.h"
#include "meshletbuilder/MeshletbuilderInterface.h"

#define STARTUP_FILE R"(assets/mixamo_single_no_texture.fbx)"
//#define STARTUP_FILE R"(assets/weight_meshlet_creation_test.fbx)"

class PipelineInterface;

class MeshletsApp : public avk::invokee, public SharedData
{
	struct FreeCMDBufferExecutionData {
		enum FreeCMDBufferExecutionType { LOAD_NEW_FILE, CHANGE_PIPELINE, CHANGE_MESHLET_BUILDER, CHANGE_VERTEX_COMPRESSOR };
		FreeCMDBufferExecutionType type;
		std::string mNextFileName;
		int mFrameWait = -1;
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

public:

	MeshletsApp(avk::queue& aQueue) : mQueue{ &aQueue } {}
	~MeshletsApp();

	// Empties all vectors and resets stuff before loading new file
	void reset();
	void load(const std::string& filename);

	void initGUI();
	void initReusableObjects();
	virtual void uploadConfig() override;

	virtual MeshletbuilderInterface* getCurrentMeshletBuilder() override;

	virtual VertexCompressionInterface* getCurrentVertexCompressor() override;

	void initialize() override;
	void update() override;
	void render() override;


private: // v== Member variables ==v

	FreeCMDBufferExecutionData mExecutionData;
	void freeCommandBufferAndExecute(FreeCMDBufferExecutionData executeAfterwards);
	void executeWithFreeCommandBuffer();

	std::pair<int, int> mPipelineID = { -1, 0 };
	std::vector<std::unique_ptr<PipelineInterface>> mPipelines;
	std::pair<int, int> mMeshletBuilderID = { 0, 0 };
	std::vector<std::unique_ptr<MeshletbuilderInterface>> mMeshletBuilder;
	std::pair<int, int> mVertexCompressorID = { 0, 0 };
	std::vector<std::unique_ptr<VertexCompressionInterface>> mVertexCompressors;

	bool mInverseMeshRootFix = true;

	int mCurrentlyPlayingAnimationId = -1;	// negative if no animation currently

	avk::queue* mQueue;
	
	std::vector<animation_data> mAnimations;

	std::vector<glm::mat4> mInitialBoneTransforms;
	std::vector<glm::mat4> mBoneTransforms;

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;

	uint32_t mTaskInvocationsExt;

	avk::query_pool mTimestampPool;
	uint64_t mLastTimestamp = 0;
	uint64_t mLastDrawMeshTasksDuration = 0;
	uint64_t mLastFrameDuration = 0;

	avk::query_pool mPipelineStatsPool;
	std::array<uint64_t, 3> mPipelineStats;

	std::string mLastErrorMessage = "";

};