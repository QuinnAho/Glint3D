#pragma once

#include <glm/glm.hpp>
#include "seeded_rng.h"

namespace microfacet
{
    // Utility class for deterministic random number generation with seed support
    using SeededRNG = SeededRng;

    // Sample a microfacet normal using Beckmann distribution
    // Returns the sampled microfacet normal in world space
    glm::vec3 sampleBeckmannNormal(
        const glm::vec3& normal,           // Surface normal
        float roughness,                   // Material roughness [0, 1]
        SeededRNG& rng                     // Random number generator
    );
    
    // Sample multiple Beckmann normals with stratified sampling
    // Returns vector of sampled microfacet normals
    std::vector<glm::vec3> sampleBeckmannNormalsStratified(
        const glm::vec3& normal,           // Surface normal
        float roughness,                   // Material roughness [0, 1]
        int sampleCount,                   // Number of samples to generate
        SeededRNG& rng                     // Random number generator
    );
    
    // Check if roughness is close enough to zero for perfect mirror fallback
    inline bool shouldUsePerfectMirror(float roughness) {
        return roughness < 0.01f; // Threshold for mirror fallback
    }
    
    // Compute reflection direction for a given incident direction and microfacet normal
    inline glm::vec3 reflect(const glm::vec3& incident, const glm::vec3& normal) {
        return incident - 2.0f * glm::dot(incident, normal) * normal;
    }
}