#pragma once

#include "MeshletbuilderInterface.h"

class AVKBuilder : public MeshletbuilderInterface {

public:

	AVKBuilder(SharedData* shared) :
		MeshletbuilderInterface("AVK-Default", shared)
	{};

protected:
	virtual void doGenerate() override;

private:

};