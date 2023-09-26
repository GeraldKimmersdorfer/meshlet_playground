#pragma once

#include "PipelineInterface.h"

class SharedData;

/// <summary>
/// asdasdasd
/// </summary>
class PipelineFactory {

public:

	PipelineFactory(SharedData* shared):
		mShared(shared) 
	{};



protected:

	SharedData* mShared;

};