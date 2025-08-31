#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Import a triangulated mesh from various formats via Assimp.
// Returns false if Assimp is not enabled or import fails.
// - positions: vertex positions in object space
// - indices:   triangle indices (triplets)
// - normals:   per-vertex normals; generated if unavailable
// - minBound/maxBound: computed bounds
// - error: optional error string
bool AssimpImportMesh(const std::string& path,
                      std::vector<glm::vec3>& positions,
                      std::vector<unsigned>& indices,
                      std::vector<glm::vec3>& normals,
                      glm::vec3& minBound,
                      glm::vec3& maxBound,
                      std::string* error = nullptr);

