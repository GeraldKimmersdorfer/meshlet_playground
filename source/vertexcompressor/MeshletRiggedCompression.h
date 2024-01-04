#pragma once
#include "VertexCompressionInterface.h"

class MeshletRiggedCompression : public VertexCompressionInterface {

	struct mrc_per_meshlet_data {
		glm::u16vec4 mMbiTable = glm::u16vec4(UINT16_MAX);	// 4 possible bone indices per meshlet
	};


public:

	MeshletRiggedCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "Meshlet Rigged (24 byte)", "_MLTR")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<vertex_data_meshlet_coding> mVertexData;
	std::vector<mrc_per_meshlet_data> mAdditionalMeshletData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;
	avk::buffer mAdditionalMeshletBuffer;

	bool mWithShuffle = false;
	bool mWithMerge = false;

};