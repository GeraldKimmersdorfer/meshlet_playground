/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

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