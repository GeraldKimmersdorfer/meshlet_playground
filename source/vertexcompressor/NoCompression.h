/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#include "VertexCompressionInterface.h"

class NoCompression : public VertexCompressionInterface {


public:

	NoCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "No compression", "_NOCOMP")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;


private:
	avk::buffer mVertexBuffer;

};