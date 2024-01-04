#pragma once
#include "VertexCompressionInterface.h"

class PermutationCodingCompression : public VertexCompressionInterface {


public:

	PermutationCodingCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "Permutation Coding (24 byte)", "_PC")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<vertex_data_permutation_coding> mVertexData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	//bool mWithShuffle = false;

};