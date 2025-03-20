#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <vector>
#include "Triangle.h"
#include "ObjLoader.h"

class Raytracer {
public:
    std::vector<Triangle> triangles; // Store the cow's triangles
    glm::vec3 lightPos;
    glm::vec3 lightColor;

    // Constructor
    Raytracer();

    // Load cow model and store as triangle mesh
    void loadModel(ObjLoader& objLoader, float reflectivity = 0.0f);

    // Check if a point is in shadow
    bool isInShadow(const glm::vec3& point, const glm::vec3& lightDir) const;

    // Main ray tracing function with reflections
    glm::vec3 traceRay(const Ray& ray, int depth = 3) const;
};

#endif
