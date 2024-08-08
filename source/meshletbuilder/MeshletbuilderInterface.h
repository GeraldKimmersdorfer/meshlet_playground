/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>
#include "../shared_structs.h"

class SharedData;

class MeshletbuilderInterface {

public:

	MeshletbuilderInterface(const std::string& name, SharedData* shared) :
		mName(name), mShared(shared)
	{};

	void generate();

	void destroy();

	const std::string& getName() { return mName; }
	const std::pair<std::vector<meshlet_redirect>, std::vector<uint32_t>> getMeshletsRedirect();
	const std::vector<meshlet_native>& getMeshletsNative();

	void overwriteMeshlets(const std::vector<meshlet_native>& meshletsNative);
	void reportBufferSizes();


protected:

	virtual void doGenerate() = 0;
	virtual void doDestroy() {}

	std::vector<meshlet_native> mMeshletsNative;
	std::vector<meshlet_redirect> mMeshletsRedirect;
	std::vector<uint32_t> mRedirectPackedIndexData;

	std::string mName;

	SharedData* mShared;

	void generateRedirectedMeshletsFromNative();

	// a hacking function such that i dont need the vertex offset inside the shader
	void addVertexOffsetToMeshlets();

private:
	// Buffer-Variable such that we can check when a model has been reloaded and meshlets need to be recreated
	uint32_t mGeneratedOnIndexCount = 0;
};