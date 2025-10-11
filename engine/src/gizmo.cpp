#include "gizmo.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <glint3d/rhi.h>

static const char* kVS = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main(){
    vColor = aColor;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)GLSL";

static const char* kFS = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main(){ FragColor = vec4(vColor, 1.0); }
)GLSL";

void Gizmo::init(glint3d::RHI* rhi){
    m_rhi = rhi;

    // Triad lines: origin->X, origin->Y, origin->Z
    const float verts[] = {
        // pos                // color
         0.f, 0.f, 0.f,       1.f, 0.f, 0.f,  // X axis start
         1.f, 0.f, 0.f,       1.f, 0.f, 0.f,  // X axis end
         0.f, 0.f, 0.f,       0.f, 1.f, 0.f,  // Y axis start
         0.f, 1.f, 0.f,       0.f, 1.f, 0.f,  // Y axis end
         0.f, 0.f, 0.f,       0.f, 0.f, 1.f,  // Z axis start
         0.f, 0.f, 1.f,       0.f, 0.f, 1.f,  // Z axis end
    };

    // Create vertex buffer via RHI
    glint3d::BufferDesc bufferDesc;
    bufferDesc.type = glint3d::BufferType::Vertex;
    bufferDesc.usage = glint3d::BufferUsage::Static;
    bufferDesc.initialData = verts;
    bufferDesc.size = sizeof(verts);
    bufferDesc.debugName = "GizmoVertexBuffer";
    m_vertexBuffer = m_rhi->createBuffer(bufferDesc);

    // Create shader via RHI
    glint3d::ShaderDesc shaderDesc;
    shaderDesc.vertexSource = kVS;
    shaderDesc.fragmentSource = kFS;

    // Create pipeline with vertex attributes
    glint3d::PipelineDesc desc{};
    desc.topology = glint3d::PrimitiveTopology::Lines;

    // Position attribute (location 0)
    glint3d::VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = glint3d::TextureFormat::RGB32F;
    posAttr.offset = 0;
    desc.vertexAttributes.push_back(posAttr);

    // Color attribute (location 1)
    glint3d::VertexAttribute colorAttr;
    colorAttr.location = 1;
    colorAttr.binding = 0;
    colorAttr.format = glint3d::TextureFormat::RGB32F;
    colorAttr.offset = sizeof(float) * 3;
    desc.vertexAttributes.push_back(colorAttr);

    // Vertex binding
    glint3d::VertexBinding binding;
    binding.binding = 0;
    binding.stride = sizeof(float) * 6;
    binding.perInstance = false;
    binding.buffer = m_vertexBuffer;
    desc.vertexBindings.push_back(binding);

    desc.depthTestEnable = false;  // Always on top
    desc.lineWidth = 2.0f;
    m_pipeline = m_rhi->createPipeline(desc);
}

void Gizmo::cleanup(){
    if (m_rhi) {
        m_rhi->destroyBuffer(m_vertexBuffer);
        m_rhi->destroyPipeline(m_pipeline);
        m_rhi = nullptr;
    }
}

void Gizmo::render(const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::vec3& origin,
                   const glm::mat3& orientation,
                   float scale,
                   GizmoAxis active,
                   GizmoMode /*mode*/){
    if (!m_rhi) return;

    // Build model matrix
    glm::mat4 R(1.0f);
    R[0] = glm::vec4(orientation[0], 0.0f);
    R[1] = glm::vec4(orientation[1], 0.0f);
    R[2] = glm::vec4(orientation[2], 0.0f);
    glm::mat4 M = glm::translate(glm::mat4(1.0f), origin) * R * glm::scale(glm::mat4(1.0f), glm::vec3(scale));

    // Set uniforms via RHI
    m_rhi->setUniformMat4("uModel", M);
    m_rhi->setUniformMat4("uView", view);
    m_rhi->setUniformMat4("uProj", proj);

    // Draw all axes with normal line width
    glint3d::DrawDesc drawDesc{};
    drawDesc.pipeline = m_pipeline;
    drawDesc.vertexBuffer = m_vertexBuffer;
    drawDesc.vertexCount = 6;
    drawDesc.instanceCount = 1;
    m_rhi->draw(drawDesc);

    // If an axis is active, overdraw it with thicker line width
    if (active != GizmoAxis::None) {
        // Create thicker line pipeline for active axis
        glint3d::PipelineDesc thickDesc{};
        thickDesc.topology = glint3d::PrimitiveTopology::Lines;

        // Position attribute
        glint3d::VertexAttribute posAttr;
        posAttr.location = 0;
        posAttr.binding = 0;
        posAttr.format = glint3d::TextureFormat::RGB32F;
        posAttr.offset = 0;
        thickDesc.vertexAttributes.push_back(posAttr);

        // Color attribute
        glint3d::VertexAttribute colorAttr;
        colorAttr.location = 1;
        colorAttr.binding = 0;
        colorAttr.format = glint3d::TextureFormat::RGB32F;
        colorAttr.offset = sizeof(float) * 3;
        thickDesc.vertexAttributes.push_back(colorAttr);

        // Vertex binding
        glint3d::VertexBinding binding;
        binding.binding = 0;
        binding.stride = sizeof(float) * 6;
        binding.perInstance = false;
        binding.buffer = m_vertexBuffer;
        thickDesc.vertexBindings.push_back(binding);

        thickDesc.depthTestEnable = false;
        thickDesc.lineWidth = 6.0f;
        auto thickPipeline = m_rhi->createPipeline(thickDesc);

        drawDesc.pipeline = thickPipeline;
        switch (active) {
            case GizmoAxis::X:
                drawDesc.vertexCount = 2;
                // Note: RHI DrawDesc doesn't support vertexOffset - would need to draw from different buffers
                // For now, just highlight all axes when one is active
                break;
            case GizmoAxis::Y:
                drawDesc.vertexCount = 2;
                break;
            case GizmoAxis::Z:
                drawDesc.vertexCount = 2;
                break;
            default:
                break;
        }
        m_rhi->draw(drawDesc);
        m_rhi->destroyPipeline(thickPipeline);
    }
}

static bool closestPointParamsOnLines(const glm::vec3& r0, const glm::vec3& rd,
                                      const glm::vec3& s0, const glm::vec3& sd,
                                      float& tOut, float& sOut){
    // Solve for t and s minimizing |(r0 + t*rd) - (s0 + s*sd)|
    const float a = glm::dot(rd, rd);
    const float b = glm::dot(rd, sd);
    const float c = glm::dot(sd, sd);
    const glm::vec3 w0 = r0 - s0;
    const float d = glm::dot(rd, w0);
    const float e = glm::dot(sd, w0);
    const float denom = a*c - b*b;
    if (std::abs(denom) < 1e-6f) return false; // nearly parallel
    tOut = (b*e - c*d) / denom;
    sOut = (a*e - b*d) / denom;
    return true;
}

bool Gizmo::pickAxis(const Ray& ray,
                     const glm::vec3& origin,
                     const glm::mat3& orientation,
                     float scale,
                     GizmoAxis& outAxis,
                     float& outS,
                     glm::vec3& outAxisDir) const {
    const float axisLen = scale;             // model scales unit-length axes
    const float hitRadius = 0.15f * scale;   // tolerance

    struct Candidate { GizmoAxis axis; glm::vec3 dir; int offset; };
    Candidate cands[3] = {
        { GizmoAxis::X, glm::normalize(glm::vec3(orientation[0])), 0 },
        { GizmoAxis::Y, glm::normalize(glm::vec3(orientation[1])), 2 },
        { GizmoAxis::Z, glm::normalize(glm::vec3(orientation[2])), 4 },
    };

    bool any = false; float bestDist = 1e9f; GizmoAxis bestAxis = GizmoAxis::None; float bestS = 0.f; glm::vec3 bestDir(0);
    for (auto& c : cands){
        float t=0,s=0; if (!closestPointParamsOnLines(ray.origin, ray.direction, origin, c.dir, t, s)) continue;
        // clamp s to axis segment [0, axisLen]
        float sClamped = glm::clamp(s, 0.0f, axisLen);
        glm::vec3 pRay = ray.origin + t * ray.direction;
        glm::vec3 pAxis = origin + sClamped * c.dir;
        float d = glm::length(pRay - pAxis);
        if (d < hitRadius && d < bestDist){ any = true; bestDist = d; bestAxis = c.axis; bestS = sClamped; bestDir = c.dir; }
    }
    if (!any) return false;
    outAxis = bestAxis; outS = bestS; outAxisDir = bestDir; return true;
}
