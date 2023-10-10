#pragma once

#include <glm/glm.hpp>

// 18 bit MeshIndex, 7 bit Vertex Count, 7 bit Triangle Count
uint32_t packMeshIdxVcTc(uint32_t meshIndex, uint8_t vertexCount, uint8_t triangleCount);
void unpackMeshIdxVcTc(uint32_t src, uint32_t& meshIndex, uint8_t& vertexCount, uint8_t& triangleCount);

void quantizePosition(glm::vec3 normalizedPosition, uint16_t* quantizedPositions);

// Encodes normals with octahedron mapping and returns the uvec2 as
// quantisized 16 bit packed in one 32 bit uint
uint32_t packNormal(glm::vec3 normal);

glm::vec3 unpackNormal(uint32_t normal);

// Encodes texture coordinate by discretizing its components into 16 bit each
// The result gets packed in one 32 bit uint.
uint32_t packTextureCoords(glm::vec2 texCoord);