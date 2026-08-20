/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

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