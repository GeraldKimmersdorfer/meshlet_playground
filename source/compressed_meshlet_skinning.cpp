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


#define STARTUP_FILE "assets/two_objects_in_one.fbx"
#define USE_MESHOPTIMIZER 1
#define USE_CACHE 0

enum PipelineType {
	VERTEX_PIPELINE,
	MESH_PIPELINE,
	NVIDIA_MESH_PIPELINE
};

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

class compressed_meshlets_app : public avk::invokee
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
		uint32_t mOffsetPositions;	// Offset to first item in Positions Texel-Buffer
		uint32_t mOffsetTexCoords;	// Offset to first item in TexCoords Texel-Buffer
		uint32_t mOffsetNormals;	// Offset to first item in Normals Texel-Buffer
		uint32_t mOffsetIndices;	// Offset to first item in Indices Texel-Buffer (unused yet)
		uint32_t mMaterialIndex;
		glm::vec3 padding;
	};

	/** The meshlet we upload to the gpu with its additional data. */
	struct alignas(16) meshlet
	{
		uint32_t mMeshIndex;
		avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	compressed_meshlets_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	void load(const std::string& filename) {
		avk::model model = std::move(avk::model_t::load_from_file(filename, aiProcess_Triangulate | aiProcess_PreTransformVertices));
		std::vector<avk::material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<meshlet> meshletsGeometry;
		std::vector<mesh_data> meshData;
		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> indices;

		// get all the meshlet indices of the model
		const auto meshIndicesInOrder = model->select_all_meshes();
		auto distinctMaterials = model->distinct_material_configs();
		// add all the materials of the model
		for (auto& pair : distinctMaterials) allMatConfigs.push_back(pair.first);

		for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
			auto meshIndex = meshIndicesInOrder[mpos];
			std::string meshname = model->name_of_mesh(mpos);

			auto& mesh = meshData.emplace_back(mesh_data{
				.mTransformationMatrix = model->transformation_matrix_for_mesh(meshIndex),
				.mOffsetPositions = static_cast<uint32_t>(positions.size()),
				.mOffsetTexCoords = static_cast<uint32_t>(texCoords.size()),
				.mOffsetNormals = static_cast<uint32_t>(normals.size()),
				.mOffsetIndices = static_cast<uint32_t>(indices.size()),
				.mMaterialIndex = 0,
				});

			// Find and assign the correct material in the allMatConfigs vector
			for (auto pair : distinctMaterials) {
				if (std::end(pair.second) != std::ranges::find(pair.second, meshIndex)) break;
				mesh.mMaterialIndex++;
			}

			auto selection = avk::make_model_references_and_mesh_indices_selection(model, meshIndex);
			//std::vector<glm::vec3> meshPositions; std::vector<uint32_t> meshIndices;
			//std::tie(meshPositions, meshIndices) = avk::get_vertices_and_indices(selection);
			auto [meshPositions, meshIndices] = avk::get_vertices_and_indices(selection);
			auto meshNormals = avk::get_normals(selection);
			auto meshTexCoords = avk::get_2d_texture_coordinates(selection, 0);

			positions.insert(positions.end(), meshPositions.begin(), meshPositions.end());
			texCoords.insert(texCoords.end(), meshTexCoords.begin(), meshTexCoords.end());
			normals.insert(normals.end(), meshNormals.begin(), meshNormals.end());
			indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());

			// create selection for the meshlets
			auto meshletSelection = avk::make_models_and_mesh_indices_selection(model, meshIndex);

#if USE_MESHOPTIMIZER
			auto cpuMeshlets = avk::divide_into_meshlets(meshletSelection, meshlet_division_meshoptimizer, true, sNumVertices, sNumIndices - ((sNumIndices / 3) % 4) * 3);
#else
			auto cpuMeshlets = avk::divide_into_meshlets(meshletSelection, true, sNumVertices, sNumIndices);
#endif
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
		mPositionsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(positions).describe_only_member(positions[0], avk::content_description::position),
			avk::storage_buffer_meta::create_from_data(positions),
			avk::uniform_texel_buffer_meta::create_from_data(positions).describe_only_member(positions[0]) // just take the vec3 as it is
		);

		mNormalsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(normals),
			avk::storage_buffer_meta::create_from_data(normals),
			avk::uniform_texel_buffer_meta::create_from_data(normals).describe_only_member(normals[0]) // just take the vec3 as it is
		);

		mTexCoordsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoords),
			avk::storage_buffer_meta::create_from_data(texCoords),
			avk::uniform_texel_buffer_meta::create_from_data(texCoords).describe_only_member(texCoords[0]) // just take the vec2 as it is   
		);

		avk::context().record_and_submit_with_fence({
			mPositionsBuffer->fill(positions.data(), 0),
			mNormalsBuffer->fill(normals.data(), 0),
			mTexCoordsBuffer->fill(texCoords.data(), 0)
			}, *mQueue
		)->wait_until_signalled();

		mPositionsBufferView = avk::context().create_buffer_view(mPositionsBuffer);
		mNormalsBufferView = avk::context().create_buffer_view(mNormalsBuffer);
		mTexCoordsBufferView = avk::context().create_buffer_view(mTexCoordsBuffer);
		
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

		// ToDo doesnt need to be reset each load
		// One for each concurrent frame
		const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < concurrentFrames; ++i) {
			mViewProjBuffers.push_back(avk::context().create_buffer(
				avk::memory_usage::host_coherent, {},
				avk::uniform_buffer_meta::create_from_data(glm::mat4())
			));
		}

		mMaterialsBuffer = avk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		auto emptyCommand = mMaterialsBuffer->fill(gpuMaterials.data(), 0);

		mImageSamplers = std::move(imageSamplers);

		// Create our graphics mesh pipeline with the required configuration:
		auto createGraphicsMeshPipeline = [this](auto taskShader, auto meshShader, uint32_t taskInvocations, uint32_t meshInvocations) {
			return avk::context().create_graphics_pipeline_for(
				// Specify which shaders the pipeline consists of:
				avk::task_shader(taskShader)
				.set_specialization_constant(0, taskInvocations),
				avk::mesh_shader(meshShader)
				.set_specialization_constant(0, taskInvocations)
				.set_specialization_constant(1, meshInvocations),
				avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
				// Some further settings:
				avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
				avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
				// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth
				// attachment, which has been configured when creating the window. See main() function!
				avk::context().create_renderpass({
					avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),
					avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
					}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
				// The following define additional data which we'll pass to the pipeline:
				avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(push_constants) },
				avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
				avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
				avk::descriptor_binding(1, 0, mMaterialsBuffer),
				// texel buffers
				avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mPositionsBufferView})),
				avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mNormalsBufferView})),
				avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mTexCoordsBufferView})),
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

		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
			});

	}

	void initCamera() {
		mOrbitCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mOrbitCam.set_pivot_distance(3.0f);
		mQuakeCam.set_translation({ 0.0f, 0.0f, 5.0f });
		mOrbitCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		mQuakeCam.set_perspective_projection(glm::radians(45.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		avk::current_composition()->add_element(mOrbitCam);
		avk::current_composition()->add_element(mQuakeCam);
		mQuakeCam.disable();
	}

	void initGUI() {
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
						ImGuiFileDialog::Instance()->OpenDialog("open_file", "Choose File", "{.fbx,.obj}", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
					}
					// display
					if (ImGuiFileDialog::Instance()->Display("open_file"))
					{
						if (ImGuiFileDialog::Instance()->IsOk())
						{
							mNewFileName = ImGuiFileDialog::Instance()->GetFilePathName();
							mLoadNewFile = true;
						}
						ImGuiFileDialog::Instance()->Close();
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
					if (mNvPipelineSupport) pipelineOptions = "Vertex\0Mesh Shader\0Nvidia Mesh Shader";
					ImGui::Combo("Pipeline", (int*)(void*)&mSelectedPipeline, pipelineOptions);
					ImGui::Separator();

					// Select the range of meshlets to be rendered:
					ImGui::Checkbox("Highlight meshlets", &mHighlightMeshlets);
					ImGui::Text("Select meshlets to be rendered:");
					ImGui::DragIntRange2("Visible range", &mShowMeshletsFrom, &mShowMeshletsTo, 1, 0, static_cast<int>(mNumMeshlets));

					ImGui::End();
				});
		}
	}

	void initGPUQueryPools() {
		mTimestampPool = avk::context().create_query_pool_for_timestamp_queries(
			static_cast<uint32_t>(avk::context().main_window()->number_of_frames_in_flight()) * 2
		);

		mPipelineStatsPool = avk::context().create_query_pool_for_pipeline_statistics_queries(
			vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations | vk::QueryPipelineStatisticFlagBits::eMeshShaderInvocationsEXT | vk::QueryPipelineStatisticFlagBits::eTaskShaderInvocationsEXT,
			avk::context().main_window()->number_of_frames_in_flight()
		);
	}

	void loadDeviceProperties() {
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

	void initialize() override
	{
		this->loadDeviceProperties();
		this->load(STARTUP_FILE);
		this->initCamera();
		this->initGUI();
		this->initGPUQueryPools();
	}

	void update() override
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

	void render() override
	{
		if (mLoadNewFile) return;	// We want to free the commandPool such that we can load a new file
		using namespace avk;

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
		auto& pipeline = mSelectedPipeline == MESH_PIPELINE ? mPipelineExt : mSelectedPipeline == NVIDIA_MESH_PIPELINE ? mPipelineNv : mPipelineExt; // ToDo: Select vertex pipeline
		context().record({
				mPipelineStatsPool->reset(inFlightIndex, 1),
				mPipelineStatsPool->begin_query(inFlightIndex),
				mTimestampPool->reset(firstQueryIndex, 2),     // reset the two values relevant for the current frame in flight
				mTimestampPool->write_timestamp(firstQueryIndex + 0, stage::all_commands), // measure before drawMeshTasks*

				// Upload the updated bone matrices into the buffer for the current frame (considering that we have cConcurrentFrames-many concurrent frames):
mViewProjBuffers[inFlightIndex]->fill(glm::value_ptr(viewProjMat), 0),

				sync::global_memory_barrier(stage::all_commands >> stage::all_commands, access::memory_write >> access::memory_write | access::memory_read),

				command::render_pass(pipeline->renderpass_reference(), context().main_window()->current_backbuffer_reference(), {
					command::bind_pipeline(pipeline.as_reference()),
					command::bind_descriptors(pipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
						descriptor_binding(0, 1, mViewProjBuffers[inFlightIndex]),
						descriptor_binding(1, 0, mMaterialsBuffer),
						descriptor_binding(3, 0, as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mPositionsBufferView})),
						descriptor_binding(3, 2, as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mNormalsBufferView})),
						descriptor_binding(3, 3, as_uniform_texel_buffer_views(std::vector<avk::buffer_view>{mTexCoordsBufferView})),
						descriptor_binding(4, 0, mMeshletsBuffer),
						descriptor_binding(4, 1, mMeshesBuffer)
					})),

					command::push_constants(pipeline->layout(), push_constants{
						mHighlightMeshlets,
						static_cast<int32_t>(mShowMeshletsFrom),
						static_cast<int32_t>(mShowMeshletsTo)
					}),

					// Draw all the meshlets with just one single draw call:
					command::conditional(
						[this]() { return mSelectedPipeline == NVIDIA_MESH_PIPELINE; },
						[this]() { return command::draw_mesh_tasks_nv(div_ceil(mNumMeshlets, mTaskInvocationsNv), 0); },
						[this]() { return command::conditional(
							[this]() { return mSelectedPipeline == MESH_PIPELINE; },
							[this]() { return command::draw_mesh_tasks_ext(div_ceil(mNumMeshlets, mTaskInvocationsExt), 1, 1); },
							[this]() { return command::draw_mesh_tasks_ext(div_ceil(mNumMeshlets, mTaskInvocationsExt), 1, 1); } // ToDo replace with vertex pipeline draw
						); }
					)
					
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

private: // v== Member variables ==v

	bool mLoadNewFile = false;
	std::string mNewFileName;
	int mFrameWait = -1;

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<avk::buffer> mViewProjBuffers;
	avk::buffer mMaterialsBuffer;
	avk::buffer mMeshletsBuffer;
	avk::buffer mMeshesBuffer;
	avk::buffer mPositionsBuffer;
	avk::buffer mTexCoordsBuffer;
	avk::buffer mNormalsBuffer;
	// ToDo: We need index buffer for normal pipeline?

	std::vector<avk::image_sampler> mImageSamplers;
	avk::graphics_pipeline mPipelineExt;
	avk::graphics_pipeline mPipelineNv;

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;

	uint32_t mNumMeshlets;
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
	

}; // compressed_meshlets_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Static Meshlets");

		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			});
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = compressed_meshlets_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Static Meshlets"),
			// Gotta enable the mesh shader extension, ...
			avk::required_device_extensions(VK_EXT_MESH_SHADER_EXTENSION_NAME),
			avk::optional_device_extensions(VK_NV_MESH_SHADER_EXTENSION_NAME),
			// ... and enable the mesh shader features that we need:
			[](vk::PhysicalDeviceMeshShaderFeaturesEXT& meshShaderFeatures) {
				meshShaderFeatures.setMeshShader(VK_TRUE);
				meshShaderFeatures.setTaskShader(VK_TRUE);
				meshShaderFeatures.setMeshShaderQueries(VK_TRUE);
			},
			[](vk::PhysicalDeviceFeatures& features) {
				features.setPipelineStatisticsQuery(VK_TRUE);
			},
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setUniformAndStorageBuffer8BitAccess(VK_TRUE);
				features.setStorageBuffer8BitAccess(VK_TRUE);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app, ui
		);

		// Create an invoker object, which defines the way how invokees/elements are invoked
		// (In this case, just sequentially in their execution order):
		avk::sequential_invoker invoker;

		// With everything configured, let us start our render loop:
		composition.start_render_loop(
			// Callback in the case of update:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Call all the update() callbacks:
				invoker.invoke_updates(aToBeInvoked);
			},
			// Callback in the case of render:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Sync (wait for fences and so) per window BEFORE executing render callbacks
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->sync_before_render();
					});

				// Call all the render() callbacks:
				invoker.invoke_renders(aToBeInvoked);

				// Render per window:
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->render_frame();
					});
			}
		); // This is a blocking call, which loops until avk::current_composition()->stop(); has been called (see update())

		result = EXIT_SUCCESS;
	}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
	return result;
}
