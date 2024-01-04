#pragma once

#include <glm/glm.hpp>

// 18 bit MeshIndex, 7 bit Vertex Count, 7 bit Triangle Count
uint32_t packMeshIdxVcTc(uint32_t meshIndex, uint8_t vertexCount, uint8_t triangleCount);
void unpackMeshIdxVcTc(uint32_t src, uint32_t& meshIndex, uint8_t& vertexCount, uint8_t& triangleCount);

// Encodes normals with octahedron mapping and returns the uvec2 as
// quantisized 16 bit packed in one 32 bit uint
uint32_t packNormal(glm::vec3 normal);

glm::u16vec2 compressNormal(glm::vec3 normal);

glm::vec3 unpackNormal(uint32_t normal);

// Encodes texture coordinate by discretizing its components into 16 bit each
// The result gets packed in one 32 bit uint.
uint32_t packTextureCoords(glm::vec2 texCoord);

glm::u16vec2 compressTextureCoords(glm::vec2 texCoords);

glm::uvec2 encodeVec3ToUVec2(const glm::vec3& value);

glm::vec3 decodeUVec2ToVec3(const glm::uvec2& value);

uint16_t packMbiluidAndPermutation(uint32_t mbiluid, uint32_t permutation);

void unpackMbiluidAndPermutation(uint16_t packedValue, uint32_t& mbiluid, uint32_t& permutation);