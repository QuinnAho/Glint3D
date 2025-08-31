#include "ObjLoader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>   // glm::min / glm::max

ObjLoader::ObjLoader()
    : minBound(std::numeric_limits<float>::max()),
    maxBound(std::numeric_limits<float>::lowest())
{}

void ObjLoader::load(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[ObjLoader] cannot open " << filename << '\n';
        return;
    }

    Positions.clear();
    Faces.clear();
    Normals.clear();

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string type;  ss >> type;

        if (type == "v")                               // vertex
        {
            glm::vec3 v;  ss >> v.x >> v.y >> v.z;
            Positions.push_back(v);

            minBound = glm::min(minBound, v);
            maxBound = glm::max(maxBound, v);
        }
        else if (type == "f")                          // face
        {
            Face f;  ss >> f.a >> f.b >> f.c;
            --f.a; --f.b; --f.c;                      // OBJ is 1-based
            Faces.push_back(f);
        }
    }
    file.close();

    computeNormals();
}

void ObjLoader::setFromRaw(const std::vector<glm::vec3>& positions,
                           const std::vector<unsigned>& indices,
                           const std::vector<glm::vec3>& normals,
                           const std::vector<glm::vec2>& uvs,
                           const std::vector<glm::vec3>& tangents)
{
    Positions = positions;
    Faces.clear();
    Faces.reserve(indices.size() / 3);
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        Face f{ indices[i + 0], indices[i + 1], indices[i + 2] };
        Faces.push_back(f);
    }

    // Bounds
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& v : Positions)
    {
        minBound = glm::min(minBound, v);
        maxBound = glm::max(maxBound, v);
    }

    // Normals
    if (!normals.empty() && normals.size() == Positions.size())
    {
        Normals = normals;
    }
    else
    {
        computeNormals();
    }

    // UVs and tangents
    Texcoords = uvs;
    if (!tangents.empty() && tangents.size() == Positions.size())
        Tangents = tangents;
    else if (!Texcoords.empty() && !Normals.empty())
        computeTangents();
}

void ObjLoader::reset()
{
    Positions.clear();
    Faces.clear();
    Normals.clear();
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());
}

/* ------------------------------------------------------------ */
void ObjLoader::computeNormals()
{
    Normals.assign(Positions.size(), glm::vec3(0.0f));

    for (const Face& f : Faces)
    {
        const glm::vec3& v0 = Positions[f.a];
        const glm::vec3& v1 = Positions[f.b];
        const glm::vec3& v2 = Positions[f.c];

        glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        Normals[f.a] += n;
        Normals[f.b] += n;
        Normals[f.c] += n;
    }
    for (glm::vec3& n : Normals) n = glm::normalize(n);
}

void ObjLoader::computeTangents()
{
    Tangents.assign(Positions.size(), glm::vec3(0.0f));
    if (Texcoords.empty()) return;
    for (const Face& f : Faces)
    {
        unsigned ia=f.a, ib=f.b, ic=f.c;
        const glm::vec3 &v0=Positions[ia], &v1=Positions[ib], &v2=Positions[ic];
        const glm::vec2 &uv0=Texcoords[ia], &uv1=Texcoords[ib], &uv2=Texcoords[ic];

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec2 dUV1 = uv1 - uv0;
        glm::vec2 dUV2 = uv2 - uv0;
        float denom = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
        float r = (fabs(denom) < 1e-8f) ? 0.0f : 1.0f / denom;
        glm::vec3 t = (e1 * dUV2.y - e2 * dUV1.y) * r;
        Tangents[ia] += t;
        Tangents[ib] += t;
        Tangents[ic] += t;
    }
    for (size_t i=0;i<Tangents.size();++i)
    {
        glm::vec3 n = Normals.size()==Tangents.size()? Normals[i] : glm::vec3(0,0,1);
        Tangents[i] = glm::normalize(Tangents[i] - n * glm::dot(n, Tangents[i]));
    }
}

glm::vec3            ObjLoader::getMinBounds()  const { return minBound; }
glm::vec3            ObjLoader::getMaxBounds()  const { return maxBound; }

int  ObjLoader::getVertCount()  const { return static_cast<int>(Positions.size()); }
int  ObjLoader::getIndexCount() const { return static_cast<int>(Faces.size() * 3); }

const float* ObjLoader::getPositions() const
{
    return reinterpret_cast<const float*>(Positions.data());
}

const unsigned int* ObjLoader::getFaces() const
{
    return reinterpret_cast<const unsigned int*>(Faces.data());
}

const float* ObjLoader::getNormals() const
{
    return reinterpret_cast<const float*>(Normals.data());
}

const float* ObjLoader::getTexcoords() const
{
    return reinterpret_cast<const float*>(Texcoords.data());
}

const float* ObjLoader::getTangents() const
{
    return reinterpret_cast<const float*>(Tangents.data());
}
