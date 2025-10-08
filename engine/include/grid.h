#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glint3d/rhi_types.h>
#include "shader.h"
#include "colors.h"

namespace glint3d { class RHI; }

class Grid {
public:
    Grid();
    ~Grid();

    // Initialize with RHI, shader, number of lines in each direction, and spacing between lines
    bool init(glint3d::RHI* rhi, Shader* shader, int lineCount = 50, float spacing = 1.0f);

    // Render the grid using view/projection matrices
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Cleanup RHI resources
    void cleanup();

private:
    glint3d::RHI* m_rhi;
    glint3d::BufferHandle m_vertexBuffer;
    glint3d::ShaderHandle m_shaderHandle;
    glint3d::PipelineHandle m_pipeline;
    Shader* m_shader;  // Legacy shader for compatibility
    int m_lineCount;
    float m_spacing;
    std::vector<glm::vec3> m_lineVertices;
};
