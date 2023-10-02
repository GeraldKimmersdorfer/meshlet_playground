#include "MeshletsApp.h"
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
#include "pipelines/VertexPulledIndirectPipeline.h"
#include "pipelines/MeshPipeline.h"
#include "pipelines/VertexIndirectPipeline.h"

#include <functional>

std::vector<glm::mat4> globalTransformPresets = {
	glm::mat4(1.0),				// none
	 glm::rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f)) * glm::scale(glm::vec3(0.01f))	// lucy
};
const char* transformPresetsNames = "None\0Lucy\0";
int selectedGlobalTransformPresetId = 0;

void openDialogOptionPane(const char* vFilter, IGFDUserDatas vUserDatas, bool* vCantContinue)
{
	ImGui::Text("IMPORT OPTIONS");
	ImGui::Separator();
	//ImGui::Combo("Meshlet builder", (int*)(void*)&selectedMeshletInterpreter, "AVK-Default\0Meshoptimizer\0");
	ImGui::Combo("Global transform", (int*)(void*)&selectedGlobalTransformPresetId, transformPresetsNames);
}

MeshletsApp::~MeshletsApp()
{
	mPipelines[mCurrentPipelineID]->destroy();
	mPipelines.clear();
}

void MeshletsApp::reset()
{
	mIndices.clear();
	mMeshData.clear();
	mVertexData.clear();
	mAnimations.clear(); 
	mBoneTransformBuffers.clear();
	mBoneTransforms.clear(); 
	mBoneTransformBuffers.clear();
	mImageSamplers.clear();
}

void MeshletsApp::load(const std::string& filename)
{
	reset();

	avk::model& model = mModel = std::move(avk::model_t::load_from_file(filename, aiProcess_Triangulate));
	std::vector<avk::material_config> allMatConfigs;
	mCurrentlyPlayingAnimationId = -1;

	const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
	const auto &globalTransform = globalTransformPresets[selectedGlobalTransformPresetId];

	// get all the meshlet indices of the model
	const auto meshIndicesInOrder = model->select_all_meshes();
	auto distinctMaterials = model->distinct_material_configs(); 
	// add all the materials of the model
	for (auto& pair : distinctMaterials) allMatConfigs.push_back(pair.first);

	// Load all Animations:

	auto aScene = model->handle();
	for (int i = 0; i < aScene->mNumAnimations; i++) {
		auto* aAnim = aScene->mAnimations[i];
		auto& anim = mAnimations.emplace_back(animation_data{
			.mName = aAnim->mName.C_Str(),
			.mDurationTicks = aAnim->mDuration,
			.mDurationSeconds = aAnim->mDuration / aAnim->mTicksPerSecond,
			.mChannelCount = aAnim->mNumChannels,
			.mTicksPerSecond = aAnim->mTicksPerSecond
			});
		anim.mClip = model->load_animation_clip(i, 0.0, anim.mDurationTicks);
		anim.mAnimation = model->prepare_animation(i, meshIndicesInOrder);
	}

	// Fill the bone transforms array with init data
	mBoneTransforms.resize(model->num_bone_matrices(meshIndicesInOrder)); //OMG... num_bone_matrices returns fake bones for meshes. I DONT WANT THAT!!!
	
	// ToDo: Gather init pose

	for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
		auto meshIndex = meshIndicesInOrder[mpos];
		std::string meshname = model->name_of_mesh(mpos);
		auto* amesh = aScene->mMeshes[meshIndex];

		auto& mesh = mMeshData.emplace_back(mesh_data{
			.mTransformationMatrix = globalTransform * model->transformation_matrix_for_mesh(meshIndex),
			.mVertexOffset = static_cast<uint32_t>(mVertexData.size()),
			.mIndexOffset = static_cast<uint32_t>(mIndices.size()),
			.mMaterialIndex = 0,
			.mAnimated = static_cast<int32_t>(amesh->HasBones()),
			});

		// Find and assign the correct material in the allMatConfigs vector
		for (auto pair : distinctMaterials) {
			if (std::end(pair.second) != std::ranges::find(pair.second, meshIndex)) break;
			mesh.mMaterialIndex++;
		}

		auto selection = avk::make_model_references_and_mesh_indices_selection(model, meshIndex);
		auto [meshPositions, meshIndices] = avk::get_vertices_and_indices(selection);
		auto meshNormals = avk::get_normals(selection);
		auto meshTexCoords = avk::get_2d_texture_coordinates(selection, 0);
		auto meshBoneIndices = avk::get_bone_indices_for_single_target_buffer(selection, meshIndicesInOrder);
		auto meshBoneWeights = avk::get_bone_weights(selection);

		mesh.mIndexCount = meshIndices.size();
		mesh.mVertexCount = meshPositions.size();

		for (int i = 0; i < meshPositions.size(); i++) {
			auto& vd = mVertexData.emplace_back(vertex_data{ 
				.mPositionTxX = glm::vec4(meshPositions[i], meshTexCoords[i].x), 
				.mTxYNormal = glm::vec4(meshTexCoords[i].y, meshNormals[i]),
				.mBoneIndices = meshBoneIndices[i],
				.mBoneWeights = meshBoneWeights[i]
			});
		}

		mIndices.insert(mIndices.end(), meshIndices.begin(), meshIndices.end());
	} 


	// ======== START UPLOADING TO GPU =============
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device, 
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mVertexData.data(), 0) }, *mQueue)->wait_until_signalled();

	// buffers for the animated bone matrices, will be populated before rendering
	for (size_t cfi = 0; cfi < concurrentFrames; ++cfi) {
		mBoneTransformBuffers.push_back(avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_data(mBoneTransforms)
		));
	}

	mIndexBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::index_buffer_meta::create_from_data(mIndices).describe_only_member(mIndices[0], avk::content_description::index),
		avk::storage_buffer_meta::create_from_data(mIndices)
	);
	avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(mIndices.data(), 0) }, * mQueue)->wait_until_signalled();


	mMeshesBuffer = avk::context().create_buffer(
		avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mMeshData)
	);
	avk::context().record_and_submit_with_fence({ mMeshesBuffer->fill(mMeshData.data(), 0), }, *mQueue)->wait_until_signalled();

	auto [gpuMaterials, imageSamplers, matCommands] = avk::convert_for_gpu_usage<avk::material_gpu_data>(
		allMatConfigs, false, false,
		avk::image_usage::general_texture,
		avk::filter_mode::trilinear
	);

	avk::context().record_and_submit_with_fence({
		matCommands
		}, *mQueue)->wait_until_signalled();

	mMaterialsBuffer = avk::context().create_buffer(
		avk::memory_usage::host_visible, {},
		avk::storage_buffer_meta::create_from_data(gpuMaterials)
	);
	mMaterialsBuffer->fill(gpuMaterials.data(), 0);

	mImageSamplers = std::move(imageSamplers);
}

void MeshletsApp::initGUI()
{
	auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
	if (nullptr != imguiManager) {
		imguiManager->add_callback([
			this, imguiManager,
				timestampPeriod = std::invoke([]() {
				// get timestamp period from physical device, could be different for other GPUs
				auto props = avk::context().physical_device().getProperties();
				return static_cast<double>(props.limits.timestampPeriod);
					}),
				lastFrameDurationMs = 0.0,
				lastDrawMeshTasksDurationMs = 0.0
		]() mutable { 
				bool config_has_changed = false;
				ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Separator();
				if (ImGui::Button("Open File")) {
					ImGuiFileDialog::Instance()->OpenDialogWithPane("open_file", "Choose File", "{.fbx,.obj,.dae,.ply,.gltf,.glb}", ".", "", std::bind(&openDialogOptionPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 300.0, 1, (IGFDUserDatas)nullptr, ImGuiFileDialogFlags_Modal);
				}

				ImGui::Separator();
				if (ImGui::BeginCombo("Animation", mCurrentlyPlayingAnimationId >= 0 ? mAnimations[mCurrentlyPlayingAnimationId].mName.c_str(): "None")) {
					if (ImGui::Selectable("None", mCurrentlyPlayingAnimationId < 0)) mCurrentlyPlayingAnimationId = -1;
					for (int n = 0; n < mAnimations.size(); n++) {
						bool is_selected = (mCurrentlyPlayingAnimationId == n);
						if (ImGui::Selectable(mAnimations[n].mName.c_str(), is_selected)) mCurrentlyPlayingAnimationId = n;
						if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (mCurrentlyPlayingAnimationId >= 0) {
					ImGui::Checkbox("Inverse Mesh Root Fix", &mInverseMeshRootFix);
				}
				ImGui::Separator();
				ImGui::TextColored(ImVec4(.5f, .3f, .4f, 1.f), "Timestamp Period: %.3f ns", timestampPeriod);
				lastFrameDurationMs = glm::mix(lastFrameDurationMs, mLastFrameDuration * 1e-6 * timestampPeriod, 0.05);
				lastDrawMeshTasksDurationMs = glm::mix(lastDrawMeshTasksDurationMs, mLastDrawMeshTasksDuration * 1e-6 * timestampPeriod, 0.05);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Frame time (timer queries): %.3lf ms", lastFrameDurationMs);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "drawMeshTasks took        : %.3lf ms", lastDrawMeshTasksDurationMs);
				ImGui::Text("mPipelineStats[0]         : %llu", mPipelineStats[0]);
				ImGui::Text("mPipelineStats[1]         : %llu", mPipelineStats[1]);
				ImGui::Text("mPipelineStats[2]         : %llu", mPipelineStats[2]);

				ImGui::Separator();
				bool quakeCamEnabled = mQuakeCam.is_enabled();
				if (ImGui::Checkbox("Enable Quake Camera", &quakeCamEnabled)) {
					if (quakeCamEnabled) { // => should be enabled
						mQuakeCam.set_matrix(mOrbitCam.matrix());
						mQuakeCam.enable();
						mOrbitCam.disable();
					}
				}
				if (quakeCamEnabled) {
					ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1] to exit Quake Camera navigation.");
					if (avk::input().key_pressed(avk::key_code::f1)) {
						mOrbitCam.set_matrix(mQuakeCam.matrix());
						mOrbitCam.enable();
						mQuakeCam.disable();
					}
				}
				if (imguiManager->begin_wanting_to_occupy_mouse() && mOrbitCam.is_enabled()) mOrbitCam.disable();
				if (imguiManager->end_wanting_to_occupy_mouse() && !mQuakeCam.is_enabled()) mOrbitCam.enable();
				ImGui::Separator();

				ImGui::Separator();
				if (ImGui::CollapsingHeader("Pipeline-Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
					static int mSelectedPipelineID = 0;
					if (ImGui::BeginCombo("Pipeline", mPipelines[mSelectedPipelineID]->getName().c_str())) {
						for (int n = 0; n < mPipelines.size(); n++) {
							bool is_selected = (mSelectedPipelineID == n);
							if (ImGui::Selectable(mPipelines[n]->getName().c_str(), is_selected)) mSelectedPipelineID = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					mPipelines[mSelectedPipelineID]->hud_setup(config_has_changed);

					if (ImGui::Button("Compile & Load Pipeline")) {
						bool withoutError = false;
						try {
							mPipelines[mSelectedPipelineID]->compile();
							withoutError = true;
						}
						catch (const std::exception& e) {
							mLastErrorMessage = e.what();
							ImGui::OpenPopup("Compile Error");
						}
						if (withoutError) {
							freeCommandBufferAndExecute({
								.type = FreeCMDBufferExecutionData::CHANGE_PIPELINE,
								.mNextPipelineID = mSelectedPipelineID
							});
						}
					}
				}

				ImGui::Separator();
				if (ImGui::CollapsingHeader("Pipeline-Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					if (mCurrentPipelineID >= 0) mPipelines[mCurrentPipelineID]->hud_config(config_has_changed);
				}

				if (ImGuiFileDialog::Instance()->Display("open_file"))
				{
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						freeCommandBufferAndExecute({
							.type = FreeCMDBufferExecutionData::LOAD_NEW_FILE,
							.mNextFileName = ImGuiFileDialog::Instance()->GetFilePathName()
							});
					}
					ImGuiFileDialog::Instance()->Close();
				}

				ImGuiIO& io = ImGui::GetIO();
				ImVec2 pos(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoDecoration
					| ImGuiWindowFlags_AlwaysAutoResize
					| ImGuiWindowFlags_NoSavedSettings;
				ImGui::SetNextWindowSize({ 400, -1 });
				if (ImGui::BeginPopupModal("Compile Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::TextWrapped(mLastErrorMessage.c_str());
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 0.1);
					if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}

				ImGui::End();

				if (config_has_changed) uploadConfig();
			});
	}
}

void MeshletsApp::initReusableObjects()
{
	// ===== CPU CAMERA ======
	mOrbitCam.set_translation({ 0.0f, 1.0f, 3.0f });
	mOrbitCam.set_pivot_distance(3.0f);
	mQuakeCam.set_translation({ 0.0f, 0.0f, 5.0f });
	mOrbitCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
	mQuakeCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
	avk::current_composition()->add_element(mOrbitCam);
	avk::current_composition()->add_element(mQuakeCam);
	mQuakeCam.disable();

	// ===== CPU UPDATER ==== 
	mSharedUpdater = &mUpdater.emplace();
	mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
		this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});

	// ===== DESCRIPTOR CACHE ====
	mDescriptorCache = avk::context().create_descriptor_cache();

	// ==== FETCH DEVICE PROPERTIES AND FEATURES ===
	mNvPipelineSupport = avk::context().supports_mesh_shader_nv(avk::context().physical_device());
	mFeatures2.pNext = &mFeaturesMeshShader; // get all in one swoop
	avk::context().physical_device().getFeatures2(&mFeatures2);
	mProps2.pNext = &mPropsMeshShader; mPropsMeshShader.pNext = &mPropsSubgroup;  // get all in one swoop
	if (mNvPipelineSupport) mPropsSubgroup.pNext = &mPropsMeshShaderNV;
	avk::context().physical_device().getProperties2(&mProps2);
	LOG_INFO(std::format("Max. preferred task threads is {}, mesh threads is {}, subgroup size is {}.", mPropsMeshShader.maxPreferredTaskWorkGroupInvocations, mPropsMeshShader.maxPreferredMeshWorkGroupInvocations, mPropsSubgroup.subgroupSize));
	LOG_INFO(std::format("This device supports the following subgroup operations: {}", vk::to_string(mPropsSubgroup.supportedOperations)));
	LOG_INFO(std::format("This device supports subgroup operations in the following stages: {}", vk::to_string(mPropsSubgroup.supportedStages)));
	mTaskInvocationsExt = mPropsMeshShader.maxPreferredTaskWorkGroupInvocations;

	// ===== GPU QUERY POOLS ====
	mTimestampPool = avk::context().create_query_pool_for_timestamp_queries(
		static_cast<uint32_t>(avk::context().main_window()->number_of_frames_in_flight()) * 2
	);
	mPipelineStatsPool = avk::context().create_query_pool_for_pipeline_statistics_queries(
		vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations | vk::QueryPipelineStatisticFlagBits::eMeshShaderInvocationsEXT | vk::QueryPipelineStatisticFlagBits::eTaskShaderInvocationsEXT,
		avk::context().main_window()->number_of_frames_in_flight()
	);

	// ===== GPU CAMERA BUFFER ====
	const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
	for (int i = 0; i < concurrentFrames; ++i) {
		mViewProjBuffers.push_back(avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::uniform_buffer_meta::create_from_data(glm::mat4())
		));
	}

	// ===== GPU CONFIG BUFFER ====
	mConfigurationBuffer = avk::context().create_buffer(
		avk::memory_usage::host_coherent, {},
		avk::uniform_buffer_meta::create_from_data(mConfig)
	);
	uploadConfig();
}

void MeshletsApp::uploadConfig()
{
	mConfigurationBuffer->fill(&mConfig, 0);
}

void MeshletsApp::initialize()
{
	this->initReusableObjects();
	this->initGUI();
	this->load(STARTUP_FILE);
	// TODO QUERY FOR NV PIPELINE SUPPORT
	mPipelines.push_back(std::make_unique<VertexPulledIndirectPipeline>(this));
	mPipelines.push_back(std::make_unique<VertexIndirectPipeline>(this));
	mPipelines.push_back(std::make_unique<MeshPipeline>(this));
	//mPipelines[mCurrentPipelineID]->initialize(mQueue);
}

void MeshletsApp::update()
{
	using namespace avk;
	if (input().key_pressed(avk::key_code::c)) {
		// Center the cursor:
		auto resolution = context().main_window()->resolution();
		context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
	}
	if (input().key_pressed(avk::key_code::escape)) {
		avk::current_composition()->stop();
	}

	// After wanting to load a file, the following code waits for number_of_frames_in_flight, such
	// that all buffers/descriptors... are not in a queue and can safely be destroyed. (Is there a vk-toolkit way to do this?)
	if (mExecutionData.mFrameWait >= 0) {
		if (mExecutionData.mFrameWait-- == 0)
			executeWithFreeCommandBuffer();
	}
}

void MeshletsApp::render()
{
	if (mExecutionData.mFrameWait >= 0) return;	// We want to free the commandPool such that we can load a new file
	if (mCurrentPipelineID < 0) return;	// No pipeline selected
	using namespace avk;

	auto mainWnd = context().main_window();
	auto inFlightIndex = mainWnd->current_in_flight_index();

	if (mCurrentlyPlayingAnimationId >= 0) {
		auto& aData = mAnimations[mCurrentlyPlayingAnimationId];
		auto& animation = aData.mAnimation;
		auto& clip = aData.mClip;
		const auto doubleTime = fmod(time().absolute_time_dp(), aData.mDurationSeconds * 2);
		auto time = glm::lerp(0.0, aData.mDurationSeconds, (doubleTime > aData.mDurationSeconds ? doubleTime - aData.mDurationSeconds : doubleTime) / aData.mDurationSeconds);
		auto targetMemory = mBoneTransforms.data();

		animation.animate(clip, time, [this, &animation, targetMemory](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix, const glm::mat4& aLocalTransformMatrix, size_t aAnimatedNodeIndex, size_t aBoneMeshTargetIndex, double aAnimationTimeInTicks) {
			glm::mat4 result;
			if (mInverseMeshRootFix) result = aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
			else result = aTransformMatrix * aInverseBindPoseMatrix;
			targetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = result;
			}
		);
	}

	auto viewProjMat = mQuakeCam.is_enabled()
		? mQuakeCam.projection_and_view_matrix()
		: mOrbitCam.projection_and_view_matrix();
	auto emptyCmd = mViewProjBuffers[inFlightIndex]->fill(glm::value_ptr(viewProjMat), 0);

	// Get a command pool to allocate command buffers from:
	auto& commandPool = context().get_command_pool_for_single_use_command_buffers(*mQueue);

	// The swap chain provides us with an "image available semaphore" for the current frame.
	// Only after the swapchain image has become available, we may start rendering into it.
	auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

	// Create a command buffer and render into the *current* swap chain image:
	auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	const auto firstQueryIndex = static_cast<uint32_t>(inFlightIndex) * 2;
	if (mainWnd->current_frame() > mainWnd->number_of_frames_in_flight()) // otherwise we will wait forever
	{
		auto timers = mTimestampPool->get_results<uint64_t, 2>(
			firstQueryIndex, 2, vk::QueryResultFlagBits::e64 // | vk::QueryResultFlagBits::eWait // => ensure that the results are available (shouldnt be necessary)
		);
		mLastDrawMeshTasksDuration = timers[1] - timers[0];
		mLastFrameDuration = timers[1] - mLastTimestamp;
		mLastTimestamp = timers[1];

		mPipelineStats = mPipelineStatsPool->get_results<uint64_t, 3>(inFlightIndex, 1, vk::QueryResultFlagBits::e64);
	}

	context().record({
			mPipelineStatsPool->reset(inFlightIndex, 1),
			mPipelineStatsPool->begin_query(inFlightIndex),
			mTimestampPool->reset(firstQueryIndex, 2),     // reset the two values relevant for the current frame in flight
			mTimestampPool->write_timestamp(firstQueryIndex + 0, stage::all_commands), // measure before drawMeshTasks*

			// Upload the updated bone matrices into the buffer for the current frame (considering that we have cConcurrentFrames-many concurrent frames):
			mViewProjBuffers[inFlightIndex]->fill(glm::value_ptr(viewProjMat), 0),
			mBoneTransformBuffers[inFlightIndex]->fill(mBoneTransforms.data(), 0),

			sync::global_memory_barrier(stage::all_commands >> stage::all_commands, access::memory_write >> access::memory_write | access::memory_read),

			mPipelines[mCurrentPipelineID]->render(inFlightIndex),

			mTimestampPool->write_timestamp(firstQueryIndex + 1, stage::mesh_shader),
			mPipelineStatsPool->end_query(inFlightIndex)
		})
		.into_command_buffer(cmdBfr)
		.then_submit_to(*mQueue)
		// Do not start to render before the image has become available:
		.waiting_for(imageAvailableSemaphore >> stage::color_attachment_output)
		.submit();

	mainWnd->handle_lifetime(std::move(cmdBfr));

}


void MeshletsApp::freeCommandBufferAndExecute(FreeCMDBufferExecutionData executeAfterwards)
{
	mExecutionData = std::move(executeAfterwards);
	mExecutionData.mFrameWait = avk::context().main_window()->number_of_frames_in_flight();
}

void MeshletsApp::executeWithFreeCommandBuffer()
{
	if (mExecutionData.type == FreeCMDBufferExecutionData::LOAD_NEW_FILE) {
		int nextPipeline = mExecutionData.mNextPipelineID >= 0 ? mExecutionData.mNextPipelineID : mCurrentPipelineID;
		if (mCurrentPipelineID >= 0) mPipelines[mCurrentPipelineID]->destroy();
		load(mExecutionData.mNextFileName);
		if (mCurrentPipelineID >= 0) mPipelines[nextPipeline]->initialize(mQueue);
		mCurrentPipelineID = nextPipeline;
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_PIPELINE) {
		//if (mCurrentPipelineID == mExecutionData.mNextPipelineID) return;
		if (mCurrentPipelineID >= 0) mPipelines[mCurrentPipelineID]->destroy();
		mCurrentPipelineID = mExecutionData.mNextPipelineID;
		mPipelines[mCurrentPipelineID]->initialize(mQueue);
	}
}
