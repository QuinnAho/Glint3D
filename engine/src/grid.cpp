// Machine Summary Block
// {"file":"engine/src/grid.cpp","purpose":"Draws the ground grid visualization using RHI-managed buffers and shaders","exports":["Grid"],"depends_on":["glint3d::RHI","glm"],"notes":["Initializes line vertex data and stores pipeline plus shader handles"]}
// Human Summary: Sets up vertex data for the editor grid and renders it via the RHI with configurable spacing and line count.

#include "grid.h"
#include "managers/transform_manager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <glint3d/rhi.h>

Grid::Grid()
    : m_rhi(nullptr)
    , m_vertexBuffer(glint3d::INVALID_HANDLE)
    , m_shaderHandle(glint3d::INVALID_HANDLE)
    , m_pipeline(glint3d::INVALID_HANDLE)
    , m_lineCount(200)
    , m_spacing(1.0f)
{}

Grid::~Grid() {
    cleanup();
}

bool Grid::init(glint3d::RHI* rhi, int lineCount, float spacing)
{
    m_rhi = rhi;
    m_lineCount = lineCount;
    m_spacing = spacing;

    if (!m_rhi) return false;

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

    // Create vertex buffer via RHI
    glint3d::BufferDesc bufferDesc;
    bufferDesc.type = glint3d::BufferType::Vertex;
    bufferDesc.usage = glint3d::BufferUsage::Static;
    bufferDesc.initialData = lines.data();
    bufferDesc.size = lines.size() * sizeof(glm::vec3);
    m_vertexBuffer = m_rhi->createBuffer(bufferDesc);

    // Create shader via RHI
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    })";

    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 gridColor;
    void main() {
        FragColor = vec4(gridColor, 1.0);
    })";

    glint3d::ShaderDesc shaderDesc;
    shaderDesc.vertexSource = vertexShaderSource;
    shaderDesc.fragmentSource = fragmentShaderSource;
    m_shaderHandle = m_rhi->createShader(shaderDesc);

    // Create pipeline
    glint3d::PipelineDesc pipelineDesc;
    pipelineDesc.shader = m_shaderHandle;
    pipelineDesc.topology = glint3d::PrimitiveTopology::Lines;

    // Position attribute (location 0)
    glint3d::VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = glint3d::TextureFormat::RGB32F;
    posAttr.offset = 0;
    pipelineDesc.vertexAttributes.push_back(posAttr);

    // Vertex binding
    glint3d::VertexBinding binding;
    binding.binding = 0;
    binding.stride = sizeof(glm::vec3);
    binding.perInstance = false;
    binding.buffer = m_vertexBuffer;
    pipelineDesc.vertexBindings.push_back(binding);

    m_pipeline = m_rhi->createPipeline(pipelineDesc);

    return true;
}

void Grid::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_rhi) return;

    // Bind pipeline
    m_rhi->bindPipeline(m_pipeline);

    glm::mat4 model = glm::mat4(1.0f);

    // Set uniforms via RHI
    m_rhi->setUniformMat4("model", model);
    m_rhi->setUniformMat4("view", view);
    m_rhi->setUniformMat4("projection", projection);
    m_rhi->setUniformVec3("gridColor", Colors::LightGray);

    // Draw via RHI
    glint3d::DrawDesc drawDesc;
    drawDesc.pipeline = m_pipeline;
    drawDesc.vertexBuffer = m_vertexBuffer;
    drawDesc.vertexCount = static_cast<uint32_t>(m_lineVertices.size());
    drawDesc.instanceCount = 1;

    m_rhi->draw(drawDesc);
}

void Grid::cleanup()
{
    if (!m_rhi) return;

    if (m_pipeline != glint3d::INVALID_HANDLE) {
        m_rhi->destroyPipeline(m_pipeline);
        m_pipeline = glint3d::INVALID_HANDLE;
    }
    if (m_shaderHandle != glint3d::INVALID_HANDLE) {
        m_rhi->destroyShader(m_shaderHandle);
        m_shaderHandle = glint3d::INVALID_HANDLE;
    }
    if (m_vertexBuffer != glint3d::INVALID_HANDLE) {
        m_rhi->destroyBuffer(m_vertexBuffer);
        m_vertexBuffer = glint3d::INVALID_HANDLE;
    }
}
