#pragma once

#include "PipelineInterface.h"
class SharedData;

class MeshNvPipeline : public PipelineInterface {

public:

	MeshNvPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud() override;

private:
	bool mHighlightMeshlets = true;
	int  mShowMeshletsFrom = 0;
	int  mShowMeshletsTo = 0;
	uint32_t mTaskInvocationsNv;

	std::vector<meshlet> mMeshlets;
	avk::buffer mMeshletsBuffer;

	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};