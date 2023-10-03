#pragma once

#include "MeshletbuilderInterface.h"

class MeshoptimizerBuilder : public MeshletbuilderInterface {

public:

	MeshoptimizerBuilder(SharedData* shared) :
		MeshletbuilderInterface("Meshoptimizer", shared)
	{};

	virtual void generate(uint32_t aMaxVertices, uint32_t aMaxIndices) override;

private:

};