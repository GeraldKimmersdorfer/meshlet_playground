#pragma once

#include "MeshletbuilderInterface.h"

class BoneLUTDependentBuilder : public MeshletbuilderInterface {

public:

	BoneLUTDependentBuilder(SharedData* shared) :
		MeshletbuilderInterface("BoneLUT Builder", shared)
	{};

protected:
	virtual void doGenerate() override;

private:

};