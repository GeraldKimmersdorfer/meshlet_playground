#pragma once
#include "VertexCompressionInterface.h"

class MeshletRiggedCompression : public VertexCompressionInterface {


public:

	MeshletRiggedCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "Meshlet Rigged (16byte)", "_MLTR")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<vertex_data_meshlet_coding> mVertexData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	//bool mWithShuffle = false;

};