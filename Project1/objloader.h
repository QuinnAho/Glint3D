#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Face {
    unsigned int a, b, c;
};

class ObjLoader {
public:
    ObjLoader();

    void load(const char* filename);

    // Accessors
    int getVertCount();
    int getIndexCount();
    const float* getPositions();
    const unsigned int* getFaces();
    const float* getNormals();  // NEW

    glm::vec3 getMinBounds();
    glm::vec3 getMaxBounds();

private:
    // Helper to compute vertex normals after reading faces
    void computeNormals();

private:
    std::vector<glm::vec3> Positions;
    std::vector<Face> Faces;

    // NEW: Store normals in a parallel array to Positions
    std::vector<glm::vec3> Normals;

    glm::vec3 minBound;
    glm::vec3 maxBound;
};
