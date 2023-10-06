#include <stdlib.h>
#include <configure_and_compose.hpp>
#include <imgui_manager.hpp>
#include <sequential_invoker.hpp>

#include "MeshletsApp.h"
/*
#include "thirdparty/permutation_coding.h"

unsigned int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}


void test_permutation_coding(const std::string& name, uint32_t payload, uint32_t pvc, uint32_t wvc, glm::vec4 inWeights = glm::vec4(0.1, 0.2, 0.2, 0.5)) {
	// SOME PERMUTATION CODING PLAYING AROUND:
	float* weights = &inWeights[0];
	uint32_t tuple_index = payload;
	auto codec = blend_attribute_codec_t{
		.entry_count = 3,
		.weight_value_count = wvc,
		.extra_value_counts = {1,1,1}
	};
	codec.payload_value_count_over_factorial = (unsigned int)ceil(pvc / factorial(codec.entry_count));
	auto code = compress_blend_attributes(weights, tuple_index, codec);

	int isValid; float weightsDecode[4]; uint32_t tupleDecode;
	tupleDecode = decompress_blend_attributes(weightsDecode, &isValid, code, codec);
	std::cout << std::endl << "====== TEST " << name << " ==============" << std::endl;
	std::cout << "Compressed:    " << code << (code <= UINT32_MAX ? " UINT32" : " UINT64") << std::endl;
	std::cout << "Original:      " << weights[0] << " | " << weights[1] << " | " << weights[2] << " | " << weights[3] << std::endl;
	std::cout << "Reconstructed: " << weightsDecode[0] << " | " << weightsDecode[1] << " | " << weightsDecode[2] << " | " << weightsDecode[3] << std::endl;
	std::cout << "Tuple Code:    " << (tupleDecode == tuple_index ? "OK " : "FALSCH ") << tupleDecode << " (Original: " << tuple_index << ")" << std::endl;
	std::cout << "Is Valid:      " << (isValid ? "True" : "False") << std::endl;
}*/

int main() // <== Starting point ==
{
	/*
	test_permutation_coding("Max8", UINT8_MAX, 2334, 19);
	test_permutation_coding("Max16", UINT16_MAX - 1, 2334, 19);
	test_permutation_coding("Max32", UINT32_MAX - 1, 2334, 19);

	test_permutation_coding("Max8", UINT8_MAX, 2334, 2 ^ 16);
	test_permutation_coding("Max16", UINT16_MAX - 1, 2334, 2 ^ 16);
	test_permutation_coding("Max32", UINT32_MAX - 1, 2334, 2 ^ 16);

	test_permutation_coding("Max8", UINT8_MAX, 0, 2 ^ 16);
	test_permutation_coding("UINT16_MAX", UINT16_MAX, 0, 2 ^ 16);
	test_permutation_coding("Max32", UINT32_MAX - 1, 0, 2 ^ 16);

	return EXIT_SUCCESS;*/

	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Skinned Meshlet Playground");

		mainWnd->set_resolution({ 1280, 800 });
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
		auto app = MeshletsApp(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Skinned Meshlet Playground"),
			// Gotta enable the mesh shader extension, ...
			avk::required_device_extensions(VK_EXT_MESH_SHADER_EXTENSION_NAME),
			avk::required_device_extensions(VK_EXT_MULTI_DRAW_EXTENSION_NAME),
			avk::optional_device_extensions(VK_NV_MESH_SHADER_EXTENSION_NAME),
			// ... and enable the mesh shader features that we need:
			[](vk::PhysicalDeviceMeshShaderFeaturesEXT& meshShaderFeatures) {
				meshShaderFeatures.setMeshShader(VK_TRUE);
				meshShaderFeatures.setTaskShader(VK_TRUE);
				meshShaderFeatures.setMeshShaderQueries(VK_TRUE);
			},
			[](vk::PhysicalDeviceFeatures& features) {
				features.setPipelineStatisticsQuery(VK_TRUE);
				features.setMultiDrawIndirect(VK_TRUE);
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