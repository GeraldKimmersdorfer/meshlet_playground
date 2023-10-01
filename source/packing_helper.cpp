#include "packing_helper.h"

uint32_t packMeshIdxVcTc(uint32_t meshIndex, uint8_t vertexCount, uint8_t triangleCount) {
	 return
		 ((static_cast<uint32_t>(meshIndex) & 0x3FFFF) << 18u) |
		 ((static_cast<uint32_t>(vertexCount) & 0x7F) << 7u) |
		 ((static_cast<uint32_t>(triangleCount) & 0x7F) << 0u);
}