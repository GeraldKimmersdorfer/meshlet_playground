/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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

#include "statistics/PropertyManager.h"

#include "statistics/CPUTimer.h"
#include "statistics/AvkTimer.h"
#include "statistics/AverageNumberProperty.h"

#include "helpers/camera_storage.h"

#define STARTUP_FILE R"(assets/mixamo_single_no_texture.fbx)"
 //#define STARTUP_FILE R"(C:\Users\Vorto\OneDrive - TU Wien\Bachelor-Arbeit\Assets\Mixamo Group\Mixamo-Group-No-Materials.fbx)"
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

	struct benchmark_settings {
		bool discardFragments = true;
		bool hideGui = true;
		bool resetTimer = true;
		bool disableVSync = true;
		int frameCount = 250;
		int copyCount = 500;
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

	void compileAndLoadNextPipeline();
	void setVSync(bool enable); // based on mVSyncEnabled
	void reportProperties();
	void resetTimer();
	void startBenchmark();
	void endBenchmark();

	void evaluateModelProperties(const std::string& filename);

	std::pair<int, int> mPipelineID = { -1, 0 };
	std::vector<std::unique_ptr<PipelineInterface>> mPipelines;
	std::pair<int, int> mMeshletBuilderID = { 0, 0 };
	std::vector<std::unique_ptr<MeshletbuilderInterface>> mMeshletBuilder;
	std::pair<int, int> mVertexCompressorID = { 0, 0 };
	std::vector<std::unique_ptr<VertexCompressionInterface>> mVertexCompressors;

	bool mInverseMeshRootFix = true;

	int mCurrentlyPlayingAnimationId = -1;	// negative if no animation currently
	float mCurrentAnimationProgress = 0.0f;
	bool mCurrentAnimationPaused = false;
	bool mCurrentAnimationProgressChanged = false;

	avk::queue* mQueue;

	std::vector<animation_data> mAnimations;

	std::vector<glm::mat4> mInitialBoneTransforms;
	std::vector<glm::mat4> mBoneTransforms;

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;
	std::map<std::string, CameraDefinition> mCameraDefinitions;

	std::unique_ptr<AvkTimer> mAvkFrameTimer;
	std::shared_ptr<AverageNumberProperty<float>> mAvkFrameProperty;
	std::unique_ptr<CpuTimer> mCpuFrameTimer;
	std::shared_ptr<AverageNumberProperty<float>> mCpuFrameProperty;



	uint32_t mTaskInvocationsExt = 0;

	avk::graphics_pipeline mBackgroundPipeline;

	std::string mShowErrorMessage = "";

	std::vector<std::string> mSelectedProperties = { "cpu_frame","gpu_frame" };
	benchmark_settings mBenchmarkSettings;
	benchmark_settings mBenchmarkSettingsSetback;
	long mBenchmarkRestFrameCounter = -1; // > 0 means benchmark is running
	bool mVSyncEnabled = true;
	bool mShowGUI = true;

};