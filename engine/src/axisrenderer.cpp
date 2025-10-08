#include "axisrenderer.h"
#include <glint3d/rhi.h>

AxisRenderer::AxisRenderer()
    : m_vertexBuffer(glint3d::INVALID_HANDLE)
    , m_shader(glint3d::INVALID_HANDLE)
    , m_pipeline(glint3d::INVALID_HANDLE)
    , m_rhi(nullptr)
{}

void AxisRenderer::init(glint3d::RHI* rhi) {
    m_rhi = rhi;
    if (!m_rhi) return;
    float axisVertices[] = {
        // X-axis (Red)
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        // Y-axis (Green)
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        // Z-axis (Blue)
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f
    };

    // Create vertex buffer via RHI
    glint3d::BufferDesc bufferDesc;
    bufferDesc.type = glint3d::BufferType::Vertex;
    bufferDesc.usage = glint3d::BufferUsage::Static;
    bufferDesc.initialData = axisVertices;
    bufferDesc.size = sizeof(axisVertices);
    m_vertexBuffer = m_rhi->createBuffer(bufferDesc);

    // Create shader via RHI
    glint3d::ShaderDesc shaderDesc;
    shaderDesc.vertexSource = vertexShaderSource;
    shaderDesc.fragmentSource = fragmentShaderSource;
    m_shader = m_rhi->createShader(shaderDesc);

    // Create pipeline with vertex layout
    glint3d::PipelineDesc pipelineDesc;
    pipelineDesc.shader = m_shader;
    pipelineDesc.topology = glint3d::PrimitiveTopology::Lines;

    // Position attribute (location 0)
    glint3d::VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = glint3d::TextureFormat::RGB32F;
    posAttr.offset = 0;
    pipelineDesc.vertexAttributes.push_back(posAttr);

    // Color attribute (location 1)
    glint3d::VertexAttribute colorAttr;
    colorAttr.location = 1;
    colorAttr.binding = 0;
    colorAttr.format = glint3d::TextureFormat::RGB32F;
    colorAttr.offset = sizeof(float) * 3;
    pipelineDesc.vertexAttributes.push_back(colorAttr);

    // Vertex binding
    glint3d::VertexBinding binding;
    binding.binding = 0;
    binding.stride = sizeof(float) * 6;
    binding.perInstance = false;
    binding.buffer = m_vertexBuffer;
    pipelineDesc.vertexBindings.push_back(binding);

    m_pipeline = m_rhi->createPipeline(pipelineDesc);
}

void AxisRenderer::render(glm::mat4& modelMatrix, glm::mat4& viewMatrix, glm::mat4& projectionMatrix) {
    if (!m_rhi) return;

    // Bind pipeline
    m_rhi->bindPipeline(m_pipeline);

    // Set uniforms via RHI
    m_rhi->setUniformMat4("model", modelMatrix);
    m_rhi->setUniformMat4("view", viewMatrix);
    m_rhi->setUniformMat4("projection", projectionMatrix);

    // Draw via RHI
    glint3d::DrawDesc drawDesc;
    drawDesc.pipeline = m_pipeline;
    drawDesc.vertexBuffer = m_vertexBuffer;
    drawDesc.vertexCount = 6;
    drawDesc.instanceCount = 1;

    m_rhi->draw(drawDesc);
}

void AxisRenderer::cleanup() {
    if (!m_rhi) return;

    if (m_pipeline != glint3d::INVALID_HANDLE) {
        m_rhi->destroyPipeline(m_pipeline);
        m_pipeline = glint3d::INVALID_HANDLE;
    }
    if (m_shader != glint3d::INVALID_HANDLE) {
        m_rhi->destroyShader(m_shader);
        m_shader = glint3d::INVALID_HANDLE;
    }
    if (m_vertexBuffer != glint3d::INVALID_HANDLE) {
        m_rhi->destroyBuffer(m_vertexBuffer);
        m_vertexBuffer = glint3d::INVALID_HANDLE;
    }
}
