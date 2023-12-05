#include "MeshletsApp.h"

#include "configure_and_compose.hpp"
#include "material_image_helpers.hpp"
#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "vk_convenience_functions.hpp"
#include "../ImGuiFileDialog/ImGuiFileDialog.h"

#include "pipelines/MeshPipeline.h"
#include "pipelines/VertexIndirectPipeline.h"
#include "meshletbuilder/MeshoptimizerBuilder.h"
#include "meshletbuilder/AVKBuilder.h"
#include "meshletbuilder/BoneLUTDependentBuilder.h"

#include "vertexcompressor/NoCompression.h"
#include "vertexcompressor/BoneLUTCompression.h"
#include "vertexcompressor/MeshletRiggedCompression.h"
#include "vertexcompressor/DynamicMeshletVertexCodec.h"

#include <functional>

#include <glm/gtx/string_cast.hpp>
#include "shadercompiler/ShaderMetaCompiler.h"
#include "helpers/hud_helpers.h"

#include "statistics/TimerManager.h"
#include "statistics/CPUTimer.h"
#include "statistics/GPUTimer.h"

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
	ImGui::Combo("Global transform", (int*)(void*)&selectedGlobalTransformPresetId, transformPresetsNames);
}

std::vector<std::string> AssetFolderNames = {
	R"(C:\Users\Vorto\OneDrive - TU Wien\Bachelor-Arbeit\Assets)",
	R"(C:\Users\gkimmersdorfer\OneDrive - TU Wien\Bachelor-Arbeit\Assets)"
};
// Returns the first Asset folder thats available
std::string getBestAvailableAssetFolder() {
	for (int i = 0; i < AssetFolderNames.size(); i++) {
		auto path = std::filesystem::path(AssetFolderNames[i]);
		if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
			return AssetFolderNames[i];
		}
	}
	return ".";
}

MeshletsApp::~MeshletsApp()
{
	if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->destroy();
	getCurrentMeshletBuilder()->destroy();
	mMeshletBuilder.clear();
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

/// Transforms all positions into the range [0,1] 
void normalizePositions(std::vector<glm::vec3>& positions, glm::vec4& invTranslation, glm::vec4& invScale) {
	glm::vec3 aabbMax = glm::vec3(FLT_MIN); glm::vec3 aabbMin(FLT_MAX);
	for (int i = 0; i < positions.size(); i++) {
		aabbMax = glm::max(aabbMax, positions[i]);
		aabbMin = glm::min(aabbMin, positions[i]);
	}
	// Calculate scale and translation such that we end up with all positions in between [0,1]
	glm::vec3 aabbSize = aabbMax - aabbMin;
	// Translation should be aabbMin and Scale should be aabbSize (BE AWARE: THATS A NON-UNIFORM SCALE!)
	// To transform the points translate -aabbMin and scale 1.0 / Scale
	glm::mat4 pointTransform = glm::scale(1.0f / aabbSize) * glm::translate(-aabbMin);
	for (int i = 0; i < positions.size(); i++) {
		positions[i] = pointTransform * glm::vec4(positions[i], 1.0f);
	}
	invTranslation = glm::vec4(aabbMin, 1.0);
	invScale = glm::vec4(aabbSize, 1.0);
}

void MeshletsApp::load(const std::string& filename)
{
	reset();

	avk::model& model = mModel = std::move(avk::model_t::load_from_file(filename, aiProcess_Triangulate | aiProcess_FlipUVs));
	std::vector<avk::material_config> allMatConfigs;
	mCurrentlyPlayingAnimationId = -1;

	const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
	const auto& globalTransform = globalTransformPresets[selectedGlobalTransformPresetId];

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
	glm::vec3 aabbMinWS{ FLT_MAX, FLT_MAX, FLT_MAX };
	glm::vec3 aabbMaxWS{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
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

		// NOTE: Problem! Normalizing positions and integrating the inverse inside the transformation matrix
		// works fine with static meshes, but with rigged meshes I would have
		// to adapt various bone-data aswell. Since the animation code and bone code etc. is already integrated
		// in the AVKToolkit and I don't intend on changing this. I use a workaround where the shader first has
		// to undo the normalization as an extra step. For that purpose I could save the invTransform inside the
		// Mesh-Struct, but I'll use scale and translation since it should be faster.
		normalizePositions(meshPositions, mesh.mPositionNormalizationInvTranslation, mesh.mPositionNormalizationInvScale);

		mesh.mIndexCount = meshIndices.size();
		mesh.mVertexCount = meshPositions.size();

		for (int i = 0; i < meshPositions.size(); i++) {
			auto& vd = mVertexData.emplace_back(vertex_data{
				.mPositionTxX = glm::vec4(meshPositions[i], meshTexCoords[i].x),
				.mTxYNormal = glm::vec4(meshTexCoords[i].y, meshNormals[i]),
				.mBoneIndices = meshBoneIndices[i],
				.mBoneWeights = meshBoneWeights[i]
				});

			// "NORMALIZE" bone weights, meaning there are a lot of bone weights that don't add up to one.
			// I don't know where that is coming from, but i intend to fix this in the following lines of code
			// which stretches the weights that are > BONE_WEIGHT_EPSILON in regards of their weight.
			float weightSum = 0.0f;
			for (int bi = 0; bi < 4; bi++) {
				if (vd.mBoneWeights[bi] < BONE_WEIGHT_EPSILON) vd.mBoneWeights[bi] = 0.0f;
				else weightSum += vd.mBoneWeights[bi];
			}
			vd.mBoneWeights /= weightSum;

			glm::vec3 vertexPosWS{ mesh.mTransformationMatrix * glm::vec4(meshPositions[i], 1.0f) };
			aabbMinWS = glm::min(aabbMinWS, vertexPosWS);
			aabbMaxWS = glm::max(aabbMaxWS, vertexPosWS);
		}

		mIndices.insert(mIndices.end(), meshIndices.begin(), meshIndices.end());
	}

	// NOTE: The AABB computation will definitely not work for most models, as the rigging often adds scaling
	// To be exact it has to be computed for each animation individually. But this is out of scope here!
	glm::vec3 aabbWSExtend = aabbMaxWS - aabbMinWS;
	mConfig.mInstancingOffset = glm::vec4(0.0F, 0.0F, -aabbWSExtend.z, 0.0F);

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
	avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(mIndices.data(), 0) }, *mQueue)->wait_until_signalled();

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
			this, imguiManager
		]() mutable {
				bool config_has_changed = false;
					ImGuiIO& io = ImGui::GetIO();

					// ================ MAIN MENU ======================
					ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
					ImGui::SetWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
					ImGui::SetWindowSize({ -1, io.DisplaySize.y }, ImGuiCond_Always);

					if (ImGui::Button("Open File", ImVec2(ImGui::GetWindowSize().x * 0.96, 0.0f))) {
						ImGuiFileDialog::Instance()->OpenDialogWithPane("open_file", "Choose File", "{.fbx,.obj,.dae,.ply,.gltf,.glb}", getBestAvailableAssetFolder(), "", std::bind(&openDialogOptionPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 300.0, 1, (IGFDUserDatas)nullptr, ImGuiFileDialogFlags_Modal);
					}

					ImGui::Separator();

					if (ImGui::SliderInt("Instance Count", (int*)(void*)&mConfig.mInstanceCount, 1, MAX_INSTANCE_COUNT)) config_has_changed = true;

					if (ImGui::SliderFloat3("Instance Offset", &mConfig.mInstancingOffset.x, -100.0f, 100.0f)) config_has_changed = true;

					if (ImGui::BeginCombo("Animation", mCurrentlyPlayingAnimationId >= 0 ? mAnimations[mCurrentlyPlayingAnimationId].mName.c_str() : "None")) {
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
					bool quakeCamEnabled = mQuakeCam.is_enabled();
					if (ImGui::Checkbox("Enable Quake Camera [F5]", &quakeCamEnabled)) {
						if (quakeCamEnabled) { // => should be enabled
							mQuakeCam.set_matrix(mOrbitCam.matrix()); mQuakeCam.enable(); mOrbitCam.disable();
						}
					}
					if (avk::input().key_pressed(avk::key_code::f5)) {
						if (quakeCamEnabled) {
							mOrbitCam.set_matrix(mQuakeCam.matrix()); mOrbitCam.enable(); mQuakeCam.disable();
						}
						else {
							mQuakeCam.set_matrix(mOrbitCam.matrix()); mQuakeCam.enable(); mOrbitCam.disable();
						}
					}
					if (imguiManager->begin_wanting_to_occupy_mouse() && mOrbitCam.is_enabled()) mOrbitCam.disable();
					if (imguiManager->end_wanting_to_occupy_mouse() && !mQuakeCam.is_enabled()) mOrbitCam.enable();
					ImGui::Separator();

					if (ImGui::CollapsingHeader("Shared Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
						hudSharedConfiguration(config_has_changed);
					}

					ImGui::Separator();
					if (ImGui::CollapsingHeader("Meshlet-Building", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::BeginCombo("Builder", mMeshletBuilder[mMeshletBuilderID.second]->getName().c_str())) {
							for (int n = 0; n < mMeshletBuilder.size(); n++) {
								bool is_selected = (mMeshletBuilderID.second == n);
								if (ImGui::Selectable(mMeshletBuilder[n]->getName().c_str(), is_selected)) {
									mMeshletBuilderID.second = n;
									freeCommandBufferAndExecute({ .type = FreeCMDBufferExecutionData::CHANGE_MESHLET_BUILDER });
								}
								if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}

					ImGui::Separator();
					if (ImGui::BeginCombo("Compressor", mVertexCompressors[mVertexCompressorID.second]->getName().c_str())) {
						for (int n = 0; n < mVertexCompressors.size(); n++) {
							bool is_selected = (mVertexCompressorID.second == n);
							if (ImGui::Selectable(mVertexCompressors[n]->getName().c_str(), is_selected)) {
								mVertexCompressorID.second = n;
								freeCommandBufferAndExecute({ .type = FreeCMDBufferExecutionData::CHANGE_VERTEX_COMPRESSOR });
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					mVertexCompressors[mVertexCompressorID.second]->hud_config(config_has_changed);

					ImGui::Separator();
					if (ImGui::CollapsingHeader("Pipeline-Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0, 0.0, 0.0, 1.0));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, { 1.0f });
						bool isOpen = ImGui::BeginCombo("Pipeline", mPipelineID.second < 0 ? "Please select" : mPipelines[mPipelineID.second]->getName().c_str());
						ImGui::PopStyleColor(1);
						ImGui::PopStyleVar(1);
						if (isOpen) {
							for (int n = 0; n < mPipelines.size(); n++) {
								bool is_selected = (mPipelineID.second == n);
								if (ImGui::Selectable(mPipelines[n]->getName().c_str(), is_selected)) mPipelineID.second = n;
								if (is_selected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						mPipelines[mPipelineID.second]->hud_setup(config_has_changed);

						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8, 0.4, 0.4, 1.0));
						if (ImGui::Button("Compile & Load Pipeline [F1]", ImVec2(ImGui::GetWindowSize().x * 0.96, 0.0f))) compileAndLoadNextPipeline();
						ImGui::PopStyleColor(1);
					}

					if (avk::input().key_pressed(avk::key_code::f1)) compileAndLoadNextPipeline();

					ImGui::Separator();
					if (ImGui::CollapsingHeader("Pipeline-Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->hud_config(config_has_changed);
					}

					ImGui::End();

					// ================ STATS WINDOW ======================
					ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
					ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - ImGui::GetWindowWidth(), 0.0f), ImGuiCond_Always);
					ImGui::Text("%.3f ms/frame", 1000.0f / io.Framerate);
					ImGui::Text("%.1f FPS", io.Framerate);
					if (ImGui::Checkbox("VSync (FIFO Presentation Mode)", &mVSyncEnabled)) {
						if (mVSyncEnabled) avk::context().main_window()->set_presentaton_mode(avk::presentation_mode::fifo);
						else avk::context().main_window()->set_presentaton_mode(avk::presentation_mode::mailbox);
					}

					if (ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto timers = mTimer->get_timers();
						if (timers.size() > 0) {
							std::string currGroup = "";
							for (const auto& tmr : timers) {
								auto thisGroup = tmr->get_group();
								if (thisGroup != currGroup) {
									currGroup = thisGroup;
									ImGui::TextColored(ImVec4(.5f, .3f, .4f, 1.f), thisGroup.c_str());
								}
								ImGui::Text("%s: %.3f (%.3f)", tmr->get_name().c_str(), tmr->get_last_value(), tmr->get_averaged_value());
							}
						}
					}



					ImGui::End();

					// ================ FILE OPEN DIALOG ======================
					ImGui::SetNextWindowPos({ io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f }, ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
					ImGui::SetNextWindowSize({ 800, 400 }, ImGuiCond_FirstUseEver);
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

					// ================ ERROR DIALOG ======================
					static std::string lastErrorMessage;
					if (!mShowErrorMessage.empty()) {
						lastErrorMessage = std::move(mShowErrorMessage);
						ImGui::OpenPopup("Application Error");
					}
					ImGui::SetNextWindowPos({ io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f }, ImGuiCond_Always, { 0.5f, 0.5f });
					ImGui::SetNextWindowSize({ 600, -1 }, ImGuiCond_Always);
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8, 0.4, 0.4, 1.0));
					ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.8, 0.4, 0.4, 0.8));
					if (ImGui::BeginPopupModal("Application Error", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
					{
						ImGui::TextWrapped(lastErrorMessage.c_str());
						ImGui::NewLine();
						ImGui::SameLine(ImGui::GetWindowWidth() - 80);
						if (ImGui::Button("Got it.")) ImGui::CloseCurrentPopup();
						ImGui::EndPopup();
					}
					ImGui::PopStyleColor(2);



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

	// ===== BACKGROUND PIPELINE ====
	mBackgroundPipeline = avk::context().create_graphics_pipeline_for(
		avk::vertex_shader(ShaderMetaCompiler::precompile("background/screen_pass.vert", {})),
		avk::fragment_shader(ShaderMetaCompiler::precompile("background/solid_color.frag", {})),
		avk::cfg::front_face::define_front_faces_to_be_clockwise(),
		avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
		avk::context().create_renderpass({
			avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
			avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			}, avk::context().main_window()->renderpass_reference().subpass_dependencies())
	);

	// ===== CPU UPDATER ==== 
	mSharedUpdater = &mUpdater.emplace();
	mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
		this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
	this->mOrbitCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
	if (this->mPipelineID.first >= 0) freeCommandBufferAndExecute({ .type = FreeCMDBufferExecutionData::CHANGE_PIPELINE }); // Recreate pipeline
		}).update(mBackgroundPipeline);

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

		// ===== TIMING =====
		mTimer = std::make_unique<TimerManager>();
		mTimer->add_timer(std::make_shared<CpuTimer>("cpu_frame", "FRAME", 240, 1.0f / 60.0f));
		mTimer->add_timer(std::make_shared<GpuTimer>("gpu_frame", "FRAME", 240, 1.0f / 60.0f));


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

MeshletbuilderInterface* MeshletsApp::getCurrentMeshletBuilder()
{
	return mMeshletBuilder[mMeshletBuilderID.first].get();
}

VertexCompressionInterface* MeshletsApp::getCurrentVertexCompressor()
{
	return mVertexCompressors[mVertexCompressorID.first].get();
}

void MeshletsApp::initialize()
{
	this->initReusableObjects();
	this->initGUI();
	this->load(STARTUP_FILE);
	// TODO QUERY FOR NV PIPELINE SUPPORT
	mPipelines.push_back(std::make_unique<VertexIndirectPipeline>(this));
	mPipelines.push_back(std::make_unique<MeshPipeline>(this));
	//mPipelines[mCurrentPipelineID]->initialize(mQueue);

	mMeshletBuilder.push_back(std::make_unique<MeshoptimizerBuilder>(this));
	mMeshletBuilder.push_back(std::make_unique<AVKBuilder>(this));
	mMeshletBuilder.push_back(std::make_unique<BoneLUTDependentBuilder>(this));

	mVertexCompressors.push_back(std::make_unique<NoCompression>(this));
	mVertexCompressors.push_back(std::make_unique<BoneLUTCompression>(this));
	mVertexCompressors.push_back(std::make_unique<MeshletRiggedCompression>(this));
	mVertexCompressors.push_back(std::make_unique<DynamicMeshletVertexCodec>(this));
	if (mAnimations.size() > 0) mCurrentlyPlayingAnimationId = 0;
}

static long update_call_count = 0;
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
	// The ImGui-Context had to be already created and theres no other callback, thats why its here
	if (update_call_count++ == 0)  activateImGuiStyle(false, 0.9); //StyleColorsSpectrum();
}

void MeshletsApp::render()
{
	//if (mExecutionData.mFrameWait >= 0) return;	// We want to free the commandPool such that we can load a new file
	//if (mPipelineID.first < 0) return;	// No pipeline selected
	using namespace avk;

	mTimer->start_timer("cpu_frame");

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
			uint32_t index = aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex;
			glm::mat4 inverseMeshRootMatrix{ 1.0 };
			if (mInverseMeshRootFix) inverseMeshRootMatrix = aInverseMeshRootMatrix;
			result = inverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix; // *mInverseLocalPointTransforms[aInfo.mMeshIndexInModel];
			targetMemory[index] = result;
			});
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

	uint32_t inFlightIndexU32 = static_cast<uint32_t>(inFlightIndex);

	auto gpu_frame_timer = std::static_pointer_cast<GpuTimer>(mTimer->get("gpu_frame"));

	auto submissionData = context().record({

			gpu_frame_timer->startAction(inFlightIndexU32),

			// Upload the updated bone matrices into the buffer for the current frame (considering that we have cConcurrentFrames-many concurrent frames):
			mViewProjBuffers[inFlightIndex]->fill(glm::value_ptr(viewProjMat), 0),
			command::conditional([this] {
				return mExecutionData.mFrameWait < 0;
			}, [this, inFlightIndex] {
				return mBoneTransformBuffers[inFlightIndex]->fill(mBoneTransforms.data(), 0);
			}),

			sync::global_memory_barrier(stage::all_commands >> stage::all_commands, access::memory_write >> access::memory_write | access::memory_read),

			avk::command::render_pass(mBackgroundPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
				avk::command::bind_pipeline(mBackgroundPipeline.as_reference()),
				avk::command::draw(6u, 1u, 0u, 0u)
			}),
			command::conditional([this] {
				return this->mPipelineID.first >= 0 && mExecutionData.mFrameWait < 0;
			}, [this, inFlightIndex] {
				return mPipelines[mPipelineID.first]->render(inFlightIndex);
			}),

			gpu_frame_timer->stopAction(inFlightIndexU32),

		}).into_command_buffer(cmdBfr).then_submit_to(*mQueue);

	mTimer->stop_timer("cpu_frame");

		// Do not start to render before the image has become available:
	submissionData.waiting_for(imageAvailableSemaphore >> stage::color_attachment_output)
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
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->destroy();
		load(mExecutionData.mNextFileName);
		getCurrentMeshletBuilder()->generate();	// regenerate Meshlets
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->initialize(mQueue);
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_PIPELINE) {
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->destroy();
		mPipelineID.first = mPipelineID.second;
		mPipelines[mPipelineID.first]->initialize(mQueue);
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_MESHLET_BUILDER) {
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->destroy();
		getCurrentMeshletBuilder()->destroy();
		mMeshletBuilderID.first = mMeshletBuilderID.second;
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->initialize(mQueue);
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_VERTEX_COMPRESSOR) {
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->destroy();
		getCurrentVertexCompressor()->destroy();
		mVertexCompressorID.first = mVertexCompressorID.second;
		if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->initialize(mQueue);
	}
}

void MeshletsApp::compileAndLoadNextPipeline()
{
	bool withoutError = false;

	try {
		mPipelines[mPipelineID.second]->compile();
		withoutError = true;
	}
	catch (const std::exception& e) {
		mShowErrorMessage = e.what();
	}
	if (withoutError) {
		freeCommandBufferAndExecute({
			.type = FreeCMDBufferExecutionData::CHANGE_PIPELINE
			});
	}
}
