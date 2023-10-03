#pragma once

#include "PipelineInterface.h"
#include "mcc.h"
class SharedData;

class MeshPipeline : public PipelineInterface {

public:

	MeshPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud_config(bool& config_has_changed) override;

	void hud_setup(bool& config_has_changes) override;

	void compile() override;

private:
	uint32_t mTaskInvocations;
	uint32_t mMeshInvocations;

	std::vector<meshlet_native> mMeshletsNative;
	std::vector<meshlet_redirect> mMeshletsRedirect;
	avk::buffer mMeshletsBuffer;

	
	std::vector<uint32_t> mPackedIndices;
	avk::buffer mPackedIndexBuffer;

	std::pair<MCC_MESHLET_EXTENSION, MCC_MESHLET_EXTENSION> mMeshletExtension = { _NV, _NV };	// first ... avtive, second ... selected
	std::pair<MCC_MESHLET_TYPE, MCC_MESHLET_TYPE> mMeshletType = { _NATIVE, _NATIVE };			// first ... avtive, second ... selected

	std::string mPathTaskShader = "";
	std::string mPathMeshShader = "";
	std::string mPathFragmentShader = "";
	bool mShadersRecompiled = false;

	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};