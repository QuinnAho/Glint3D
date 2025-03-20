#include "raytracer.h"
#include <glm/glm.hpp>
#include <iostream>

// Constructor - Initialize Light Position
Raytracer::Raytracer()
    : lightPos(glm::vec3(-2.0f, 4.0f, -3.0f)),
    lightColor(glm::vec3(1.0f, 1.0f, 1.0f)) {}

// Load the Cow Model and Convert to Triangle Mesh
void Raytracer::loadModel(ObjLoader& objLoader, float reflectivity) {
    const float* rawPositions = objLoader.getPositions(); // Get raw float array
    const unsigned int* indices = objLoader.getFaces();   // Get triangle indices
    size_t numVertices = objLoader.getVertCount();
    size_t numTriangles = objLoader.getIndexCount() / 3;

    std::vector<glm::vec3> positions(numVertices);

    // Convert raw float array to glm::vec3 vector
    for (size_t i = 0; i < numVertices; i++) {
        positions[i] = glm::vec3(rawPositions[i * 3], rawPositions[i * 3 + 1], rawPositions[i * 3 + 2]);
    }

    // Construct triangle objects
    for (size_t i = 0; i < numTriangles; i++) {
        glm::vec3 v0 = positions[indices[i * 3]];
        glm::vec3 v1 = positions[indices[i * 3 + 1]];
        glm::vec3 v2 = positions[indices[i * 3 + 2]];
        triangles.emplace_back(v0, v1, v2, reflectivity);
    }
}


// Check if a Point is in Shadow
bool Raytracer::isInShadow(const glm::vec3& point, const glm::vec3& lightDir) const {
    float t;
    glm::vec3 normal;
    Ray shadowRay(point + lightDir * 0.001f, lightDir); // Offset to avoid self-shadowing

    for (const auto& tri : triangles) {
        if (tri.intersect(shadowRay, t, normal)) {
            return true; // Object blocks the light
        }
    }
    return false;
}

// Main Ray Tracing Function with Reflections
glm::vec3 Raytracer::traceRay(const Ray& ray, int depth) const {
    float tMin = FLT_MAX;
    glm::vec3 hitColor(0.0f);
    glm::vec3 hitNormal;
    const Triangle* hitObject = nullptr;

    // Find the closest triangle hit
    for (const auto& tri : triangles) {
        float t;
        glm::vec3 normal;
        if (tri.intersect(ray, t, normal) && t < tMin) {
            tMin = t;
            hitNormal = normal;
            hitObject = &tri;
        }
    }

    // No hit, return background color
    if (!hitObject) return glm::vec3(0.1f, 0.1f, 0.1f); // Dark gray background

    // Compute lighting (Phong model)
    glm::vec3 hitPoint = ray.origin + tMin * ray.direction;
    glm::vec3 lightDir = glm::normalize(lightPos - hitPoint);
    float brightness = (!isInShadow(hitPoint, lightDir)) ? glm::max(glm::dot(hitNormal, lightDir), 0.0f) : 0.0f;
    hitColor = glm::vec3(1.0f) * lightColor * brightness; // White cow model

    // Recursive Reflection
    if (hitObject->reflectivity > 0.0f && depth > 0) {
        glm::vec3 reflectionDir = glm::reflect(ray.direction, hitNormal);
        Ray reflectedRay(hitPoint + reflectionDir * 0.001f, reflectionDir);
        glm::vec3 reflectedColor = traceRay(reflectedRay, depth - 1);
        hitColor = glm::mix(hitColor, reflectedColor, hitObject->reflectivity);
    }

    return hitColor;
}
