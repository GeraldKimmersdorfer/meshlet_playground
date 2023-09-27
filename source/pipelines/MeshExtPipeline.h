#pragma once

#include "PipelineInterface.h"
class SharedData;

class MeshExtPipeline : public PipelineInterface {

public:

	MeshExtPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud(bool& config_has_changed) override;

private:
	uint32_t mTaskInvocationsExt;

	std::vector<meshlet> mMeshlets;
	avk::buffer mMeshletsBuffer;

	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};