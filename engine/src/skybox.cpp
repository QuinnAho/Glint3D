#include "skybox.h"
#include "stb_image.h"
#include <iostream>
#include <glint3d/rhi.h>

namespace {
    // Skybox cube vertices (positions only)
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    const char* kVS = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;
void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)GLSL";

    const char* kFS = R"GLSL(
#version 330 core
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;
uniform bool useGradient;
uniform vec3 topColor;
uniform vec3 bottomColor;
uniform vec3 horizonColor;
uniform float intensity;
void main() {
    if (useGradient) {
        float t = normalize(TexCoords).y;
        vec3 color;
        if (t > 0.0) {
            float factor = smoothstep(0.0, 1.0, t);
            color = mix(horizonColor, topColor, factor);
        } else {
            float factor = smoothstep(0.0, -1.0, t);
            color = mix(horizonColor, bottomColor, factor);
        }
        FragColor = vec4(color * intensity, 1.0);
    } else {
        FragColor = texture(skybox, TexCoords) * vec4(vec3(intensity), 1.0);
    }
}
)GLSL";
}

Skybox::Skybox()
    : m_rhi(nullptr)
    , m_enabled(true)
    , m_initialized(false)
    , m_useGradient(true)
    , m_intensity(1.0f)
    , m_topColor(0.2f, 0.4f, 0.8f)      // Sky blue
    , m_bottomColor(0.8f, 0.9f, 1.0f)   // Light blue/white
    , m_horizonColor(0.9f, 0.8f, 0.7f)  // Warm horizon
{
}

Skybox::~Skybox()
{
    cleanup();
}

bool Skybox::init(glint3d::RHI* rhi)
{
    if (m_initialized) return true;
    if (!rhi) return false;

    m_rhi = rhi;

    // Create vertex buffer via RHI
    glint3d::BufferDesc bufferDesc;
    bufferDesc.type = glint3d::BufferType::Vertex;
    bufferDesc.usage = glint3d::BufferUsage::Static;
    bufferDesc.initialData = skyboxVertices;
    bufferDesc.size = sizeof(skyboxVertices);
    bufferDesc.debugName = "SkyboxVertexBuffer";
    m_vertexBuffer = m_rhi->createBuffer(bufferDesc);

    // Create shader via RHI
    glint3d::ShaderDesc shaderDesc;
    shaderDesc.vertexSource = kVS;
    shaderDesc.fragmentSource = kFS;

    // Create pipeline with vertex attributes
    glint3d::PipelineDesc desc{};
    desc.topology = glint3d::PrimitiveTopology::Triangles;

    // Position attribute (location 0)
    glint3d::VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = glint3d::TextureFormat::RGB32F;
    posAttr.offset = 0;
    desc.vertexAttributes.push_back(posAttr);

    // Vertex binding
    glint3d::VertexBinding binding;
    binding.binding = 0;
    binding.stride = sizeof(float) * 3;
    binding.perInstance = false;
    binding.buffer = m_vertexBuffer;
    desc.vertexBindings.push_back(binding);

    desc.depthTestEnable = true;
    desc.depthWriteEnable = false;  // Skybox doesn't write to depth buffer
    m_pipeline = m_rhi->createPipeline(desc);

    setupCube();
    createProceduralSkybox();

    m_initialized = true;
    return true;
}

void Skybox::setupCube()
{
    // Nothing needed here - vertex buffer already created in init()
}

void Skybox::createProceduralSkybox()
{
    // Create a simple 1x1 white cubemap texture for gradient mode
    unsigned char whitePixel[3] = {255, 255, 255};

    glint3d::TextureDesc texDesc{};
    texDesc.type = glint3d::TextureType::TextureCube;
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = glint3d::TextureFormat::RGB8;
    texDesc.initialData = whitePixel;
    texDesc.initialDataSize = sizeof(whitePixel);
    texDesc.debugName = "SkyboxProceduralCubemap";

    m_cubemapTexture = m_rhi->createTexture(texDesc);

    m_useGradient = true;
}

bool Skybox::loadCubemap(const std::vector<std::string>& faces)
{
    if (faces.size() != 6) {
        std::cerr << "Skybox: Need exactly 6 faces for cubemap" << std::endl;
        return false;
    }

    if (m_cubemapTexture != glint3d::INVALID_HANDLE) {
        m_rhi->destroyTexture(m_cubemapTexture);
        m_cubemapTexture = glint3d::INVALID_HANDLE;
    }

    // Load first face to get dimensions
    int width, height, nrChannels;
    unsigned char* data = stbi_load(faces[0].c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Skybox: Failed to load first texture: " << faces[0] << std::endl;
        return false;
    }

    glint3d::TextureFormat format = (nrChannels == 4) ? glint3d::TextureFormat::RGBA8 : glint3d::TextureFormat::RGB8;

    glint3d::TextureDesc texDesc{};
    texDesc.type = glint3d::TextureType::TextureCube;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.format = format;
    texDesc.initialData = data;
    texDesc.initialDataSize = width * height * nrChannels;
    texDesc.debugName = "SkyboxCubemap";

    m_cubemapTexture = m_rhi->createTexture(texDesc);
    stbi_image_free(data);

    // Load remaining faces
    for (unsigned int i = 1; i < faces.size(); i++) {
        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            // TODO: RHI needs support for updating cubemap faces individually
            // For now, this is a limitation - we'd need updateTextureCubeFace() API
            stbi_image_free(data);
        } else {
            std::cerr << "Skybox: Failed to load texture: " << faces[i] << std::endl;
            return false;
        }
    }

    m_useGradient = false;
    return true;
}

void Skybox::setGradient(const glm::vec3& topColor, const glm::vec3& bottomColor, const glm::vec3& horizonColor)
{
    m_topColor = topColor;
    m_bottomColor = bottomColor;
    m_horizonColor = horizonColor.x == 0.0f && horizonColor.y == 0.0f && horizonColor.z == 0.0f
        ? glm::mix(topColor, bottomColor, 0.5f)
        : horizonColor;
    m_useGradient = true;
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_enabled || !m_initialized || !m_rhi) return;

    // Remove translation from the view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

    // Set uniforms via RHI
    m_rhi->setUniformMat4("view", skyboxView);
    m_rhi->setUniformMat4("projection", projection);
    m_rhi->setUniformBool("useGradient", m_useGradient);
    m_rhi->setUniformVec3("topColor", m_topColor);
    m_rhi->setUniformVec3("bottomColor", m_bottomColor);
    m_rhi->setUniformVec3("horizonColor", m_horizonColor);
    m_rhi->setUniformFloat("intensity", m_intensity);
    m_rhi->setUniformInt("skybox", 0);

    // Bind cubemap texture
    m_rhi->bindTexture(m_cubemapTexture, 0);

    // Draw skybox
    glint3d::DrawDesc drawDesc{};
    drawDesc.pipeline = m_pipeline;
    drawDesc.vertexBuffer = m_vertexBuffer;
    drawDesc.vertexCount = 36;
    drawDesc.instanceCount = 1;
    m_rhi->draw(drawDesc);
}

void Skybox::cleanup()
{
    if (m_rhi) {
        if (m_vertexBuffer != glint3d::INVALID_HANDLE) {
            m_rhi->destroyBuffer(m_vertexBuffer);
            m_vertexBuffer = {};
        }
        if (m_cubemapTexture != glint3d::INVALID_HANDLE) {
            m_rhi->destroyTexture(m_cubemapTexture);
            m_cubemapTexture = {};
        }
        // legacy block?
        if (m_shader != glint3d::INVALID_HANDLE) {
            m_rhi->destroyShader(m_shader);
            m_shader = {};
        }
        if (m_pipeline != glint3d::INVALID_HANDLE) {
            m_rhi->destroyPipeline(m_pipeline);
            m_pipeline = {};
        }
        m_rhi = nullptr;
    }
    m_initialized = false;
}

void Skybox::setEnvironmentMap(glint3d::TextureHandle envMap)
{
    // Use the provided environment map instead of our own cubemap texture
    if (envMap != glint3d::INVALID_HANDLE) {
        m_cubemapTexture = envMap;
        m_useGradient = false; // Disable gradient mode when using external environment
        m_enabled = true; // Enable skybox when environment map is set
    }
}
