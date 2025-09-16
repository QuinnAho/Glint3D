#include "grid.h"
#include "gl_platform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <glint3d/rhi.h>

// Static RHI instance for uniform bridging
glint3d::RHI* Grid::s_rhi = nullptr;

Grid::Grid() : m_VAO(0), m_VBO(0), m_shader(nullptr), m_lineCount(200) {}

Grid::~Grid() {
    cleanup();
}

bool Grid::init(Shader* shader, int lineCount, float spacing)
{
    m_shader = shader;
    m_lineCount = lineCount;

    std::vector<glm::vec3> lines;
    float halfSize = static_cast<float>(lineCount) / 2.0f;

    for (int i = -lineCount; i <= lineCount; ++i)
    {
        float x = static_cast<float>(i) * spacing;
        lines.emplace_back(glm::vec3(x, 0.0f, -halfSize * spacing));
        lines.emplace_back(glm::vec3(x, 0.0f, halfSize * spacing));
    }

    for (int i = -lineCount; i <= lineCount; ++i)
    {
        float z = static_cast<float>(i) * spacing;
        lines.emplace_back(glm::vec3(-halfSize * spacing, 0.0f, z));
        lines.emplace_back(glm::vec3(halfSize * spacing, 0.0f, z));
    }

    m_lineVertices = lines;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return true;
}

void Grid::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_shader) return;
    m_shader->use();

    glm::mat4 model = glm::mat4(1.0f);

    // Route uniforms through RHI only (no direct shader calls)
    if (s_rhi) {
        s_rhi->setUniformMat4("model", model);
        s_rhi->setUniformMat4("view", view);
        s_rhi->setUniformMat4("projection", projection);
        s_rhi->setUniformVec3("gridColor", Colors::LightGray);
    }

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineVertices.size()));
    glBindVertexArray(0);
}

void Grid::cleanup()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    m_VAO = 0;
    m_VBO = 0;
}

void Grid::setRHI(glint3d::RHI* rhi)
{
    s_rhi = rhi;
}
