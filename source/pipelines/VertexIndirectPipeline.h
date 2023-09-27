#pragma once

#include "PipelineInterface.h"
class SharedData;

class VertexIndirectPipeline : public PipelineInterface {

public:

	VertexIndirectPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud(bool& config_has_changed) override;

private:
	avk::buffer mIndirectDrawCommandBuffer;
	avk::buffer mPositionsBuffer;
	avk::buffer mTexCoordsBuffer;
	avk::buffer mNormalsBuffer;
	avk::buffer mBoneIndicesBuffer;
	avk::buffer mBoneWeightsBuffer;
	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};