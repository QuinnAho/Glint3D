#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <vector>
#include <glm/glm.hpp> // Include GLM for vec3

// Simple struct to hold face indices
struct Face {
    unsigned int a, b, c;
};

class ObjLoader {
private:
    std::vector<glm::vec3> Positions;
    std::vector<Face> Faces;
    glm::vec3 minBound, maxBound; // Bounding box variables

public:
    ObjLoader();
    void load(const char* filename);

    int getVertCount();
    int getIndexCount();
    const float* getPositions();
    const unsigned int* getFaces();

    // Bounding box functions
    glm::vec3 getMinBounds();
    glm::vec3 getMaxBounds();
};

#endif
