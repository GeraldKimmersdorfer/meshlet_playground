/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#include "VertexCompressionInterface.h"

class OptimalSimplexCoding : public VertexCompressionInterface {

	// the pragma pack is necessary to ensure that the struct is not padded
#pragma pack(push, 1)
	struct cpu_compressed_vertex_data {
		glm::u16vec3 position;
		glm::u16vec2 normal;
		glm::u16vec2 texCoord;
		uint16_t tupleIndex;
		uint16_t boneWeights;
	}; // 18 bytes
#pragma pack(pop)

public:

	OptimalSimplexCoding(SharedData* shared)
		: VertexCompressionInterface(shared, "Optimal Simplex Coding", "_OSS")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<uint8_t> mVertexData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	bool mWithReuse = true;
	bool mSortVertexDataInMeshletOrder = true;

};