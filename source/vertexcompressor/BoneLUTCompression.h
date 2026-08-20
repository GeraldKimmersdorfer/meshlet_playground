/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#include "VertexCompressionInterface.h"

class BoneLUTCompression : public VertexCompressionInterface {


public:

	BoneLUTCompression(SharedData* shared)
		: VertexCompressionInterface(shared, "Bone LUT (48 byte)", "_LUT")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<vertex_data_bone_lookup> mVertexData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	bool mWithReuse = true;
	bool mWithShuffle = false;
	bool mWithMerge = false;

};