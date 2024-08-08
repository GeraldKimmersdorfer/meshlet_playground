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

class MeshletRiggedCompression : public VertexCompressionInterface {

	struct mrc_per_meshlet_data {
		glm::u16vec4 mMbiTable = glm::u16vec4(UINT16_MAX);	// 4 possible bone indices per meshlet
	};


public:

	MeshletRiggedCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "Meshlet Rigged", "_MLTR")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<vertex_data_meshlet_coding> mVertexData;
	std::vector<mrc_per_meshlet_data> mAdditionalMeshletData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;
	avk::buffer mAdditionalMeshletBuffer;

	bool mWithReuse = true;
	bool mWithShuffle = false;
	bool mWithMerge = false;

};