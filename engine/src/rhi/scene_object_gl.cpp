#include "rhi/scene_object_gl.h"
#include "scene_manager.h"
#include "gl_platform.h"

using namespace glint3d;

namespace rhi {

void setupSceneObjectGL(SceneObject& obj) {
    if (obj.objLoader.getVertCount() == 0) {
        return;
    }

    const size_t Nv = obj.objLoader.getVertCount();
    const bool hasNormals = (obj.objLoader.getNormals() != nullptr);
    const bool hasUVs = obj.objLoader.hasTexcoords();

    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO_positions);
    if (hasNormals) glGenBuffers(1, &obj.VBO_normals);
    if (hasUVs) glGenBuffers(1, &obj.VBO_uvs);
    if (obj.objLoader.getIndexCount() > 0) glGenBuffers(1, &obj.EBO);

    glBindVertexArray(obj.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, Nv * 3 * sizeof(float), obj.objLoader.getPositions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    if (hasNormals) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
        glBufferData(GL_ARRAY_BUFFER, Nv * 3 * sizeof(float), obj.objLoader.getNormals(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }

    if (hasUVs) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_uvs);
        glBufferData(GL_ARRAY_BUFFER, Nv * 2 * sizeof(float), obj.objLoader.getTexcoords(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
    }

    if (obj.objLoader.getIndexCount() > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.objLoader.getIndexCount() * sizeof(unsigned int),
                     obj.objLoader.getFaces(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
}

void cleanupSceneObjectGL(SceneObject& obj) {
    if (obj.VAO) {
        glDeleteVertexArrays(1, &obj.VAO);
        obj.VAO = 0;
    }
    if (obj.VBO_positions) {
        glDeleteBuffers(1, &obj.VBO_positions);
        obj.VBO_positions = 0;
    }
    if (obj.VBO_normals) {
        glDeleteBuffers(1, &obj.VBO_normals);
        obj.VBO_normals = 0;
    }
    if (obj.VBO_uvs) {
        glDeleteBuffers(1, &obj.VBO_uvs);
        obj.VBO_uvs = 0;
    }
    if (obj.VBO_tangents) {
        glDeleteBuffers(1, &obj.VBO_tangents);
        obj.VBO_tangents = 0;
    }
    if (obj.EBO) {
        glDeleteBuffers(1, &obj.EBO);
        obj.EBO = 0;
    }
}

} // namespace rhi
