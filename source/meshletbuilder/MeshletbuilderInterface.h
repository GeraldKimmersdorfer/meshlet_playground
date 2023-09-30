#pragma once

#include <string>
#include <vector>
#include "../shared_data.h"

class MeshletbuilderInterface {

public:

	MeshletbuilderInterface(const std::string& name) :
		mName(name)
	{};

	virtual std::vector<meshlet_native> generate(std::vector<vertex_data>& vertexData, std::vector<uint32_t>& indexData, std::vector<mesh_data>& meshData, uint32_t aMaxVertices, uint32_t aMaxIndices) = 0;

	const std::string& getName() { return mName; }

protected:

	std::string mName;

private:

};