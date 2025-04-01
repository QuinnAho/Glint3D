#include "objloader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits> 
#include <glm/gtc/type_ptr.hpp> // for glm::min, glm::max

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>

ObjLoader::ObjLoader()
{
    // Initialize bounding box with extreme values
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());
}

void ObjLoader::load(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file: " << filename << std::endl;
        return;
    }
    else {
        std::cout << "Opened file successfully: " << filename << std::endl;
    }

    Positions.clear();
    Faces.clear();
    Normals.clear();

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            Positions.push_back(v);

            // Update bounding box
            minBound = glm::min(minBound, v);
            maxBound = glm::max(maxBound, v);

            // Debug
            // std::cout << "Vertex: " << v.x << ", " << v.y << ", " << v.z << std::endl;
        }
        else if (type == "f") {
            Face face;
            ss >> face.a >> face.b >> face.c;
            // Convert to 0-based indices
            face.a--;
            face.b--;
            face.c--;
            Faces.push_back(face);

            // Debug
            // std::cout << "Face: " << face.a << ", " << face.b << ", " << face.c << std::endl;
        }
    }

    file.close();

    // Once loaded, compute vertex normals
    computeNormals();
}

void ObjLoader::computeNormals()
{
    // 1) Initialize Normals to zero for each position
    Normals.resize(Positions.size(), glm::vec3(0.0f));

    // 2) For each face, compute face normal & accumulate to each vertex
    for (auto& f : Faces)
    {
        glm::vec3 v0 = Positions[f.a];
        glm::vec3 v1 = Positions[f.b];
        glm::vec3 v2 = Positions[f.c];

        glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        // Accumulate this face normal in each of the face's vertices
        Normals[f.a] += faceNormal;
        Normals[f.b] += faceNormal;
        Normals[f.c] += faceNormal;
    }

    // 3) Normalize each vertex normal so it becomes unit length
    for (auto& n : Normals) {
        n = glm::normalize(n);
    }
}

// Accessors
glm::vec3 ObjLoader::getMinBounds() { return minBound; }
glm::vec3 ObjLoader::getMaxBounds() { return maxBound; }

int ObjLoader::getVertCount() {
    return static_cast<int>(Positions.size());
}

int ObjLoader::getIndexCount() {
    return static_cast<int>(Faces.size() * 3);
}

const float* ObjLoader::getPositions() {
    // We store positions in a vector<glm::vec3>, so reinterpret as floats
    return reinterpret_cast<const float*>(Positions.data());
}

const unsigned int* ObjLoader::getFaces() {
    // Each Face has 3 unsigned int indices
    return reinterpret_cast<const unsigned int*>(Faces.data());
}

// NEW: Return pointer to normals array
const float* ObjLoader::getNormals() {
    return reinterpret_cast<const float*>(Normals.data());
}
