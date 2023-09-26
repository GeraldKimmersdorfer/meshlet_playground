#pragma once

#include "PipelineInterface.h"
class SharedData;

class VertexPulledIndirectNoCompressionPipeline : public PipelineInterface {

public:

	VertexPulledIndirectNoCompressionPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

private:
	avk::buffer mIndirectDrawCommandBuffer;
	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};