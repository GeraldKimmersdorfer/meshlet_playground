#pragma once


#include "SharedData.h"

#include "pipelines/VertexPulledIndirectNoCompressionPipeline.h"

#include <functional>

//#define STARTUP_FILE "../../assets/skinning_dummy/dummy.fbx"
#define STARTUP_FILE "../../assets/two_objects_in_one.fbx"
#define USE_CACHE 0

enum PipelineType : int {
	VERTEX_PIPELINE,
	MESH_PIPELINE,
	NVIDIA_MESH_PIPELINE
};

enum MeshletInterpreter : int {
	AVK_DEFAULT,
	MESHOPTIMIZER
};

class MeshletsApp : public avk::invokee, public SharedData
{
public:

	MeshletsApp(avk::queue& aQueue) : mQueue{ &aQueue } {}

	// Empties all vectors and resets stuff before loading new file
	void reset();
	void load(const std::string& filename);

	void initCamera();
	void initGUI();
	void initGPUQueryPools();
	void loadDeviceProperties();

	void initialize() override;
	void update() override;
	void render() override;


private: // v== Member variables ==v
	int mSelectedPipelineIndex = 0;
	std::vector<std::unique_ptr<PipelineInterface>> mPipelines;

	bool mLoadNewFile = false;
	std::string mNewFileName;
	int mFrameWait = -1;

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

};