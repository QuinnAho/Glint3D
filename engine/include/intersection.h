#pragma once

#include <glm/glm.hpp>
#include "triangle.h"

// CPU ray tracing hit record carrying shading payload
struct HitRecord
{
    bool hit = false;
    float t = 0.0f;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    const Triangle* tri = nullptr;

    // Material snapshot (PBR)
    glm::vec3 baseColor{1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float ior = 1.5f;
    glm::vec3 f0{0.04f};
    float density = 1.0f;
    bool hasReflection = false;
    bool hasRefraction = false;
};

