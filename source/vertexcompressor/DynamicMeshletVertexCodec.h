#pragma once
#include "VertexCompressionInterface.h"

class DynamicMeshletVertexCodec : public VertexCompressionInterface {

	struct cpu_compressed_vertex_data {
		glm::u16vec3 mPosition;
		glm::u16vec2 mNormal;
		glm::u16vec2 mTexCoord;
		uint16_t mBoneIndexLuid;
		uint32_t mBoneWeightsAndPermutation;
	};

public:

	DynamicMeshletVertexCodec(SharedData* shared)
		: VertexCompressionInterface(shared, "Dynamic Meshlet", "_DMLT")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:
	std::vector<cpu_compressed_vertex_data> mCompressedVertexData;

	std::vector<vertex_data_meshlet_coding> mVertexData;
	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mVertexBuffer;
	avk::buffer mBoneLUTBuffer;

	//bool mWithShuffle = false;

};