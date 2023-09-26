#include "imgui.h"
#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include "vk_convenience_functions.hpp"
#include "../meshoptimizer/src/meshoptimizer.h"
#include "../ImGuiFileDialog/ImGuiFileDialog.h"

#include <functional>

#define STARTUP_FILE "../../assets/skinning_dummy/dummy.fbx"
//#define STARTUP_FILE "../../assets/two_objects_in_one.fbx"
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

class MeshletsApp : public avk::invokee
{
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
		uint32_t mVertexOffset;				// Offset to first item in Positions Texel-Buffer
		uint32_t mIndexOffset;				// Offset to first item in Indices Texel-Buffer
		uint32_t mIndexCount;				// Amount if indices
		uint32_t mMaterialIndex;			// index of material for mesh
		int32_t mAnimated = false;	// Index offset inside bone matrix buffer, -1 if not animated
		glm::vec3 padding;
	};

	struct vertex_data {
		glm::vec4 mPositionTxX;
		glm::vec4 mNormalTxY;
		glm::uvec4 mBoneIndices;
		glm::vec4 mBoneWeights;
	};	// mind padding and alignment!

	struct animation_data {
		std::string mName;
		double mDurationTicks;
		double mDurationSeconds;
		unsigned int mChannelCount;
		double mTicksPerSecond;
		avk::animation_clip_data mClip;
		avk::animation mAnimation;
	};

	/** The meshlet we upload to the gpu with its additional data. */
	struct alignas(16) meshlet
	{
		uint32_t mMeshIndex;
		avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
	};

public:

	MeshletsApp(avk::queue& aQueue) : mQueue{ &aQueue } {}

	void load(const std::string& filename);

	void initCamera();
	void initGUI();
	void initGPUQueryPools();
	void loadDeviceProperties();

	void initialize() override;
	void update() override;
	void render() override;


private: // v== Member variables ==v

	bool mLoadNewFile = false;
	std::string mNewFileName;
	int mFrameWait = -1;

	bool mInverseMeshRootFix = true;

	int mCurrentlyPlayingAnimationId = -1;	// negative if no animation currently

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<animation_data> mAnimations;
	std::vector<glm::mat4> mBoneTransforms;
	std::vector<glm::mat4> mInitialBoneTransforms;

	std::vector<avk::buffer> mViewProjBuffers;
	std::vector<avk::buffer> mBoneTransformBuffers;
	avk::buffer mMaterialsBuffer;
	avk::buffer mMeshletsBuffer;
	avk::buffer mMeshesBuffer;
	avk::buffer mVertexBuffer;
	
	avk::buffer mIndirectDrawCommandBuffer;
	avk::buffer mIndexBuffer;

	std::vector<avk::image_sampler> mImageSamplers;
	avk::graphics_pipeline mPipelineExt;
	avk::graphics_pipeline mPipelineNv;
	avk::graphics_pipeline mPipelineVertex;

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;

	uint32_t mNumMeshlets;
	uint32_t mNumMeshes;
	uint32_t mTaskInvocationsExt;
	uint32_t mTaskInvocationsNv;

	avk::buffer_view mPositionsBufferView;
	avk::buffer_view mTexCoordsBufferView;
	avk::buffer_view mNormalsBufferView;

	bool mHighlightMeshlets = true;
	int  mShowMeshletsFrom = 0;
	int  mShowMeshletsTo = 0;

	avk::query_pool mTimestampPool;
	uint64_t mLastTimestamp = 0;
	uint64_t mLastDrawMeshTasksDuration = 0;
	uint64_t mLastFrameDuration = 0;

	avk::query_pool mPipelineStatsPool;
	std::array<uint64_t, 3> mPipelineStats;

	PipelineType mSelectedPipeline = PipelineType::MESH_PIPELINE;

	// VK PROPERTIES:
	vk::PhysicalDeviceProperties2 mProps2;
	vk::PhysicalDeviceMeshShaderPropertiesEXT mPropsMeshShader;
	vk::PhysicalDeviceMeshShaderPropertiesNV mPropsMeshShaderNV;
	vk::PhysicalDeviceSubgroupProperties mPropsSubgroup;
	bool mNvPipelineSupport = false;

	// VK FEATURES:
	vk::PhysicalDeviceFeatures2 mFeatures2;
	vk::PhysicalDeviceMeshShaderFeaturesEXT mFeaturesMeshShader;


};