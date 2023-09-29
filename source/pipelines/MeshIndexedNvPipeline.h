#pragma once

#include "PipelineInterface.h"
class SharedData;

class MeshIndexedNvPipeline : public PipelineInterface {

public:

	MeshIndexedNvPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud(bool& config_has_changed) override;

private:
	uint32_t mTaskInvocationsNv;

	std::vector<meshlet_redirect> mMeshlets;
	std::vector<uint32_t> mPackedIndices;
	avk::buffer mMeshletsBuffer;
	avk::buffer mPackedIndexBuffer;

	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};