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