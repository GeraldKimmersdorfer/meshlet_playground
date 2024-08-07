#pragma once
#include "VertexCompressionInterface.h"

class PDifferenceCodec8 : public VertexCompressionInterface {

	struct cpu_compressed_vertex_data {
		glm::u16vec3 position;
		glm::u16vec2 normal;
		glm::u16vec2 texCoord;
		uint32_t boneAttributes;
	}; // 20 bytes

	struct cpu_temporary_vertex_data {
		glm::u16vec3 position;
		glm::u16vec2 normal;
		glm::u16vec2 texCoord;
		uint32_t boneAttributesDifference;
		uint8_t boneAttributesResolution; // 0, 1, 2, 3, 4

		uint32_t getFinalByteSize(uint8_t padToByte = 1) const {
			const auto size_static = sizeof(position) + sizeof(normal) + sizeof(texCoord);
			const auto size_dynamic = boneAttributesResolution;
			uint32_t size_total = size_static + size_dynamic;
			if (padToByte > 1) {
				const auto bytes_too_much = size_total % padToByte;
				const auto size_padding = bytes_too_much ? padToByte - bytes_too_much : 0;
				size_total += size_padding;
			}
			return size_total;
		}

		// IMPORTANT: This implementation assumes that both the gpu and cpu reads data in little endian
		// If this is not the case, at least this function has to be adopted!!!
		void addToDataVector(std::vector<uint8_t>& data, uint8_t padToByte = 1) const {
			assert(uint64_t(boneAttributesDifference) < (uint64_t(1) << (boneAttributesResolution * 8)));
			// add position/normal/texcoord:
			uint32_t sizeStart = data.size();
			auto ptr = reinterpret_cast<const uint8_t*>(&position);
			data.insert(data.end(), ptr, ptr + sizeof(position) + sizeof(normal) + sizeof(texCoord)); // 14 bytes

			if (boneAttributesResolution > 0) {
				ptr = reinterpret_cast<const uint8_t*>(&boneAttributesDifference);
				data.insert(data.end(), ptr, ptr + boneAttributesResolution);
			}
			uint8_t sizeDiff = static_cast<uint8_t>(data.size() - sizeStart);
			assert(sizeDiff == getFinalByteSize(1));

			if (padToByte > 1) {
				const auto bytes_too_much = sizeDiff % padToByte;
				const auto size_padding = bytes_too_much ? padToByte - bytes_too_much : 0;
				for (uint8_t i = 0; i < size_padding; ++i) data.push_back(0);
				sizeDiff = static_cast<uint8_t>(data.size() - sizeStart);
				assert(sizeDiff == getFinalByteSize(padToByte));
			}
		}
	};

	struct meshlet_extension {
		uint32_t boneAttributesMin;
		uint32_t boneAttributesResolution = 0; // 0, 1, 2, 3 (byte resolution necessary for this meshlet)
	};

public:

	PDifferenceCodec8(SharedData* shared)
		: VertexCompressionInterface(shared, "P. Difference Codec 8bit", "_PDC8")
	{}

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) override;

	// Has to free all ressources
	virtual void doDestroy() override;

	void hud_config(bool& config_has_changed) override;


private:

	std::vector<uint8_t> m8BitVertexData;
	avk::buffer m8BitVertexBuffer;

	std::vector<meshlet_extension> mMeshletExtensionData;
	avk::buffer mMeshletExtensionBuffer;

	std::vector<glm::u16vec4> mBoneLUTData;
	avk::buffer mBoneLUTBuffer;


	bool mWithReuse = true;
	bool mSortVertexDataInMeshletOrder = true;

};