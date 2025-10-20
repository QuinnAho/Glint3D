#ifndef AXIS_RENDERER_H
#define AXIS_RENDERER_H

#include <glm/glm.hpp>
#include <glint3d/rhi_types.h>

// Forward declarations
namespace glint3d { class RHI; }
class TransformManager;

class AxisRenderer {
private:
    glint3d::BufferHandle m_vertexBuffer;
    glint3d::ShaderHandle m_shader; //to-do remove
    glint3d::PipelineHandle m_pipeline;
    glint3d::RHI* m_rhi;

    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    layout(std140, binding = 0) uniform TransformBlock {
        mat4 model;
        mat4 view;
        mat4 projection;
        mat4 lightSpaceMatrix;
    };
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        ourColor = aColor;
    })";

    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(ourColor, 1.0);
    })";

public:
    AxisRenderer();
    void init(glint3d::RHI* rhi);
    void render(const glm::mat4& modelMatrix,
                const glm::mat4& viewMatrix,
                const glm::mat4& projectionMatrix,
                TransformManager& transforms);
    void cleanup();
};

#endif
