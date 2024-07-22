#include "camera_storage.h"
#include <fstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

// Function to save camera definitions to a file
void saveCameraDefinitionsToFile(const std::map<std::string, CameraDefinition>& cameraDefinitions, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }

    // Write the number of camera definitions
    size_t numCameras = cameraDefinitions.size();
    file.write(reinterpret_cast<const char*>(&numCameras), sizeof(size_t));

    for (const auto& cameraDef : cameraDefinitions) {
        // Write the camera name length and name
        size_t nameLength = cameraDef.first.size();
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
        file.write(cameraDef.first.data(), nameLength);

        // Write the view matrix
        file.write(reinterpret_cast<const char*>(&cameraDef.second.mViewMatrix), sizeof(glm::mat4));

        // Write the projection matrix
        file.write(reinterpret_cast<const char*>(&cameraDef.second.mProjectionMatrix), sizeof(glm::mat4));
    }

    file.close();
}

// Function to read camera definitions from a file
void readCameraDefinitionsFromFile(std::map<std::string, CameraDefinition>& cameraDefinitions, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return; // Ignore if not existant
        //throw std::runtime_error("Could not open file for reading: " + filename);
    }

    // Read the number of camera definitions
    size_t numCameras;
    file.read(reinterpret_cast<char*>(&numCameras), sizeof(size_t));

    cameraDefinitions.clear();

    for (size_t i = 0; i < numCameras; ++i) {
        // Read the camera name length and name
        size_t nameLength;
        file.read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
        std::string name(nameLength, '\0');
        file.read(&name[0], nameLength);

        // Read the view matrix
        glm::mat4 viewMatrix;
        file.read(reinterpret_cast<char*>(&viewMatrix), sizeof(glm::mat4));

        // Read the projection matrix
        glm::mat4 projectionMatrix;
        file.read(reinterpret_cast<char*>(&projectionMatrix), sizeof(glm::mat4));

        // Insert into the map
        cameraDefinitions[name] = { name, viewMatrix, projectionMatrix };
    }

    file.close();
}
