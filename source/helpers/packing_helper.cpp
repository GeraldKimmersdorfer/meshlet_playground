#include "packing_helper.h"

glm::vec2 OctWrap(glm::vec2 v)
{
	auto yx = glm::vec2(v.y, v.x);
	auto mult = glm::vec2(
		(v.x >= 0.0) ? 1.0 : -1.0,
		(v.y >= 0.0) ? 1.0 : -1.0
	);
	return (glm::vec2(1.0) - glm::abs(yx)) * mult;
}

glm::vec2 octahedronEncode(glm::vec3 normal)
{
	auto n = normal / (glm::abs(normal.x) + glm::abs(normal.y) + glm::abs(normal.z));
	auto tmp = OctWrap(glm::vec2(n.x, n.y));
	n.x = (n.z >= 0.0) ? n.x : tmp.x;
	n.y = (n.z >= 0.0) ? n.y : tmp.y;
	n.x = n.x * 0.5 + 0.5;
	n.y = n.y * 0.5 + 0.5;
	return glm::vec2(n.x, n.y);
}

glm::vec3 octahedronDecode(glm::vec2 f)
{
	f = f * glm::vec2(2.0f) - glm::vec2(1.0f);

	// https://twitter.com/Stubbesaurus/status/937994790553227264
	glm::vec3 n = glm::vec3(
		f.x,
		f.y,
		1.0f - glm::abs(f.x) - glm::abs(f.y)
	);
	float t = glm::clamp(-n.z, 0.0f, 1.0f);
	n.x += n.x >= 0.0f ? -t : t;
	n.y += n.y >= 0.0f ? -t : t;
	return glm::normalize(n);
}

uint32_t packMeshIdxVcTc(uint32_t meshIndex, uint8_t vertexCount, uint8_t triangleCount) {
	 return
		 ((static_cast<uint32_t>(meshIndex) & 0x3FFFF) << 18u) |
		 ((static_cast<uint32_t>(vertexCount) & 0x7F) << 7u) |
		 ((static_cast<uint32_t>(triangleCount) & 0x7F) << 0u);
}

void unpackMeshIdxVcTc(uint32_t src, uint32_t& meshIndex, uint8_t& vertexCount, uint8_t& triangleCount)
{
	meshIndex = (src >> 18u) & 0x3FFFF;
	vertexCount = (src >> 7u) & 0x7F;
	triangleCount = (src >> 0u) & 0x7F;
}

void quantizePosition(glm::vec3 normalizedPosition, uint16_t* quantizedPositions)
{
	quantizedPositions[0] = normalizedPosition.x * 0xFFFF;
	quantizedPositions[1] = normalizedPosition.y * 0xFFFF;
	quantizedPositions[2] = normalizedPosition.z * 0xFFFF;
}

uint32_t packNormal(glm::vec3 normal) {
	auto octahedron = octahedronEncode(normal);
	return (static_cast<uint32_t>(octahedron.x * 0xFFFF) << 16u) | (static_cast<uint32_t>(octahedron.y * 0xFFFF) << 0u);
}

glm::vec3 unpackNormal(uint32_t normal) {
	auto octahedron = glm::vec2(
		(normal >> 16u) * (1.0F/0xFFFF),
		(normal & 0xFFFF) * (1.0F/0xFFFF)
	);
	return octahedronDecode(octahedron);
}

uint32_t packTextureCoords(glm::vec2 texCoord)
{
	texCoord = glm::clamp(texCoord, glm::vec2(0.0), glm::vec2(1.0));
	assert(texCoord.x >= 0.0 && texCoord.x <= 1.0 && texCoord.y >= 0.0 && texCoord.y <= 1.0);
	return (static_cast<uint32_t>(texCoord.x * 0xFFFF) << 16u) | (static_cast<uint32_t>(texCoord.y * 0xFFFF) << 0u);
}
