#include "assimp_loader.h"

#include <limits>
#include <iostream>

#ifdef USE_ASSIMP
  #include <assimp/Importer.hpp>
  #include <assimp/scene.h>
  #include <assimp/postprocess.h>
#endif

bool AssimpImportMesh(const std::string& path,
                      std::vector<glm::vec3>& positions,
                      std::vector<unsigned>& indices,
                      std::vector<glm::vec3>& normals,
                      glm::vec3& minBound,
                      glm::vec3& maxBound,
                      std::string* error)
{
    positions.clear();
    indices.clear();
    normals.clear();
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());

#ifndef USE_ASSIMP
    if (error) *error = "Assimp disabled. Define USE_ASSIMP and link Assimp to enable.";
    (void)path; // unused
    return false;
#else
    Assimp::Importer importer;
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_PreTransformVertices |
        aiProcess_SortByPType;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene || !scene->mRootNode)
    {
        if (error) *error = importer.GetErrorString();
        return false;
    }

    // After aiProcess_PreTransformVertices, node transforms are baked into meshes
    for (unsigned m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        if (!mesh) continue;

        const unsigned baseIndex = static_cast<unsigned>(positions.size());
        positions.reserve(positions.size() + mesh->mNumVertices);
        normals.reserve(normals.size() + mesh->mNumVertices);

        for (unsigned v = 0; v < mesh->mNumVertices; ++v)
        {
            aiVector3D p = mesh->mVertices[v];
            positions.emplace_back(p.x, p.y, p.z);
            minBound = glm::min(minBound, glm::vec3(p.x, p.y, p.z));
            maxBound = glm::max(maxBound, glm::vec3(p.x, p.y, p.z));

            if (mesh->HasNormals())
            {
                aiVector3D n = mesh->mNormals[v];
                normals.emplace_back(n.x, n.y, n.z);
            }
            else
            {
                normals.emplace_back(0.0f); // will be fixed later if needed
            }
        }

        for (unsigned f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue; // ensure triangulated (should be by flags)
            indices.push_back(baseIndex + face.mIndices[0]);
            indices.push_back(baseIndex + face.mIndices[1]);
            indices.push_back(baseIndex + face.mIndices[2]);
        }
    }

    // If no normals were provided by any mesh, generate flat ones per face
    bool needNormals = normals.empty();
    if (needNormals)
    {
        normals.assign(positions.size(), glm::vec3(0.0f));
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            unsigned i0 = indices[i+0], i1 = indices[i+1], i2 = indices[i+2];
            const glm::vec3& v0 = positions[i0];
            const glm::vec3& v1 = positions[i1];
            const glm::vec3& v2 = positions[i2];
            glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            normals[i0] += n; normals[i1] += n; normals[i2] += n;
        }
        for (auto& n : normals) n = glm::normalize(n);
    }

    return !positions.empty() && !indices.empty();
#endif
}

