#pragma once

#include "MeshletbuilderInterface.h"

class MeshoptimizerBuilder : public MeshletbuilderInterface {

public:

	MeshoptimizerBuilder(SharedData* shared) :
		MeshletbuilderInterface("Meshoptimizer", shared)
	{};

protected:
	virtual void doGenerate() override;

private:

};