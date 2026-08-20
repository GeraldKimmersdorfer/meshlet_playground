/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

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
#include "vertexcompressor/DPDifferenceCodec16.h"
#include "vertexcompressor/PermutationCodingCompression.h"
#include "vertexcompressor/PDifferenceCodec16.h"
#include "vertexcompressor/PDifferenceCodec8.h"
#include "vertexcompressor/PDifferenceCodec32.h"
#include "vertexcompressor/QuickPermutationCoding.h"
#include "vertexcompressor/OptimalSimplexCoding.h"

#include <functional>

#include <glm/gtx/string_cast.hpp>
#include "shadercompiler/ShaderMetaCompiler.h"
#include "helpers/hud.h"
#include "helpers/log.h"
#include "helpers/util.h"

#include "statistics/NumberProperty.h"
#include "statistics/StringProperty.h"

#include "../meshoptimizer/src/meshoptimizer.h"

std::vector<glm::mat4> globalTransformPresets = {
	glm::mat4(1.0),				// none
	 glm::rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f)) * glm::scale(glm::vec3(0.01f))	// lucy
};
const char* transformPresetsNames = "None\0Lucy\0";
int selectedGlobalTransformPresetId = 0;
bool optimizeMeshUsingMeshoptimizer = false;

void openDialogOptionPane(const char* vFilter, IGFDUserDatas vUserDatas, bool* vCantContinue)
{
	ImGui::Text("IMPORT OPTIONS");
	ImGui::Separator();
	ImGui::Combo("Global transform", (int*)(void*)&selectedGlobalTransformPresetId, transformPresetsNames);
	ImGui::Checkbox("Optimize Mesh using Meshoptimizer", &optimizeMeshUsingMeshoptimizer);
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
	mExtendedMeshData.clear();
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
	for (int i = 0; i < positions.size(); i++) {
		positions[i] = (positions[i] - aabbMin) / aabbSize;
	}
	invTranslation = glm::vec4(aabbMin, 1.0);
	invScale = glm::vec4(aabbSize, 1.0);
}

void normalizeTexCoords(std::vector<glm::vec2>& texCoords, glm::vec4& invTranslationScale) {
	glm::vec2 texMin(FLT_MAX); glm::vec2 texMax(FLT_MIN);
	for (int i = 0; i < texCoords.size(); i++) {
		texMin = glm::min(texMin, texCoords[i]);
		texMax = glm::max(texMax, texCoords[i]);
	}
	glm::vec2 aabrSize = texMax - texMin;
	for (int i = 0; i < texCoords.size(); i++) {
		texCoords[i] = (texCoords[i] - texMin) / aabrSize;
	}
	invTranslationScale = glm::vec4(texMin, aabrSize);
}

void normalizeBoneWeights(std::vector<glm::vec4>& meshBoneWeights, float epsilon = BONE_WEIGHT_EPSILON) {
	for (auto& weights : meshBoneWeights) {
		float weightSum = 0.0f;

		// Apply epsilon threshold and calculate the weight sum.
		for (int i = 0; i < 4; ++i) {
			if (weights[i] < epsilon) {
				weights[i] = 0.0f;
			}
			else {
				weightSum += weights[i];
			}
		}

		// Normalize the bone weights.
		if (weightSum > 0.0f) {
			for (int i = 0; i < 4; ++i) {
				weights[i] /= weightSum;
			}
		}
	}
}

// This function is supposed to optimize the mesh using meshoptimizer. it applies
// indexing, vertex cache optimization, overdraw optimization, vertex fetch optimization
// on the given mesh data.
void optimizeMesh(std::vector<vertex_data>& vertexData, std::vector<uint32_t>& indices) {
	// Step 1: Generate a vertex remap table
	std::vector<uint32_t> remap(vertexData.size());
	size_t vertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertexData.data(), vertexData.size(), sizeof(vertex_data));

	// Step 2: Remap vertices based on generated remap table
	std::vector<vertex_data> remappedVertices(vertexCount);
	meshopt_remapVertexBuffer(remappedVertices.data(), vertexData.data(), vertexData.size(), sizeof(vertex_data), remap.data());

	// Step 3: Remap indices
	std::vector<uint32_t> remappedIndices(indices.size());
	meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());

	// Step 4: Optimize vertex cache
	meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), vertexCount);

	// Step 5: Optimize overdraw
	meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].mPositionTxX.x, vertexCount, sizeof(vertex_data), 1.05f);

	// Step 6: Optimize vertex fetch
	meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.data(), vertexCount, sizeof(vertex_data));

	// Update the input vertex and index data with optimized data
	vertexData = std::move(remappedVertices);
	indices = std::move(remappedIndices);
}


void MeshletsApp::load(const std::string& filename)
{
	reset();
	// Load camera definitions
	readCameraDefinitionsFromFile(mCameraDefinitions);

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

		auto& emesh = mExtendedMeshData.emplace_back(extended_mesh_data{});
		emesh.vertexOffset = static_cast<uint32_t>(mVertexData.size());
		emesh.indexOffset = static_cast<uint32_t>(mIndices.size());
		auto& mesh = mMeshData.emplace_back(mesh_data{});
		mesh.transformationMatrix = globalTransform * model->transformation_matrix_for_mesh(meshIndex);
		mesh.materialIndex = 0;
		mesh.animated = static_cast<int32_t>(amesh->HasBones());

		// Find and assign the correct material in the allMatConfigs vector
		for (auto pair : distinctMaterials) {
			if (std::end(pair.second) != std::ranges::find(pair.second, meshIndex)) break;
			mesh.materialIndex++;
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
		normalizePositions(meshPositions, mesh.positionTranslation, mesh.positionScale);

		// Same thing as above, but for texture coordinates
		normalizeTexCoords(meshTexCoords, mesh.texCoordsTranslationScale);

		// "NORMALIZE" bone weights, meaning there are a lot of bone weights that don't add up to one.
		// I don't know where that is coming from, but i intend to fix this in the following lines of code
		// which stretches the weights that are > BONE_WEIGHT_EPSILON in regards of their weight.
		normalizeBoneWeights(meshBoneWeights, BONE_WEIGHT_EPSILON);

		emesh.indexCount = meshIndices.size();
		emesh.vertexCount = meshPositions.size();

		std::vector<vertex_data> meshVertexData;
		meshVertexData.reserve(meshPositions.size());
		for (int i = 0; i < meshPositions.size(); i++) {
			auto& vd = meshVertexData.emplace_back(vertex_data{
				.mPositionTxX = glm::vec4(meshPositions[i], meshTexCoords[i].x),
				.mTxYNormal = glm::vec4(meshTexCoords[i].y, meshNormals[i]),
				.mBoneIndices = meshBoneIndices[i],
				.mBoneWeights = meshBoneWeights[i]
				});
		}

		if (optimizeMeshUsingMeshoptimizer) {
			optimizeMesh(meshVertexData, meshIndices);
		}

		mIndices.insert(mIndices.end(), meshIndices.begin(), meshIndices.end());
		mVertexData.insert(mVertexData.end(), meshVertexData.begin(), meshVertexData.end());
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

	evaluateModelProperties(filename);
	//if (mAnimations.size() > 0) mCurrentlyPlayingAnimationId = 0;
}

void MeshletsApp::initGUI()
{
	auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();

	if (nullptr != imguiManager) {
		imguiManager->add_callback([
			this, imguiManager
		]() mutable {
				if (!mShowGUI) return;
				bool config_has_changed = false;
				ImGuiIO& io = ImGui::GetIO();

				// ================ MAIN MENU ======================
				ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
				ImGui::SetWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
				ImGui::SetWindowSize({ -1, io.DisplaySize.y }, ImGuiCond_Always);

				if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open File", ImVec2(ImGui::GetWindowSize().x * 0.96, 0.0f))) {
					IGFD::FileDialogConfig openFileDialogConfig;
					openFileDialogConfig.path = getBestAvailableAssetFolder();
					openFileDialogConfig.fileName = "";
					openFileDialogConfig.countSelectionMax = 1;
					openFileDialogConfig.userDatas = (IGFDUserDatas)nullptr;
					openFileDialogConfig.flags = ImGuiFileDialogFlags_Modal;
					openFileDialogConfig.sidePane = std::bind(&openDialogOptionPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
					openFileDialogConfig.sidePaneWidth = 300.0f;
					ImGuiFileDialog::Instance()->OpenDialog("open_file", "Choose File", "{.fbx,.obj,.dae,.ply,.gltf,.glb}", openFileDialogConfig);
				}

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
					ImGui::SetNextItemWidth(200);
					const float step = 0.05f; // Your desired step size
					if (ImGui::SliderFloat("##AnimationProgress", &mCurrentAnimationProgress, 0.0f, 1.0f, "%.3f")) {
						mCurrentAnimationProgress = roundf(mCurrentAnimationProgress / step) * step;
						mCurrentAnimationProgressChanged = true;
					}
					//if (ImGui::SliderFloat("##AnimationProgress", &mCurrentAnimationProgress, 0.0f, 1.0f)) mCurrentAnimationProgressChanged = true;
					ImGui::SameLine();
					if (ImGui::Button(mCurrentAnimationPaused ? ICON_FA_PLAY : ICON_FA_PAUSE)) mCurrentAnimationPaused = !mCurrentAnimationPaused;
					ImGui::Checkbox("Inverse Mesh Root Fix", &mInverseMeshRootFix);
				}


				bool quakeCamEnabled = mQuakeCam.is_enabled();
				if (ImGui::CollapsingHeader(ICON_FA_CAMERA " Camera", ImGuiTreeNodeFlags_Leaf)) {
					if (ImGui::Checkbox("Enable Quake Camera [F5]", &quakeCamEnabled)) {
						if (quakeCamEnabled) { // => should be enabled
							mQuakeCam.set_matrix(mOrbitCam.matrix()); mQuakeCam.enable(); mOrbitCam.disable();
						}
					}
					static char mCameraName[30] = "New Camera";
					ImGui::SetNextItemWidth(100);
					ImGui::InputText("##cameraname", mCameraName, 30);
					ImGui::SameLine();
					if (ImGui::Button(ICON_FA_SAVE " Save Camera")) {
						auto proj = mQuakeCam.projection_matrix();
						auto view = mQuakeCam.matrix();
						if (!quakeCamEnabled) {
							proj = mOrbitCam.projection_matrix();
							view = mOrbitCam.matrix();
						}
						mCameraDefinitions[mCameraName] = { mCameraName, view, proj };
						saveCameraDefinitionsToFile(mCameraDefinitions);
					}

					static std::string currentCameraName = "Default";
					if (ImGui::BeginCombo("Select Camera", currentCameraName.c_str())) {
						for (const auto& cameraDef : mCameraDefinitions) {
							bool isSelected = (currentCameraName == cameraDef.first);
							if (ImGui::Selectable(cameraDef.first.c_str(), isSelected)) {
								currentCameraName = cameraDef.first;
								// Load camera view and projection matrices
								if (quakeCamEnabled) {
									mQuakeCam.set_matrix(cameraDef.second.mViewMatrix);
									mQuakeCam.set_projection_matrix(cameraDef.second.mProjectionMatrix);
								}
								else {
									mOrbitCam.set_matrix(cameraDef.second.mViewMatrix);
									mOrbitCam.set_projection_matrix(cameraDef.second.mProjectionMatrix);
								}
							}
							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
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

				if (ImGui::CollapsingHeader(ICON_FA_COG " Shared Configuration", ImGuiTreeNodeFlags_Leaf)) {
					hudSharedConfiguration(config_has_changed);
				}

				if (ImGui::CollapsingHeader(ICON_FA_CUBES " Meshlet-Building", ImGuiTreeNodeFlags_Leaf)) {
					bool highlight = mMeshletBuilderID.first != mMeshletBuilderID.second;
					if (highlight) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0, 0.0, 0.0, 1.0));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, { 1.0f });
					}
					bool isOpen = ImGui::BeginCombo("Builder", mMeshletBuilder[mMeshletBuilderID.second]->getName().c_str());
					if (highlight) {
						ImGui::PopStyleColor(1);
						ImGui::PopStyleVar(1);
					}
					if (isOpen) {
						for (int n = 0; n < mMeshletBuilder.size(); n++) {
							bool is_selected = (mMeshletBuilderID.second == n);
							if (ImGui::Selectable(mMeshletBuilder[n]->getName().c_str(), is_selected)) {
								mMeshletBuilderID.second = n;
								//freeCommandBufferAndExecute({ .type = FreeCMDBufferExecutionData::CHANGE_MESHLET_BUILDER });
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				if (ImGui::CollapsingHeader(ICON_FA_FILE_ARCHIVE "  Vertex-Compression", ImGuiTreeNodeFlags_Leaf)) {
					bool highlight = mVertexCompressorID.first != mVertexCompressorID.second;
					if (highlight) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0, 0.0, 0.0, 1.0));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, { 1.0f });
					}
					bool isOpen = ImGui::BeginCombo("Compressor", mVertexCompressors[mVertexCompressorID.second]->getName().c_str());
					if (highlight) {
						ImGui::PopStyleColor(1);
						ImGui::PopStyleVar(1);
					}
					if (isOpen) {
						for (int n = 0; n < mVertexCompressors.size(); n++) {
							bool is_selected = (mVertexCompressorID.second == n);
							if (ImGui::Selectable(mVertexCompressors[n]->getName().c_str(), is_selected)) {
								mVertexCompressorID.second = n;
								//freeCommandBufferAndExecute({ .type = FreeCMDBufferExecutionData::CHANGE_VERTEX_COMPRESSOR });
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					mVertexCompressors[mVertexCompressorID.second]->hud_config(config_has_changed);
				}

				ImGui::Separator();
				if (ImGui::CollapsingHeader(ICON_FA_CODE_BRANCH " Rendering", ImGuiTreeNodeFlags_Leaf)) {
					bool highlight = mPipelineID.first != mPipelineID.second;
					if (highlight) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0, 0.0, 0.0, 1.0));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, { 1.0f });
					}
					bool isOpen = ImGui::BeginCombo("Pipeline", mPipelineID.second < 0 ? "Please select" : mPipelines[mPipelineID.second]->getName().c_str());
					if (highlight) {
						ImGui::PopStyleColor(1);
						ImGui::PopStyleVar(1);
					}

					if (isOpen) {
						for (int n = 0; n < mPipelines.size(); n++) {
							bool is_selected = (mPipelineID.second == n);
							if (ImGui::Selectable(mPipelines[n]->getName().c_str(), is_selected)) mPipelineID.second = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					mPipelines[mPipelineID.second]->hud_setup(config_has_changed);

					if (ImGui::Checkbox("Discard all fragments", (bool*)(void*)&mConfig.discardAllFragments)) {
						config_has_changed = true;
					}
				}

				bool highlight = mPipelineID.first != mPipelineID.second || mVertexCompressorID.first != mVertexCompressorID.second || mMeshletBuilderID.first != mMeshletBuilderID.second;
				if (highlight) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8, 0.4, 0.4, 1.0));
				if (ImGui::Button("Compile & Load [F1]", ImVec2(ImGui::GetWindowSize().x * 0.96, 0.0f))) compileAndLoadNextPipeline();
				if (highlight) ImGui::PopStyleColor(1);

				if (avk::input().key_pressed(avk::key_code::f1)) compileAndLoadNextPipeline();

				if (ImGui::CollapsingHeader("Pipeline-Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					if (mPipelineID.first >= 0) mPipelines[mPipelineID.first]->hud_config(config_has_changed);
				}

				ImGui::End();

				// ================ STATS WINDOW ======================

				ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_NoMove);
				ImGui::SetWindowSize({ 380, -1 }, ImGuiCond_Always);
				ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - ImGui::GetWindowWidth(), 0.0f), ImGuiCond_Always);
				ImGui::Text("%.3f ms/frame", 1000.0f / io.Framerate);
				ImGui::Text("%.1f FPS", io.Framerate);
				static bool vSyncEnable = mVSyncEnabled;
				if (ImGui::Checkbox("VSync (FIFO Presentation Mode)", &vSyncEnable)) setVSync(vSyncEnable);

				ImGui::Separator();

				{
					// === PROPERTIES ===
					auto rootProps = mPropertyManager->getRootProperties();

					const float checkboxScale = 0.5f; // Scale factor for checkboxes

					if (rootProps.size() > 0) {
						std::stack<std::pair<std::shared_ptr<PropertyInterface>, int>> stack;
						for (const auto& root : rootProps) {
							stack.push({ root, 0 });
						}

						while (!stack.empty()) {
							auto [current, level] = stack.top();
							stack.pop();

							if (level > 0) ImGui::Indent(level * ImGui::GetStyle().IndentSpacing);
							if (current->getChildren().empty()) {
								bool isSelected = std::find(mSelectedProperties.begin(), mSelectedProperties.end(), current->getName()) != mSelectedProperties.end();

								// Save the current font size and scale down for the checkbox
								ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(checkboxScale * ImGui::GetStyle().FramePadding.x, checkboxScale * ImGui::GetStyle().FramePadding.y));

								// Render the scaled checkbox
								if (ImGui::Checkbox(current->getFormattedName().c_str(), &isSelected)) {
									if (isSelected) {
										mSelectedProperties.push_back(current->getName());
									}
									else {
										mSelectedProperties.erase(std::remove(mSelectedProperties.begin(), mSelectedProperties.end(), current->getName()), mSelectedProperties.end());
									}
								}

								// Revert the font size and style
								ImGui::PopStyleVar();
								ImGui::PopFont();

								ImGui::SameLine();
								ImGui::Text("%s", current->getValueAsFormattedString().c_str());
							}
							else {
								if (ImGui::CollapsingHeader(current->getFormattedName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
									for (const auto& child : current->getChildren()) {
										stack.push({ child, level + 1 });
									}
								}
							}
							if (level > 0) ImGui::Unindent(level * ImGui::GetStyle().IndentSpacing);
						}
					}
					else {
						ImGui::TextColored(ImVec4(1.0f, .0f, .0f, 1.f), "No property defined");
					}

					if (ImGui::Button("Reset Timer", ImVec2(200, 0))) resetTimer();
					if (ImGui::Button("Report Properties", ImVec2(200, 0))) reportProperties();

				}



				if (ImGui::CollapsingHeader(ICON_FA_VIALS " Benchmark", ImGuiTreeNodeFlags_DefaultOpen)) {

					ImGui::Checkbox("Discard Fragments", &mBenchmarkSettings.discardFragments);
					ImGui::Checkbox("Hide GUI", &mBenchmarkSettings.hideGui);
					ImGui::Checkbox("Reset Timer", &mBenchmarkSettings.resetTimer);
					ImGui::Checkbox("Disable VSync", &mBenchmarkSettings.disableVSync);
					ImGui::InputInt("Frames to Render", &mBenchmarkSettings.frameCount);
					ImGui::InputInt("Copy Count", &mBenchmarkSettings.copyCount);
					if (ImGui::Button(ICON_FA_RECORD_VINYL " Start Benchmark", ImVec2(200, 0))) {
						startBenchmark();
						config_has_changed = true;
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
	//mQuakeCam.set_translation({ 0.0f, 0.0f, 5.0f });
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

		//LOG_F(INFO, std::format("Max. preferred task threads is {}, mesh threads is {}, subgroup size is {}.", mPropsMeshShader.maxPreferredTaskWorkGroupInvocations, mPropsMeshShader.maxPreferredMeshWorkGroupInvocations, mPropsSubgroup.subgroupSize));
		//LOG_F(INFO, std::format("This device supports the following subgroup operations: {}", vk::to_string(mPropsSubgroup.supportedOperations)));
		//LOG_F(INFO, std::format("This device supports subgroup operations in the following stages: {}", vk::to_string(mPropsSubgroup.supportedStages)));
		mTaskInvocationsExt = mPropsMeshShader.maxPreferredTaskWorkGroupInvocations;


		const auto uintFormatter = [](const unsigned int& value) { return siFormatter<float, 2, 1.0f, 1000>(value); };
		const auto byteFormatter = [](const float& value) { return siFormatter<float, 2, 0.5f, 1024>(value, "B"); };
		const auto secondsFormatter = [](const float& value) { return siFormatter<float, 2, 0.5f, 1000>(value, "s"); };
		const auto generalFloatFormatter = [](const float& value) { return std::format("{:.2f}", value); };

		// ===== PROPERTIES AND TIMING =====
		const auto propGroupFile = std::make_shared<PropertyGroup>("file_info");
		propGroupFile->addChild(std::make_shared<StringProperty>("file_name"));
		propGroupFile->addChild(std::make_shared<NumberProperty<float>>("file_size", byteFormatter));
		propGroupFile->addChild(std::make_shared<NumberProperty<unsigned int>>("meshes", uintFormatter));
		propGroupFile->addChild(std::make_shared<NumberProperty<unsigned int>>("vertices", uintFormatter));
		propGroupFile->addChild(std::make_shared<NumberProperty<unsigned int>>("faces", uintFormatter));
		propGroupFile->addChild(std::make_shared<NumberProperty<unsigned int>>("bones", uintFormatter));
		propGroupFile->addChild(std::make_shared<NumberProperty<float>>("avg_bones_pv", generalFloatFormatter));
		mPropertyManager->add_property(propGroupFile);

		const auto propGroupBuffer = std::make_shared<PropertyGroup>("buffer_info");
		// set by vertexcompressor
		propGroupBuffer->addChild(std::make_shared<NumberProperty<unsigned int>>("lut_count", uintFormatter));
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("lut_size", byteFormatter));
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("vb_size", byteFormatter));
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("emb_size", byteFormatter));

		// set by pipeline
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("ib_size", byteFormatter));

		// set by meshletbuilder
		propGroupBuffer->addChild(std::make_shared<NumberProperty<unsigned int>>("meshlets", uintFormatter));
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("mb_size", byteFormatter));
		propGroupBuffer->addChild(std::make_shared<NumberProperty<float>>("mb_redirect_size", byteFormatter));

		mPropertyManager->add_property(propGroupBuffer);

		const auto propGroupTiming = std::make_shared<PropertyGroup>("timing");
		mCpuFrameProperty = std::make_shared<AverageNumberProperty<float>>("cpu_frame", 240, secondsFormatter);
		propGroupTiming->addChild(mCpuFrameProperty);
		mAvkFrameProperty = std::make_shared<AverageNumberProperty<float>>("gpu_frame", 240, secondsFormatter);
		propGroupTiming->addChild(mAvkFrameProperty);
		mPropertyManager->add_property(propGroupTiming);

		mAvkFrameTimer = std::make_unique<AvkTimer>(std::move(mPropertyManager->getShared("gpu_frame")));
		mCpuFrameTimer = std::make_unique<CpuTimer>(std::move(mPropertyManager->getShared("cpu_frame")));

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
	//mMeshletBuilder.push_back(std::make_unique<BoneLUTDependentBuilder>(this)); NOT DONE

	mVertexCompressors.push_back(std::make_unique<NoCompression>(this));
	mVertexCompressors.push_back(std::make_unique<BoneLUTCompression>(this));
	mVertexCompressors.push_back(std::make_unique<PermutationCodingCompression>(this));
	mVertexCompressors.push_back(std::make_unique<QuickPermutationCoding>(this));
	mVertexCompressors.push_back(std::make_unique<PDifferenceCodec8>(this));
	mVertexCompressors.push_back(std::make_unique<PDifferenceCodec16>(this));
	mVertexCompressors.push_back(std::make_unique<PDifferenceCodec32>(this));
	//mVertexCompressors.push_back(std::make_unique<MeshletRiggedCompression>(this)); NOT DONE
	mVertexCompressors.push_back(std::make_unique<DPDifferenceCodec16>(this));

	mVertexCompressors.push_back(std::make_unique<OptimalSimplexCoding>(this));

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

	if (mCurrentlyPlayingAnimationId >= 0 && (mCurrentAnimationPaused == false || mCurrentAnimationProgressChanged == true)) {
		auto& aData = mAnimations[mCurrentlyPlayingAnimationId];
		auto& animation = aData.mAnimation;
		auto& clip = aData.mClip;
		const auto dt = time().delta_time() / aData.mDurationSeconds;
		mCurrentAnimationProgress += dt;
		if (mCurrentAnimationProgress > 1.0) mCurrentAnimationProgress = 1.0 - mCurrentAnimationProgress;
		auto time = aData.mDurationSeconds * mCurrentAnimationProgress;
		mCurrentAnimationProgressChanged = false;
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
}

void MeshletsApp::render()
{
	//if (mExecutionData.mFrameWait >= 0) return;	// We want to free the commandPool such that we can load a new file
	//if (mPipelineID.first < 0) return;	// No pipeline selected
	using namespace avk;

	if (mBenchmarkRestFrameCounter-- == 0) endBenchmark();

	mCpuFrameTimer->start();

	auto mainWnd = context().main_window();
	auto inFlightIndex = mainWnd->current_in_flight_index();


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

	auto submissionData = context().record({

			mAvkFrameTimer->start(inFlightIndexU32),

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

			mAvkFrameTimer->stop(inFlightIndexU32),

		}).into_command_buffer(cmdBfr).then_submit_to(*mQueue);

	mCpuFrameTimer->stop();

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

		uint32_t oldPipelineID = mPipelineID.first;
		uint32_t oldMeshletBuilderID = mMeshletBuilderID.first;
		uint32_t oldVertexCompressorID = mVertexCompressorID.first;

		mPipelineID.first = mPipelineID.second;
		getCurrentMeshletBuilder()->destroy();
		getCurrentVertexCompressor()->destroy();
		mMeshletBuilderID.first = mMeshletBuilderID.second;
		mVertexCompressorID.first = mVertexCompressorID.second;
		try {
			mPipelines[mPipelineID.first]->initialize(mQueue);
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			mShowErrorMessage = e.what();
			mMeshletBuilderID.first = oldMeshletBuilderID;
			mVertexCompressorID.first = oldVertexCompressorID;
			mPipelines[oldPipelineID]->initialize(mQueue);
		}
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_MESHLET_BUILDER) {
		throw std::runtime_error("Not implemented anymore");
	}
	else if (mExecutionData.type == FreeCMDBufferExecutionData::CHANGE_VERTEX_COMPRESSOR) {
		throw std::runtime_error("Not implemented anymore");
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

void MeshletsApp::setVSync(bool enable)
{
	if (mVSyncEnabled == enable) return;
	mVSyncEnabled = enable;
	if (mVSyncEnabled) avk::context().main_window()->set_presentaton_mode(avk::presentation_mode::fifo);
	else avk::context().main_window()->set_presentaton_mode(avk::presentation_mode::mailbox);
}

void MeshletsApp::reportProperties()
{
	for (const auto& propName : mSelectedProperties) {
		auto prop = mPropertyManager->get(propName);
		if (prop) {
			const std::string value = prop->getValueAsString();
			if (mSelectedProperties.size() == 1) {
				setClipboardText(prop->getValueAsString());
				LOG_S(INFO) << prop->getName() << ": " << value << " (copied to clipboard)";
			}
			else {
				LOG_S(INFO) << prop->getName() << ": " << value;
			}
		}
	}
}

void MeshletsApp::resetTimer()
{
	mAvkFrameProperty->reset();
	mCpuFrameProperty->reset();
}

void MeshletsApp::startBenchmark()
{
	const auto& conf = mBenchmarkSettings;
	auto& confSetback = mBenchmarkSettingsSetback;
	if (conf.disableVSync) {
		confSetback.disableVSync = mVSyncEnabled;
		setVSync(false);
	}
	if (conf.hideGui) {
		confSetback.hideGui = mShowGUI;
		mShowGUI = false;
	}
	if (conf.discardFragments) {
		confSetback.discardFragments = mConfig.discardAllFragments;
		mConfig.discardAllFragments = true;
	}
	if (conf.resetTimer) resetTimer();
	confSetback.frameCount = mConfig.mCopyCount;
	mConfig.mCopyCount = conf.copyCount;
	mBenchmarkRestFrameCounter = mBenchmarkSettings.frameCount;
}

void MeshletsApp::endBenchmark()
{
	const auto& conf = mBenchmarkSettings;
	const auto& confSetback = mBenchmarkSettingsSetback;
	if (conf.disableVSync) setVSync(confSetback.disableVSync);
	if (conf.hideGui) mShowGUI = confSetback.hideGui;
	if (conf.discardFragments) mConfig.discardAllFragments = confSetback.discardFragments;
	mConfig.mCopyCount = confSetback.frameCount;
	reportProperties();
	uploadConfig();
}


std::string getFileName(const std::string& filePath) {
	// Find the last occurrence of the path separator
	size_t pos = filePath.find_last_of("\\/");
	if (pos != std::string::npos) {
		// Return the substring after the last path separator
		return filePath.substr(pos + 1);
	}
	// If no path separator is found, return the original string
	return filePath;
}

void MeshletsApp::evaluateModelProperties(const std::string& filename) {
	mPropertyManager->get("file_name")->setString(getFileName(filename));
	mPropertyManager->get("file_size")->setFloat((float)std::filesystem::file_size(filename));
	mPropertyManager->get("meshes")->setUint(mMeshData.size());
	mPropertyManager->get("vertices")->setUint(mVertexData.size());
	mPropertyManager->get("faces")->setUint(mIndices.size() / 3);
	mPropertyManager->get("bones")->setUint(mBoneTransforms.size());

	// Evaluate average bones per vertex
	float avgBonesPerVertex = 0.0f;
	{
		uint32_t sum = 0;
		for (const auto& vdata : mVertexData) for (int i = 0; i < 4; ++i) if (vdata.mBoneWeights[i] > BONE_WEIGHT_EPSILON) sum++;
		avgBonesPerVertex = (float)sum / (float)mVertexData.size();
	}
	mPropertyManager->get("avg_bones_pv")->setFloat(avgBonesPerVertex);
}
