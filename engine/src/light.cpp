#include "light.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <glint3d/rhi.h>

std::string intToString(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

Light::Light()
{
}

Light::~Light()
{
    cleanup();
}

void Light::addLight(const glm::vec3& position, const glm::vec3& color, float intensity)
{
    LightSource newLight;
    newLight.type = LightType::POINT;
    newLight.position = position;
    newLight.color = color;
    newLight.intensity = intensity;
    newLight.enabled = true; // default on
    m_lights.push_back(newLight);
}

void Light::addDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    LightSource newLight;
    newLight.type = LightType::DIRECTIONAL;
    newLight.direction = glm::normalize(direction);
    newLight.position = glm::vec3(0.0f); // Not used for directional lights
    newLight.color = color;
    newLight.intensity = intensity;
    newLight.enabled = true;
    m_lights.push_back(newLight);
}

void Light::addSpotLight(const glm::vec3& position, const glm::vec3& direction,
                         const glm::vec3& color, float intensity,
                         float innerConeDeg, float outerConeDeg)
{
    LightSource s;
    s.type = LightType::SPOT;
    s.position = position;
    s.direction = glm::length(direction) > 1e-4f ? glm::normalize(direction) : glm::vec3(0, -1, 0);
    s.color = color;
    s.intensity = intensity;
    s.enabled = true;
    s.innerConeDeg = innerConeDeg;
    s.outerConeDeg = outerConeDeg;
    if (s.outerConeDeg < s.innerConeDeg) std::swap(s.outerConeDeg, s.innerConeDeg);
    m_lights.push_back(s);
}

size_t Light::getLightCount() const
{
    return m_lights.size();
}

// Initialize indicator geometry (cube for point lights, arrow for directional lights, cone for spot lights)
void Light::initIndicator(glint3d::RHI* rhi)
{
    m_rhi = rhi;
    if (!m_rhi) return;

    // Cube vertices for point lights
    float cubeVertices[] = {
        // positions for 12 triangles (36 vertices)
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,

        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,

         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,

        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f
    };

    // Create cube vertex buffer via RHI
    glint3d::BufferDesc cubeBufferDesc;
    cubeBufferDesc.type = glint3d::BufferType::Vertex;
    cubeBufferDesc.usage = glint3d::BufferUsage::Static;
    cubeBufferDesc.initialData = cubeVertices;
    cubeBufferDesc.size = sizeof(cubeVertices);
    cubeBufferDesc.debugName = "LightIndicatorCubeBuffer";
    m_indicatorVBO = m_rhi->createBuffer(cubeBufferDesc);

    // Arrow vertices for directional lights (shaft + arrowhead)
    float arrowVertices[] = {
        // Arrow shaft (thin cylinder approximated with lines)
        0.0f, 0.0f, 0.0f,    0.0f, 0.0f, -0.8f,  // Shaft line
        0.0f, 0.0f, -0.8f,   0.2f, 0.0f, -0.6f,  // Right arrowhead line
        0.0f, 0.0f, -0.8f,  -0.2f, 0.0f, -0.6f,  // Left arrowhead line
        0.0f, 0.0f, -0.8f,   0.0f, 0.2f, -0.6f,  // Top arrowhead line
        0.0f, 0.0f, -0.8f,   0.0f, -0.2f, -0.6f, // Bottom arrowhead line
    };

    // Create arrow vertex buffer via RHI
    glint3d::BufferDesc arrowBufferDesc;
    arrowBufferDesc.type = glint3d::BufferType::Vertex;
    arrowBufferDesc.usage = glint3d::BufferUsage::Static;
    arrowBufferDesc.initialData = arrowVertices;
    arrowBufferDesc.size = sizeof(arrowVertices);
    arrowBufferDesc.debugName = "LightIndicatorArrowBuffer";
    m_arrowVBO = m_rhi->createBuffer(arrowBufferDesc);

    // Spot light cone outline (unit, pointing down -Z)
    {
        std::vector<float> verts;
        auto push = [&](float x, float y, float z){ verts.push_back(x); verts.push_back(y); verts.push_back(z); };
        // Axis line
        push(0,0,0); push(0,0,-0.9f);
        // Circle at z = -0.9
        const int N = 24;
        const float r = 0.3f;
        for (int i=0;i<N;i++) {
            float a0 = (float)i * 6.2831853f / N;
            float a1 = (float)(i+1) * 6.2831853f / N;
            push(r*cosf(a0), r*sinf(a0), -0.9f); push(r*cosf(a1), r*sinf(a1), -0.9f);
        }
        // Spokes from origin to circle (every 90deg)
        for (int i=0;i<4;i++) {
            float a = (float)i * 6.2831853f / 4;
            push(0,0,0); push(r*cosf(a), r*sinf(a), -0.9f);
        }

        // Create spot cone vertex buffer via RHI
        glint3d::BufferDesc spotBufferDesc;
        spotBufferDesc.type = glint3d::BufferType::Vertex;
        spotBufferDesc.usage = glint3d::BufferUsage::Static;
        spotBufferDesc.initialData = verts.data();
        spotBufferDesc.size = verts.size() * sizeof(float);
        spotBufferDesc.debugName = "LightIndicatorSpotBuffer";
        m_spotVBO = m_rhi->createBuffer(spotBufferDesc);
    }

    // Create shader via RHI
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 indicatorColor;
        void main()
        {
            FragColor = vec4(indicatorColor, 1.0);
        }
    )";

    glint3d::ShaderDesc shaderDesc;
    shaderDesc.vertexSource = vertexShaderSource;
    shaderDesc.fragmentSource = fragmentShaderSource;
    m_indicatorShader = m_rhi->createShader(shaderDesc);

    // Create pipeline for triangles (cube)
    glint3d::PipelineDesc cubePipeDesc{};
    cubePipeDesc.shader = m_indicatorShader;
    cubePipeDesc.topology = glint3d::PrimitiveTopology::Triangles;

    glint3d::VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = glint3d::TextureFormat::RGB32F;
    posAttr.offset = 0;
    cubePipeDesc.vertexAttributes.push_back(posAttr);

    glint3d::VertexBinding binding;
    binding.binding = 0;
    binding.stride = sizeof(float) * 3;
    binding.perInstance = false;
    binding.buffer = m_indicatorVBO;
    cubePipeDesc.vertexBindings.push_back(binding);

    cubePipeDesc.depthTestEnable = true;
    m_cubePipeline = m_rhi->createPipeline(cubePipeDesc);

    // Create pipeline for lines (arrow + spot cone)
    glint3d::PipelineDesc linePipeDesc{};
    linePipeDesc.shader = m_indicatorShader;
    linePipeDesc.topology = glint3d::PrimitiveTopology::Lines;
    linePipeDesc.vertexAttributes.push_back(posAttr);

    glint3d::VertexBinding lineBinding;
    lineBinding.binding = 0;
    lineBinding.stride = sizeof(float) * 3;
    lineBinding.perInstance = false;
    lineBinding.buffer = m_arrowVBO;  // Will be overridden in draw calls
    linePipeDesc.vertexBindings.push_back(lineBinding);

    linePipeDesc.depthTestEnable = true;
    linePipeDesc.lineWidth = 3.0f;
    m_linePipeline = m_rhi->createPipeline(linePipeDesc);
}


// Render visual indicators for each light using RHI
void Light::renderIndicators(const glm::mat4& view, const glm::mat4& projection, int selectedIndex) const
{
    if (!m_rhi || m_indicatorShader == glint3d::INVALID_HANDLE)
        return; // Not initialized

    // Set uniforms via RHI
    m_rhi->setUniformMat4("view", view);
    m_rhi->setUniformMat4("projection", projection);

    for (size_t i = 0; i < m_lights.size(); i++) {
        glm::mat4 model(1.0f);

        if (m_lights[i].type == LightType::POINT) {
            // Render cube at light position
            model = glm::translate(model, m_lights[i].position);
            m_rhi->setUniformMat4("model", model);
            m_rhi->setUniformVec3("indicatorColor", m_lights[i].color);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = m_cubePipeline;
            drawDesc.vertexBuffer = m_indicatorVBO;
            drawDesc.vertexCount = 36;
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);
        }
        else if (m_lights[i].type == LightType::DIRECTIONAL) {
            // Calculate transformation to orient arrow in light direction
            glm::vec3 forward(0.0f, 0.0f, -1.0f); // Arrow points down -Z
            glm::vec3 lightDir = glm::normalize(m_lights[i].direction);

            // Create rotation matrix to align arrow with light direction
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                // 180 degree rotation needed
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }

            m_rhi->setUniformMat4("model", model);
            m_rhi->setUniformVec3("indicatorColor", m_lights[i].color);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = m_linePipeline;
            drawDesc.vertexBuffer = m_arrowVBO;
            drawDesc.vertexCount = 10; // 5 lines * 2 vertices = 10
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);
        }
        else if (m_lights[i].type == LightType::SPOT) {
            // Translate to light position, orient cone along light direction
            model = glm::translate(model, m_lights[i].position);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[i].direction);
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }

            m_rhi->setUniformMat4("model", model);
            m_rhi->setUniformVec3("indicatorColor", m_lights[i].color);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = m_linePipeline;
            drawDesc.vertexBuffer = m_spotVBO;
            drawDesc.vertexCount = 58; // 2 (axis) + 48 (circle) + 8 (spokes)
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);
        }
    }

    // Draw selection highlight
    if (selectedIndex >= 0 && selectedIndex < (int)m_lights.size()) {
        glm::vec3 selColor(0.2f, 0.7f, 1.0f);
        m_rhi->setUniformVec3("indicatorColor", selColor);

        if (m_lights[selectedIndex].type == LightType::POINT) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_lights[selectedIndex].position)
                            * glm::scale(glm::mat4(1.0f), glm::vec3(1.3f));
            m_rhi->setUniformMat4("model", model);

            // Create wireframe pipeline for selection highlight
            glint3d::PipelineDesc wireDesc{};
            wireDesc.shader = m_indicatorShader;
            wireDesc.topology = glint3d::PrimitiveTopology::Triangles;
            wireDesc.polygonMode = glint3d::PolygonMode::Line;
            wireDesc.lineWidth = 2.0f;

            glint3d::VertexAttribute posAttr;
            posAttr.location = 0;
            posAttr.binding = 0;
            posAttr.format = glint3d::TextureFormat::RGB32F;
            posAttr.offset = 0;
            wireDesc.vertexAttributes.push_back(posAttr);

            glint3d::VertexBinding binding;
            binding.binding = 0;
            binding.stride = sizeof(float) * 3;
            binding.perInstance = false;
            binding.buffer = m_indicatorVBO;
            wireDesc.vertexBindings.push_back(binding);

            wireDesc.depthTestEnable = true;
            auto wirePipeline = m_rhi->createPipeline(wireDesc);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = wirePipeline;
            drawDesc.vertexBuffer = m_indicatorVBO;
            drawDesc.vertexCount = 36;
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);

            m_rhi->destroyPipeline(wirePipeline);
        } else if (m_lights[selectedIndex].type == LightType::DIRECTIONAL) {
            // Highlight directional light with thicker lines
            glm::mat4 model(1.0f);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[selectedIndex].direction);

            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }

            m_rhi->setUniformMat4("model", model);

            // Create thicker line pipeline for selection
            glint3d::PipelineDesc thickDesc{};
            thickDesc.shader = m_indicatorShader;
            thickDesc.topology = glint3d::PrimitiveTopology::Lines;
            thickDesc.lineWidth = 5.0f;

            glint3d::VertexAttribute posAttr;
            posAttr.location = 0;
            posAttr.binding = 0;
            posAttr.format = glint3d::TextureFormat::RGB32F;
            posAttr.offset = 0;
            thickDesc.vertexAttributes.push_back(posAttr);

            glint3d::VertexBinding binding;
            binding.binding = 0;
            binding.stride = sizeof(float) * 3;
            binding.perInstance = false;
            binding.buffer = m_arrowVBO;
            thickDesc.vertexBindings.push_back(binding);

            thickDesc.depthTestEnable = true;
            auto thickPipeline = m_rhi->createPipeline(thickDesc);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = thickPipeline;
            drawDesc.vertexBuffer = m_arrowVBO;
            drawDesc.vertexCount = 10;
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);

            m_rhi->destroyPipeline(thickPipeline);
        } else if (m_lights[selectedIndex].type == LightType::SPOT) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, m_lights[selectedIndex].position);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[selectedIndex].direction);
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }

            m_rhi->setUniformMat4("model", model);

            // Create thicker line pipeline for selection
            glint3d::PipelineDesc thickDesc{};
            thickDesc.shader = m_indicatorShader;
            thickDesc.topology = glint3d::PrimitiveTopology::Lines;
            thickDesc.lineWidth = 4.0f;

            glint3d::VertexAttribute posAttr;
            posAttr.location = 0;
            posAttr.binding = 0;
            posAttr.format = glint3d::TextureFormat::RGB32F;
            posAttr.offset = 0;
            thickDesc.vertexAttributes.push_back(posAttr);

            glint3d::VertexBinding binding;
            binding.binding = 0;
            binding.stride = sizeof(float) * 3;
            binding.perInstance = false;
            binding.buffer = m_spotVBO;
            thickDesc.vertexBindings.push_back(binding);

            thickDesc.depthTestEnable = true;
            auto thickPipeline = m_rhi->createPipeline(thickDesc);

            glint3d::DrawDesc drawDesc{};
            drawDesc.pipeline = thickPipeline;
            drawDesc.vertexBuffer = m_spotVBO;
            drawDesc.vertexCount = 58;
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);

            m_rhi->destroyPipeline(thickPipeline);
        }
    }
}

const LightSource* Light::getFirstLight() const
{
    if (m_lights.empty())
        return nullptr;
    return &m_lights[0];
}

bool Light::removeLightAt(size_t index)
{
    if (index >= m_lights.size()) return false;
    m_lights.erase(m_lights.begin() + index);
    return true;
}

void Light::cleanup()
{
    if (m_rhi) {
        if (m_indicatorVBO != glint3d::INVALID_HANDLE) {
            m_rhi->destroyBuffer(m_indicatorVBO);
            m_indicatorVBO = {};
        }
        if (m_arrowVBO != glint3d::INVALID_HANDLE) {
            m_rhi->destroyBuffer(m_arrowVBO);
            m_arrowVBO = {};
        }
        if (m_spotVBO != glint3d::INVALID_HANDLE) {
            m_rhi->destroyBuffer(m_spotVBO);
            m_spotVBO = {};
        }
        if (m_indicatorShader != glint3d::INVALID_HANDLE) {
            m_rhi->destroyShader(m_indicatorShader);
            m_indicatorShader = {};
        }
        if (m_cubePipeline != glint3d::INVALID_HANDLE) {
            m_rhi->destroyPipeline(m_cubePipeline);
            m_cubePipeline = {};
        }
        if (m_linePipeline != glint3d::INVALID_HANDLE) {
            m_rhi->destroyPipeline(m_linePipeline);
            m_linePipeline = {};
        }
        m_rhi = nullptr;
    }
}
