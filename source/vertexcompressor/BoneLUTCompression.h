#pragma once
#include "VertexCompressionInterface.h"

class BoneLUTCompression : public VertexCompressionInterface {


public:

	BoneLUTCompression(SharedData* shared)
		: VertexCompressionInterface("Bone LUT (128bit)", shared)
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;


private:
	std::vector<vertex_data_bone_lookup> mVertexData;
	std::vector<glm::uvec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	void createLUT();

};