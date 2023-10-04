#pragma once


#include "shared_data.h"
#include <functional>
#include "pipelines/PipelineInterface.h"

#define STARTUP_FILE "../../assets/skinning_dummy/dummy.fbx"
//#define STARTUP_FILE "assets/two_seperate_triangles_2.glb"

class PipelineInterface;

class MeshletsApp : public avk::invokee, public SharedData
{
	struct FreeCMDBufferExecutionData {
		enum FreeCMDBufferExecutionType { LOAD_NEW_FILE, CHANGE_PIPELINE, CHANGE_MESHLET_BUILDER };
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

	bool mInverseMeshRootFix = true;

	int mCurrentlyPlayingAnimationId = -1;	// negative if no animation currently

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;
	
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