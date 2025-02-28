#include "objloader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits> 
#include <glm/gtc/type_ptr.hpp> 

ObjLoader::ObjLoader() {

    // Initialize bounding box with extreme values
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());
}

void ObjLoader::load(const char* filename) {
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

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            Positions.push_back(v);

            minBound = glm::min(minBound, v);
            maxBound = glm::max(maxBound, v);

            std::cout << "Vertex: " << v.x << ", " << v.y << ", " << v.z << std::endl;
        }
        else if (type == "f") { 
            Face face;
            ss >> face.a >> face.b >> face.c;
            face.a--; face.b--; face.c--; 
            Faces.push_back(face);
            std::cout << "Face: " << face.a << ", " << face.b << ", " << face.c << std::endl;
        }
    }

    file.close();
}

glm::vec3 ObjLoader::getMinBounds() { return minBound; }
glm::vec3 ObjLoader::getMaxBounds() { return maxBound; }

// Existing Functions
int ObjLoader::getVertCount() { return Positions.size(); }
int ObjLoader::getIndexCount() { return Faces.size() * 3; }
const float* ObjLoader::getPositions() { return reinterpret_cast<const float*>(Positions.data()); }
const unsigned int* ObjLoader::getFaces() { return reinterpret_cast<const unsigned int*>(Faces.data()); }
