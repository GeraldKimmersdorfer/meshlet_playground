#pragma once

#include <string>
#include <map>
#include <glm/glm.hpp>

// Struct definition for CameraDefinition
struct CameraDefinition {
    std::string mName;
    glm::mat4 mViewMatrix;
    glm::mat4 mProjectionMatrix;
};

// Function declarations with default filename
void saveCameraDefinitionsToFile(const std::map<std::string, CameraDefinition>& cameraDefinitions, const std::string& filename = "camera_storage.dat");
void readCameraDefinitionsFromFile(std::map<std::string, CameraDefinition>& cameraDefinitions, const std::string& filename = "camera_storage.dat");
