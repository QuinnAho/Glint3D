#pragma once

#include "gl_platform.h"
#include <glm/glm.hpp>
#include <vector>
#include "shader.h"
#include "colors.h"

namespace glint3d { class RHI; }

class Grid {
public:
    Grid();
    ~Grid();

    // Initialize with shader, number of lines in each direction, and spacing between lines
    bool init(Shader* shader, int lineCount = 50, float spacing = 1.0f);

    // Render the grid using view/projection matrices
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Cleanup OpenGL resources
    void cleanup();

    // Set RHI instance for uniform management
    static void setRHI(glint3d::RHI* rhi);

private:
    // Static RHI instance for uniform bridging
    static glint3d::RHI* s_rhi;
    GLuint m_VAO, m_VBO;
    Shader* m_shader;
    int m_lineCount;
    float m_spacing;
    std::vector<glm::vec3> m_lineVertices;
};
