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

#include <functional>

std::vector<glm::mat4> globalTransformPresets = {
	glm::mat4(1.0),				// none
	 glm::rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f)) * glm::scale(glm::vec3(0.01f))	// lucy
};
const char* transformPresetsNames = "None\0Lucy\0";
MeshletInterpreter selectedMeshletInterpreter = MeshletInterpreter::MESHOPTIMIZER;
int selectedGlobalTransformPresetId = 0;


avk::command::action_type_command draw_indexed_indirect_nobind(const avk::buffer_t& aParametersBuffer, const avk::buffer_t& aIndexBuffer, uint32_t aNumberOfDraws, vk::DeviceSize aParametersOffset, uint32_t aParametersStride)
{
	using namespace avk::command;
	const auto& indexMeta = aIndexBuffer.template meta<avk::index_buffer_meta>();
	vk::IndexType indexType;
	switch (indexMeta.sizeof_one_element()) {
		case sizeof(uint16_t) : indexType = vk::IndexType::eUint16; break;
			case sizeof(uint32_t) : indexType = vk::IndexType::eUint32; break;
			default: AVK_LOG_ERROR("The given size[" + std::to_string(indexMeta.sizeof_one_element()) + "] does not correspond to a valid vk::IndexType"); break;
	}

	return action_type_command{
		avk::sync::sync_hint {
			{{ // DESTINATION dependencies for previous commands:
				vk::PipelineStageFlagBits2KHR::eAllGraphics,
				vk::AccessFlagBits2KHR::eInputAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
			}},
			{{ // SOURCE dependencies for subsequent commands:
				vk::PipelineStageFlagBits2KHR::eAllGraphics,
				vk::AccessFlagBits2KHR::eColorAttachmentWrite | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite
			}}
		},
		{}, // no resource-specific sync hints
		[
			indexType,
			lParametersBufferHandle = aParametersBuffer.handle(),
			lIndexBufferHandle = aIndexBuffer.handle(),
			aNumberOfDraws, aParametersOffset, aParametersStride
		](avk::command_buffer_t& cb) {
			cb.handle().bindIndexBuffer(lIndexBufferHandle, 0u, indexType);
			cb.handle().drawIndexedIndirect(lParametersBufferHandle, aParametersOffset, aNumberOfDraws, aParametersStride);
		}
	};
}

auto meshlet_division_meshoptimizer = [](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices, const avk::model_t& aModel, std::optional<avk::mesh_index_t> aMeshIndex, uint32_t aMaxVertices, uint32_t aMaxIndices) {
	// definitions
	size_t max_triangles = aMaxIndices / 3;
	const float cone_weight = 0.0f;

	// get the maximum number of meshlets that could be generated
	size_t max_meshlets = meshopt_buildMeshletsBound(aIndices.size(), aMaxVertices, max_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * aMaxVertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	// let meshoptimizer build the meshlets for us
	size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
		aIndices.data(), aIndices.size(), &tVertices[0].x, tVertices.size(), sizeof(glm::vec3),
		aMaxVertices, max_triangles, cone_weight);

	// copy the data over to Auto-Vk-Toolkit's meshlet structure
	std::vector<avk::meshlet> generatedMeshlets(meshlet_count);
	generatedMeshlets.resize(meshlet_count);
	generatedMeshlets.reserve(meshlet_count);
	for (int k = 0; k < meshlet_count; k++) {
		auto& m = meshlets[k];
		auto& gm = generatedMeshlets[k];
		gm.mIndexCount = m.triangle_count * 3;
		gm.mVertexCount = m.vertex_count;
		gm.mVertices.reserve(m.vertex_count);
		gm.mVertices.resize(m.vertex_count);
		gm.mIndices.reserve(gm.mIndexCount);
		gm.mIndices.resize(gm.mIndexCount);
		std::ranges::copy(meshlet_vertices.begin() + m.vertex_offset,
			meshlet_vertices.begin() + m.vertex_offset + m.vertex_count,
			gm.mVertices.begin());
		std::ranges::copy(meshlet_triangles.begin() + m.triangle_offset,
			meshlet_triangles.begin() + m.triangle_offset + gm.mIndexCount,
			gm.mIndices.begin());
	}
	return generatedMeshlets;
	};

void openDialogOptionPane(const char* vFilter, IGFDUserDatas vUserDatas, bool* vCantContinue)
{
	ImGui::Text("IMPORT OPTIONS");
	ImGui::Separator();
	ImGui::Combo("Meshlet builder", (int*)(void*)&selectedMeshletInterpreter, "AVK-Default\0Meshoptimizer\0");
	ImGui::Combo("Global transform", (int*)(void*)&selectedGlobalTransformPresetId, transformPresetsNames);
}

void MeshletsApp::load(const std::string& filename)
{
	avk::model model = std::move(avk::model_t::load_from_file(filename, aiProcess_Triangulate));
	std::vector<avk::material_config> allMatConfigs;
	std::vector<meshlet> meshletsGeometry;
	std::vector<mesh_data> meshData;
	std::vector<vertex_data> vertexData;
	std::vector<uint32_t> indices;
	mAnimations.clear(); mBoneTransformBuffers.clear(); 
	mBoneTransforms.clear(); mBoneTransformBuffers.clear();
	mImageSamplers.clear();
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

		auto& mesh = meshData.emplace_back(mesh_data{
			.mTransformationMatrix = globalTransform * model->transformation_matrix_for_mesh(meshIndex),
			.mVertexOffset = static_cast<uint32_t>(vertexData.size()),
			.mIndexOffset = static_cast<uint32_t>(indices.size()),
			.mMaterialIndex = 0,
			.mAnimated = static_cast<int32_t>(amesh->HasBones())
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
		if (meshPositions.size() != meshBoneIndices.size() || meshPositions.size() != meshTexCoords.size()) {
			LOG_WARNING("Model has a mesh with a different indices, weights and position count.");
		}
		if (meshPositions.size() != meshNormals.size() || meshPositions.size() != meshTexCoords.size()) {
			LOG_WARNING("Model has a mesh with a different texcoord, normal and position count.");
		}
		mesh.mIndexCount = meshIndices.size();

		for (int i = 0; i < meshPositions.size(); i++) {
			auto& vd = vertexData.emplace_back(vertex_data{ 
				.mPositionTxX = glm::vec4(meshPositions[i], meshTexCoords[i].x), 
				.mNormalTxY = glm::vec4(meshNormals[i], meshTexCoords[i].y),
			});
			if (mesh.mAnimated) {
				vd.mBoneIndices = meshBoneIndices[i];
				vd.mBoneWeights = meshBoneWeights[i];
			}
		}

		indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());

		// create selection for the meshlets
		auto meshletSelection = avk::make_models_and_mesh_indices_selection(model, meshIndex);

		std::vector<avk::meshlet> cpuMeshlets;
		if (selectedMeshletInterpreter == MESHOPTIMIZER) {
			cpuMeshlets = std::move(avk::divide_into_meshlets(meshletSelection, meshlet_division_meshoptimizer, true, sNumVertices, sNumIndices - ((sNumIndices / 3) % 4) * 3));
		}
		else if (selectedMeshletInterpreter == AVK_DEFAULT) {
			cpuMeshlets = std::move(avk::divide_into_meshlets(meshletSelection, true, sNumVertices, sNumIndices));
		}
#if USE_CACHE
		avk::serializer serializer("direct_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
		auto [gpuMeshlets, _] = avk::convert_for_gpu_usage_cached<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(serializer, cpuMeshlets);
		serializer.flush();
#else
		auto [gpuMeshlets, _] = avk::convert_for_gpu_usage<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(cpuMeshlets);
#endif

		for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
			auto& genMeshlet = gpuMeshlets[mshltidx];
			auto& ml = meshletsGeometry.emplace_back(meshlet{
				.mMeshIndex = static_cast<uint32_t>(mpos),
				.mGeometry = genMeshlet
				}
			);
		}
	} // end for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++)

	// Create a descriptor cache that helps us to conveniently create descriptor sets:
	mDescriptorCache = avk::context().create_descriptor_cache();
	mImageSamplers.clear(); mViewProjBuffers.clear();

	// ======== START UPLOADING TO GPU =============
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(vertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(vertexData.data(), 0) }, *mQueue)->wait_until_signalled();

	// buffers for the animated bone matrices, will be populated before rendering
	for (size_t cfi = 0; cfi < concurrentFrames; ++cfi) {
		mBoneTransformBuffers.push_back(avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_data(mBoneTransforms)
		));
	}

	// === FOR CLASSIC PIPELINE ===
	mIndexBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::index_buffer_meta::create_from_data(indices).describe_only_member(indices[0], avk::content_description::index),
		avk::storage_buffer_meta::create_from_data(indices)
	);
	avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(indices.data(), 0) }, *mQueue)->wait_until_signalled();
	auto gpuDrawCommands = std::vector<VkDrawIndexedIndirectCommand>(meshData.size());
	for (int i = 0; i < gpuDrawCommands.size(); i++) {
		gpuDrawCommands[i] = VkDrawIndexedIndirectCommand{
			.indexCount = meshData[i].mIndexCount,
			.instanceCount = 1,
			.firstIndex = meshData[i].mIndexOffset,
			.vertexOffset = static_cast<int32_t>(meshData[i].mVertexOffset),	// Note: Not strictly necessary, we could also set it to 0 and pull the vertex offset from the mesh buffer
			.firstInstance = static_cast<uint32_t>(i),							// we missuse that such that we know where to access the mesh array in the shader
		};
	}
	mIndirectDrawCommandBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::indirect_buffer_meta::create_from_data(gpuDrawCommands),
		avk::storage_buffer_meta::create_from_data(gpuDrawCommands)
	);
	avk::context().record_and_submit_with_fence({
			mIndirectDrawCommandBuffer->fill(gpuDrawCommands.data(), 0)
		}, *mQueue
	)->wait_until_signalled();
	// ======

	mMeshesBuffer = avk::context().create_buffer(
		avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(meshData)
	);
	avk::context().record_and_submit_with_fence({ mMeshesBuffer->fill(meshData.data(), 0), }, *mQueue)->wait_until_signalled();

	mMeshletsBuffer = avk::context().create_buffer(
		avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(meshletsGeometry)
	);
	mNumMeshlets = static_cast<uint32_t>(meshletsGeometry.size());
	mShowMeshletsTo = static_cast<int>(mNumMeshlets);
	mNumMeshes = static_cast<uint32_t>(meshData.size());

	// For all the different materials, transfer them in structs which are well
	// suited for GPU-usage (proper alignment, and containing only the relevant data),
	// also load all the referenced images from file and provide access to them
	// via samplers; It all happens in `ak::convert_for_gpu_usage`:
	auto [gpuMaterials, imageSamplers, matCommands] = avk::convert_for_gpu_usage<avk::material_gpu_data>(
		allMatConfigs, false, false,
		avk::image_usage::general_texture,
		avk::filter_mode::trilinear
	);

	avk::context().record_and_submit_with_fence({
		mMeshletsBuffer->fill(meshletsGeometry.data(), 0),
		matCommands
		}, *mQueue)->wait_until_signalled();

	mMaterialsBuffer = avk::context().create_buffer(
		avk::memory_usage::host_visible, {},
		avk::storage_buffer_meta::create_from_data(gpuMaterials)
	);
	auto emptyCommand = mMaterialsBuffer->fill(gpuMaterials.data(), 0);

	mImageSamplers = std::move(imageSamplers);

	
	for (int i = 0; i < concurrentFrames; ++i) {
		mViewProjBuffers.push_back(avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::uniform_buffer_meta::create_from_data(glm::mat4())
		));
	}

	// Create our graphics mesh pipeline with the required configuration:
	auto createGraphicsMeshPipeline = [this](auto taskShader, auto meshShader, uint32_t taskInvocations, uint32_t meshInvocations) {
		return avk::context().create_graphics_pipeline_for(
			avk::task_shader(taskShader)
			.set_specialization_constant(0, taskInvocations),
			avk::mesh_shader(meshShader)
			.set_specialization_constant(0, taskInvocations)
			.set_specialization_constant(1, meshInvocations),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			avk::context().create_renderpass({
				avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
				avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
				}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
			avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(push_constants) },
			avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
			avk::descriptor_binding(1, 0, mMaterialsBuffer),
			avk::descriptor_binding(2, 0, mBoneTransformBuffers[0]),
			avk::descriptor_binding(3, 0, mVertexBuffer),
			avk::descriptor_binding(4, 0, mMeshletsBuffer),
			avk::descriptor_binding(4, 1, mMeshesBuffer)
		);
	};

	mPipelineExt = createGraphicsMeshPipeline("shaders/meshlet.task", "shaders/meshlet.mesh", mPropsMeshShader.maxPreferredTaskWorkGroupInvocations, mPropsMeshShader.maxPreferredMeshWorkGroupInvocations);
	// we want to use an updater, so create one:
	mUpdater.emplace();
	mUpdater->on(avk::shader_files_changed_event(mPipelineExt.as_reference())).update(mPipelineExt);

	if (mNvPipelineSupport) {
		mTaskInvocationsNv = mPropsMeshShaderNV.maxTaskWorkGroupInvocations;
		mPipelineNv = createGraphicsMeshPipeline("shaders/meshlet.nv.task", "shaders/meshlet.nv.mesh", mPropsMeshShaderNV.maxTaskWorkGroupInvocations, mPropsMeshShaderNV.maxMeshWorkGroupInvocations);
		mUpdater->on(avk::shader_files_changed_event(mPipelineNv.as_reference())).update(mPipelineNv);
	}

	// === FOR VERTEX PIPELINE ===
	mPipelineVertex = avk::context().create_graphics_pipeline_for(
		avk::vertex_shader("shaders/transform_and_pass_pulled_pos_nrm_uv.vert"),
		avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
		avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
		avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
		avk::context().create_renderpass({
			avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
			avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
		avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(push_constants) },
		avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
		avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
		avk::descriptor_binding(1, 0, mMaterialsBuffer),
		avk::descriptor_binding(2, 0, mBoneTransformBuffers[0]),
		avk::descriptor_binding(3, 0, mVertexBuffer),
		avk::descriptor_binding(4, 0, mMeshletsBuffer), // Meshlet Buffer nicht notwendig, Performance implication?
		avk::descriptor_binding(4, 1, mMeshesBuffer)
	);
	mUpdater->on(avk::shader_files_changed_event(mPipelineVertex.as_reference())).update(mPipelineVertex);
	// ======

	mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
		this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});
}

void MeshletsApp::initCamera()
{
	mOrbitCam.set_translation({ 0.0f, 1.0f, 3.0f });
	mOrbitCam.set_pivot_distance(3.0f);
	mQuakeCam.set_translation({ 0.0f, 0.0f, 5.0f });
	mOrbitCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
	mQuakeCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
	avk::current_composition()->add_element(mOrbitCam);
	avk::current_composition()->add_element(mQuakeCam);
	mQuakeCam.disable();
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
				ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Separator();
				if (ImGui::Button("Open File")) {
					ImGuiFileDialog::Instance()->OpenDialogWithPane("open_file", "Choose File", "{.fbx,.obj,.dae,.ply}", ".", "", std::bind(&openDialogOptionPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 300.0, 1, (IGFDUserDatas)nullptr, ImGuiFileDialogFlags_Modal);
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
				auto pipelineOptions = "Vertex\0Mesh Shader\0";
				if (mNvPipelineSupport) pipelineOptions = "Vertex\0Mesh Shader\0Nvidia Mesh Shader\0";
				ImGui::Combo("Pipeline", (int*)(void*)&mSelectedPipeline, pipelineOptions);
				ImGui::Separator();

				// Select the range of meshlets to be rendered:
				ImGui::Checkbox("Highlight meshlets", &mHighlightMeshlets);
				ImGui::Text("Select meshlets to be rendered:");
				ImGui::DragIntRange2("Visible range", &mShowMeshletsFrom, &mShowMeshletsTo, 1, 0, static_cast<int>(mNumMeshlets));

				if (ImGuiFileDialog::Instance()->Display("open_file"))
				{
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						mNewFileName = ImGuiFileDialog::Instance()->GetFilePathName();
						mLoadNewFile = true;
					}
					ImGuiFileDialog::Instance()->Close();
				}

				ImGui::End();
			});
	}
}

void MeshletsApp::initGPUQueryPools()
{
	mTimestampPool = avk::context().create_query_pool_for_timestamp_queries(
		static_cast<uint32_t>(avk::context().main_window()->number_of_frames_in_flight()) * 2
	);

	mPipelineStatsPool = avk::context().create_query_pool_for_pipeline_statistics_queries(
		vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations | vk::QueryPipelineStatisticFlagBits::eMeshShaderInvocationsEXT | vk::QueryPipelineStatisticFlagBits::eTaskShaderInvocationsEXT,
		avk::context().main_window()->number_of_frames_in_flight()
	);
}

void MeshletsApp::loadDeviceProperties()
{
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
}

void MeshletsApp::initialize()
{
	this->loadDeviceProperties();
	this->load(STARTUP_FILE);
	this->initCamera();
	this->initGUI();
	this->initGPUQueryPools();
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
		// Stop the current composition:
		current_composition()->stop();
	}

	// After wanting to load a file, the following code waits for number_of_frames_in_flight, such
	// that all buffers/descriptors... are not in a queue and can safely be destroyed. (Is there a vk-toolkit way to do this?)
	if (mLoadNewFile) {
		if (mFrameWait == -1) mFrameWait = context().main_window()->number_of_frames_in_flight();
		else if (mFrameWait > 0) mFrameWait--;
		else if (mFrameWait == 0) {
			load(mNewFileName);
			mLoadNewFile = false;
			mFrameWait = -1;
		}
	}
}

void MeshletsApp::render()
{
	if (mLoadNewFile) return;	// We want to free the commandPool such that we can load a new file
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
	auto& pipeline = mSelectedPipeline == MESH_PIPELINE ? mPipelineExt : mSelectedPipeline == NVIDIA_MESH_PIPELINE ? mPipelineNv : mPipelineVertex;
	context().record({
			mPipelineStatsPool->reset(inFlightIndex, 1),
			mPipelineStatsPool->begin_query(inFlightIndex),
			mTimestampPool->reset(firstQueryIndex, 2),     // reset the two values relevant for the current frame in flight
			mTimestampPool->write_timestamp(firstQueryIndex + 0, stage::all_commands), // measure before drawMeshTasks*

			// Upload the updated bone matrices into the buffer for the current frame (considering that we have cConcurrentFrames-many concurrent frames):
			mViewProjBuffers[inFlightIndex]->fill(glm::value_ptr(viewProjMat), 0),
			mBoneTransformBuffers[inFlightIndex]->fill(mBoneTransforms.data(), 0),

			sync::global_memory_barrier(stage::all_commands >> stage::all_commands, access::memory_write >> access::memory_write | access::memory_read),

			command::render_pass(pipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
				command::bind_pipeline(pipeline.as_reference()),
				command::bind_descriptors(pipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
					descriptor_binding(0, 1, mViewProjBuffers[inFlightIndex]),
					descriptor_binding(1, 0, mMaterialsBuffer),
					descriptor_binding(2, 0, mBoneTransformBuffers[inFlightIndex]),
					descriptor_binding(3, 0, mVertexBuffer),
					descriptor_binding(4, 0, mMeshletsBuffer),
					descriptor_binding(4, 1, mMeshesBuffer)
				})),

				command::push_constants(pipeline->layout(), push_constants{
					mHighlightMeshlets,
					static_cast<int32_t>(mShowMeshletsFrom),
					static_cast<int32_t>(mShowMeshletsTo)
				}),

				// Draw all the meshlets with just one single draw call:
				command::custom_commands([this](avk::command_buffer_t& cb) {
					if (mSelectedPipeline == NVIDIA_MESH_PIPELINE) {
						cb.record(command::draw_mesh_tasks_nv(div_ceil(mNumMeshlets, mTaskInvocationsNv), 0));
					}
					else if (mSelectedPipeline == MESH_PIPELINE) {
						cb.record(command::draw_mesh_tasks_ext(div_ceil(mNumMeshlets, mTaskInvocationsExt), 1, 1));
					}
					else if (mSelectedPipeline == VERTEX_PIPELINE) {
						// Note: command::draw_indexed_indirect needs vertex buffer! Suggest to change that
						cb.record(draw_indexed_indirect_nobind(mIndirectDrawCommandBuffer.as_reference(), mIndexBuffer.as_reference(), mNumMeshes, 0, static_cast<uint32_t>(sizeof(VkDrawIndexedIndirectCommand))));
					}
				}),

			}),
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
