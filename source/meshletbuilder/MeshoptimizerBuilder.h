#pragma once

#include "MeshletbuilderInterface.h"

class MeshoptimizerBuilder : public MeshletbuilderInterface {

public:

	MeshoptimizerBuilder() :
		MeshletbuilderInterface("Meshoptimizer")
	{};

	virtual std::vector<meshlet_data> generate(std::vector<vertex_data>& vertexData, std::vector<uint32_t>& indexData, std::vector<mesh_data>& meshData, uint32_t aMaxVertices, uint32_t aMaxIndices) override;

private:

};