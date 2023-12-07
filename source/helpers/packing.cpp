#include "packing.h"

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

uint32_t packNormal(glm::vec3 normal) {
	auto compressed = compressNormal(normal);
	return (static_cast<uint32_t>(compressed.x) << 16u) | (static_cast<uint32_t>(compressed.y) << 0u);
}

glm::u16vec2 compressNormal(glm::vec3 normal)
{
	auto octahedron = octahedronEncode(normal);
	return glm::u16vec2{
		static_cast<uint16_t>(octahedron.x * 0xFFFF),
		static_cast<uint16_t>(octahedron.y * 0xFFFF)
	};
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
	auto compressed = compressTextureCoords(texCoord);
	return (static_cast<uint32_t>(compressed.x) << 16u) | (static_cast<uint32_t>(compressed.y) << 0u);
}

glm::u16vec2 compressTextureCoords(glm::vec2 texCoords)
{
	assert(texCoords.x >= 0.0 && texCoords.x <= 1.0 && texCoords.y >= 0.0 && texCoords.y <= 1.0);
	return glm::u16vec2(
		static_cast<uint16_t>(texCoords.x * 0xFFFF),
		static_cast<uint16_t>(texCoords.y * 0xFFFF)
	);
}

glm::uvec2 encodeVec3ToUVec2(const glm::vec3& value)
{
	unsigned int x = static_cast<unsigned int>(value.x * float((1u << 21u) - 1u));
	unsigned int y = static_cast<unsigned int>(value.y * float((1u << 21u) - 1u));
	unsigned int z = static_cast<unsigned int>(value.z * float((1u << 21u) - 1u));

	return glm::uvec2((x << 11u) | (y >> 10u), (y << 22u) | z);
}

glm::vec3 decodeUVec2ToVec3(const glm::uvec2& value)
{
	unsigned int x = (value.x >> 11u) & ((1u << 21u) - 1u);
	unsigned int y = ((value.x << 10u) | (value.y >> 22u)) & ((1u << 21u) - 1u);
	unsigned int z = value.y & ((1u << 21u) - 1u);

	return glm::vec3(float(x) / float((1u << 21u) - 1u),
		float(y) / float((1u << 21u) - 1u),
		float(z) / float((1u << 21u) - 1u));
}
