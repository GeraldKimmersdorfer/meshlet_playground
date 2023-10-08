#pragma once

#include <glm/glm.hpp>

// 18 bit MeshIndex, 7 bit Vertex Count, 7 bit Triangle Count
uint32_t packMeshIdxVcTc(uint32_t meshIndex, uint8_t vertexCount, uint8_t triangleCount);

void unpackMeshIdxVcTc(uint32_t src, uint32_t& meshIndex, uint8_t& vertexCount, uint8_t& triangleCount);