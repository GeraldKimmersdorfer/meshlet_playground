#include "rigged_model.h"
#include <auto_vk_toolkit.hpp>

RiggedModel::RiggedModel(std::string& filename)
{
	auto model = avk::model_t::load_from_file(filename, aiProcess_Triangulate | aiProcess_FlipUVs);	// Its necessary to flip the normals!

	std::vector<avk::material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
	std::vector<loaded_data_for_draw_call> dataForDrawCall;
	std::vector<meshlet> meshletsGeometry;
	std::vector<animated_model_data> animatedModels;

	// load the animation
	auto curClip = model->load_animation_clip(0, 0, 6000); // if too large the actual end of the animation will be taken (why not the default??)
	curClip.mTicksPerSecond = 30;

	auto& curEntry = animatedModels.emplace_back();
	curEntry.mModelName = model->path();
	curEntry.mClip = curClip;

	// get all the meshlet indices of the model
	const auto meshIndicesInOrder = model->select_all_meshes();
	curEntry.mNumBoneMatrices = model->num_bone_matrices(meshIndicesInOrder);
	curEntry.mBoneMatricesBufferIndex = 0; // formerly i
}
