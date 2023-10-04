#pragma once
#include "VertexCompressionInterface.h"

class NoCompression : public VertexCompressionInterface {


public:

	NoCompression(SharedData* shared)
		: VertexCompressionInterface("No compression", shared)
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;


private:
	avk::buffer mVertexBuffer;

};