/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

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