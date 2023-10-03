#pragma once

#include "PipelineInterface.h"
#include "mcc.h"
class SharedData;

class VertexIndirectPipeline : public PipelineInterface {

public:

	VertexIndirectPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud_config(bool& config_has_changed) override;

	void hud_setup(bool& config_has_changes) override;

	void compile() override;

private:
	avk::buffer mIndirectDrawCommandBuffer;
	avk::buffer mPositionsBuffer;
	avk::buffer mTexCoordsBuffer;
	avk::buffer mNormalsBuffer;
	avk::buffer mBoneIndicesBuffer;
	avk::buffer mBoneWeightsBuffer;
	avk::graphics_pipeline mPipeline;

	std::pair<MCC_VERTEX_GATHER_TYPE, MCC_VERTEX_GATHER_TYPE> mVertexGatherType = { _PUSH, _PUSH };	// first ... avtive, second ... selected

	std::string mPathVertexShader = "";
	std::string mPathFragmentShader = "";
	bool mShadersRecompiled = false;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};