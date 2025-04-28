#pragma once
/* ────────────────────────────────────────────
   Very small OBJ loader  (positions + faces)
   – normals are generated automatically
   – all read-only accessors are now  const
   ──────────────────────────────────────────── */
#include <vector>
#include <glm/glm.hpp>

struct Face { unsigned int a, b, c; };

class ObjLoader
{
public:
    ObjLoader();

    /* ------------ IO --------------- */
    void load(const char* filename);

    /* ------------ accessors -------- */
    int                  getVertCount()  const;
    int                  getIndexCount() const;

    const float* getPositions()  const;  // 3×getVertCount() floats
    const unsigned int* getFaces()      const;  // 3×getIndexCount()/3 uints
    const float* getNormals()    const;  // optional (same size as pos)

    glm::vec3            getMinBounds()  const;
    glm::vec3            getMaxBounds()  const;

private:
    void computeNormals();                       // helper

    std::vector<glm::vec3> Positions;
    std::vector<Face>      Faces;
    std::vector<glm::vec3> Normals;

    glm::vec3 minBound, maxBound;
};
