#pragma once

#include "imgui.h"
#include "imgui_manager.hpp"
#include "invokee.hpp"

#include "shared_data.h"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include <functional>

#include "pipelines/PipelineInterface.h"
#include "vertexcompressor/VertexCompressionInterface.h"
#include "meshletbuilder/MeshletbuilderInterface.h"

//#define STARTUP_FILE "../../assets/skinning_dummy/dummy.fbx"
//#define STARTUP_FILE "assets/two_seperate_triangles_2.glb"
#define STARTUP_FILE R"(C:\Users\Vorto\OneDrive - TU Wien\Bachelor-Arbeit\Assets\Mixamo Group\Single_noTexture.fbx)"

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

	int mCurrentPipelineID = -1;
	int mSelectedPipelineID = 0;
	std::vector<std::unique_ptr<PipelineInterface>> mPipelines;
	int mCurrentMeshletBuilderID = 0;
	int mSelectedMeshBuilderID = 0;
	std::vector<std::unique_ptr<MeshletbuilderInterface>> mMeshletBuilder;
	int mCurrentVertexCompressorID = 0;
	int mSelectedVertexCompressorID = 0;
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