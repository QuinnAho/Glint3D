#include "mesh_loader.h"
#include "ObjLoader.h"
#include <algorithm>

static std::string toLowerExt(const std::string& s){
    auto pos = s.find_last_of('.');
    if (pos == std::string::npos) return {};
    std::string e = s.substr(pos);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return e;
}

bool LoadMeshFromFile(const std::string& path, MeshData& out, PBRMaterial* pbrOut, std::string* error)
{
    out = MeshData{};
    std::string ext = toLowerExt(path);
    if (ext == ".obj")
    {
        ObjLoader l; l.load(path.c_str());
        int vc = l.getVertCount();
        int ic = l.getIndexCount();
        const float* pos = l.getPositions();
        const float* nor = l.getNormals();
        const unsigned* idx = l.getFaces();
        out.positions.resize(vc);
        for (int i=0;i<vc;i++) out.positions[i] = glm::vec3(pos[3*i+0], pos[3*i+1], pos[3*i+2]);
        if (nor) { out.normals.resize(vc); for (int i=0;i<vc;i++) out.normals[i] = glm::vec3(nor[3*i+0], nor[3*i+1], nor[3*i+2]); }
        out.indices.resize(ic);
        for (int i=0;i<ic;i++) out.indices[i] = idx[i];
        out.minBound = l.getMinBounds();
        out.maxBound = l.getMaxBounds();
        return !out.positions.empty() && !out.indices.empty();
    }
    else
    {
        glm::vec3 minB, maxB; std::string err;
        if (!AssimpImportMesh(path, out.positions, out.indices, out.normals, minB, maxB, &err, &out.uvs, &out.tangents, pbrOut, true))
        {
            if (error) *error = err;
            return false;
        }
        out.minBound = minB; out.maxBound = maxB;
        return true;
    }
}

