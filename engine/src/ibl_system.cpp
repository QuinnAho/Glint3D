// Machine Summary Block
// {"file":"engine/src/ibl_system.cpp","purpose":"Implements image-based lighting asset preparation and GPU pipeline bindings","exports":["IblSystem"],"depends_on":["glint3d::RHI","glint3d::TextureSlots","glm"],"notes":["Generates cubemaps, irradiance, prefilter, and BRDF LUT using the runtime RHI"]}
// Human Summary: Handles loading HDR environments and generating the derived cubemaps plus LUTs, managing GPU state via the RHI.

#include "ibl_system.h"
#include "image_io.h"
#include "path_utils.h"
#include <iostream>
#include <cmath>
#include <glint3d/texture_slots.h>

using namespace glint3d;
namespace Slots = glint3d::TextureSlots;

namespace {
    // Cube vertices for environment mapping
    float cubeVertices[] = {
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

    // Quad vertices for BRDF LUT generation
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
}

IBLSystem::IBLSystem()
    : m_rhi(nullptr)
    , m_environmentMap(glint3d::INVALID_HANDLE)
    , m_irradianceMap(glint3d::INVALID_HANDLE)
    , m_prefilterMap(glint3d::INVALID_HANDLE)
    , m_brdfLUT(glint3d::INVALID_HANDLE)
    , m_captureFramebuffer(glint3d::INVALID_HANDLE)
    , m_equirectToCubemapShader(glint3d::INVALID_HANDLE)
    , m_irradianceShader(glint3d::INVALID_HANDLE)
    , m_prefilterShader(glint3d::INVALID_HANDLE)
    , m_brdfShader(glint3d::INVALID_HANDLE)
    , m_cubeBuffer(glint3d::INVALID_HANDLE)
    , m_quadBuffer(glint3d::INVALID_HANDLE)
    , m_cubePipeline(glint3d::INVALID_HANDLE)
    , m_quadPipeline(glint3d::INVALID_HANDLE)
    , m_intensity(1.0f)
    , m_initialized(false)
{
}

IBLSystem::~IBLSystem()
{
    cleanup();
}

bool IBLSystem::init(glint3d::RHI* rhi)
{
    if (m_initialized) return true;
    if (!rhi) return false;

    m_rhi = rhi;

    // Note: Legacy capture framebuffer removed - now using on-demand RenderTargets per cubemap face
    // Each IBL generation method creates temporary render targets as needed

    // Create shaders
    createShaders();

    // Setup geometry
    setupCube();
    setupQuad();

    m_initialized = true;
    return true;
}

void IBLSystem::createShaders()
{
    using namespace glint3d;

    // Create RHI shaders (for future use)
    // Also create legacy Shader* for IBL generation (Phase 3 TODO)

    // Equirectangular to cubemap shader
    const char* equirectVertexShader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n";

    const char* equirectFragmentShader =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform sampler2D equirectangularMap;\n"
        "const vec2 invAtan = vec2(0.1591, 0.3183);\n"
        "vec2 SampleSphericalMap(vec3 v) {\n"
        "    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));\n"
        "    uv *= invAtan;\n"
        "    uv += 0.5;\n"
        "    return uv;\n"
        "}\n"
        "void main() {\n"
        "    vec2 uv = SampleSphericalMap(normalize(WorldPos));\n"
        "    vec3 color = texture(equirectangularMap, uv).rgb;\n"
        "    FragColor = vec4(color, 1.0);\n"
        "}\n";

    ShaderDesc equirectDesc{};
    equirectDesc.vertexSource = equirectVertexShader;
    equirectDesc.fragmentSource = equirectFragmentShader;
    equirectDesc.debugName = "equirect_to_cubemap";
    m_equirectToCubemapShader = m_rhi->createShader(equirectDesc);

    // Irradiance convolution shader
    const char* irradianceVertexShader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n";

    const char* irradianceFragmentShader =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform samplerCube environmentMap;\n"
        "const float PI = 3.14159265359;\n"
        "void main() {\n"
        "    vec3 N = normalize(WorldPos);\n"
        "    vec3 irradiance = vec3(0.0);\n"
        "    vec3 up    = vec3(0.0, 1.0, 0.0);\n"
        "    vec3 right = normalize(cross(up, N));\n"
        "    up         = normalize(cross(N, right));\n"
        "    float sampleDelta = 0.025;\n"
        "    float nrSamples = 0.0;\n"
        "    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {\n"
        "        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {\n"
        "            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));\n"
        "            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;\n"
        "            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);\n"
        "            nrSamples++;\n"
        "        }\n"
        "    }\n"
        "    irradiance = PI * irradiance * (1.0 / float(nrSamples));\n"
        "    FragColor = vec4(irradiance, 1.0);\n"
        "}\n";

    ShaderDesc irradianceDesc{};
    irradianceDesc.vertexSource = irradianceVertexShader;
    irradianceDesc.fragmentSource = irradianceFragmentShader;
    irradianceDesc.debugName = "irradiance_convolution";
    m_irradianceShader = m_rhi->createShader(irradianceDesc);

    // Prefilter shader
    const char* prefilterVertexShader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n";

    const char* prefilterFragmentShader =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform samplerCube environmentMap;\n"
        "uniform float roughness;\n"
        "const float PI = 3.14159265359;\n"
        "float DistributionGGX(vec3 N, vec3 H, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float a2 = a*a;\n"
        "    float NdotH = max(dot(N, H), 0.0);\n"
        "    float NdotH2 = NdotH*NdotH;\n"
        "    float nom   = a2;\n"
        "    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
        "    denom = PI * denom * denom;\n"
        "    return nom / denom;\n"
        "}\n"
        "float RadicalInverse_VdC(uint bits) {\n"
        "    bits = (bits << 16u) | (bits >> 16u);\n"
        "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
        "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
        "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
        "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
        "    return float(bits) * 2.3283064365386963e-10;\n"
        "}\n"
        "vec2 Hammersley(uint i, uint N) {\n"
        "    return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
        "}\n"
        "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float phi = 2.0 * PI * Xi.x;\n"
        "    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
        "    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
        "    vec3 H;\n"
        "    H.x = cos(phi) * sinTheta;\n"
        "    H.y = sin(phi) * sinTheta;\n"
        "    H.z = cosTheta;\n"
        "    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
        "    vec3 tangent   = normalize(cross(up, N));\n"
        "    vec3 bitangent = cross(N, tangent);\n"
        "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
        "    return normalize(sampleVec);\n"
        "}\n"
        "void main() {\n"
        "    vec3 N = normalize(WorldPos);\n"
        "    vec3 R = N;\n"
        "    vec3 V = R;\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    vec3 prefilteredColor = vec3(0.0);\n"
        "    float totalWeight = 0.0;\n"
        "    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(dot(N, L), 0.0);\n"
        "        if(NdotL > 0.0) {\n"
        "            float D   = DistributionGGX(N, H, roughness);\n"
        "            float NdotH = max(dot(N, H), 0.0);\n"
        "            float HdotV = max(dot(H, V), 0.0);\n"
        "            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;\n"
        "            float resolution = 512.0;\n"
        "            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);\n"
        "            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);\n"
        "            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);\n"
        "            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;\n"
        "            totalWeight      += NdotL;\n"
        "        }\n"
        "    }\n"
        "    prefilteredColor = prefilteredColor / totalWeight;\n"
        "    FragColor = vec4(prefilteredColor, 1.0);\n"
        "}\n";

    ShaderDesc prefilterDesc{};
    prefilterDesc.vertexSource = prefilterVertexShader;
    prefilterDesc.fragmentSource = prefilterFragmentShader;
    prefilterDesc.debugName = "prefilter_envmap";
    m_prefilterShader = m_rhi->createShader(prefilterDesc);

    // BRDF LUT shader
    const char* brdfVertexShader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "void main() {\n"
        "    TexCoords = aTexCoords;\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "}\n";

    const char* brdfFragmentShader =
        "#version 330 core\n"
        "out vec2 FragColor;\n"
        "in vec2 TexCoords;\n"
        "const float PI = 3.14159265359;\n"
        "float RadicalInverse_VdC(uint bits) {\n"
        "    bits = (bits << 16u) | (bits >> 16u);\n"
        "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
        "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
        "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
        "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
        "    return float(bits) * 2.3283064365386963e-10;\n"
        "}\n"
        "vec2 Hammersley(uint i, uint N) {\n"
        "    return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
        "}\n"
        "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float phi = 2.0 * PI * Xi.x;\n"
        "    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
        "    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
        "    vec3 H;\n"
        "    H.x = cos(phi) * sinTheta;\n"
        "    H.y = sin(phi) * sinTheta;\n"
        "    H.z = cosTheta;\n"
        "    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
        "    vec3 tangent   = normalize(cross(up, N));\n"
        "    vec3 bitangent = cross(N, tangent);\n"
        "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
        "    return normalize(sampleVec);\n"
        "}\n"
        "float GeometrySchlickGGX(float NdotV, float roughness) {\n"
        "    float a = roughness;\n"
        "    float k = (a * a) / 2.0;\n"
        "    float nom   = NdotV;\n"
        "    float denom = NdotV * (1.0 - k) + k;\n"
        "    return nom / denom;\n"
        "}\n"
        "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
        "    float NdotV = max(dot(N, V), 0.0);\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    float ggx1 = GeometrySchlickGGX(NdotV, roughness);\n"
        "    float ggx2 = GeometrySchlickGGX(NdotL, roughness);\n"
        "    return ggx1 * ggx2;\n"
        "}\n"
        "vec2 IntegrateBRDF(float NdotV, float roughness) {\n"
        "    vec3 V;\n"
        "    V.x = sqrt(1.0 - NdotV*NdotV);\n"
        "    V.y = 0.0;\n"
        "    V.z = NdotV;\n"
        "    float A = 0.0;\n"
        "    float B = 0.0;\n"
        "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(L.z, 0.0);\n"
        "        float NdotH = max(H.z, 0.0);\n"
        "        float VdotH = max(dot(V, H), 0.0);\n"
        "        if(NdotL > 0.0) {\n"
        "            float G = GeometrySmith(N, V, L, roughness);\n"
        "            float G_Vis = (G * VdotH) / (NdotH * NdotV);\n"
        "            float Fc = pow(1.0 - VdotH, 5.0);\n"
        "            A += (1.0 - Fc) * G_Vis;\n"
        "            B += Fc * G_Vis;\n"
        "        }\n"
        "    }\n"
        "    A /= float(SAMPLE_COUNT);\n"
        "    B /= float(SAMPLE_COUNT);\n"
        "    return vec2(A, B);\n"
        "}\n"
        "void main() {\n"
        "    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);\n"
        "    FragColor = integratedBRDF;\n"
        "}\n";

    ShaderDesc brdfDesc{};
    brdfDesc.vertexSource = brdfVertexShader;
    brdfDesc.fragmentSource = brdfFragmentShader;
    brdfDesc.debugName = "brdf_lut";
    m_brdfShader = m_rhi->createShader(brdfDesc);
}

glint3d::TextureHandle IBLSystem::loadHDRTexture(const std::string& path)
{
    using namespace glint3d;

    // Resolve path to handle different working directories
    std::string resolvedPath = PathUtils::resolveAssetPath(path);
    if (resolvedPath.empty()) {
        std::cerr << "Failed to resolve HDR/EXR path: " << path << std::endl;
        return INVALID_HANDLE;
    }

    ImageIO::ImageDataFloat img;
    if (!ImageIO::LoadImageFloat(resolvedPath, img, /*flipY=*/true)) {
        std::cerr << "Failed to load HDR/EXR image: " << resolvedPath << " (original: " << path << ")" << std::endl;
        return INVALID_HANDLE;
    }

    // Create texture descriptor
    TextureDesc desc{};
    desc.type = TextureType::Texture2D;
    desc.width = img.width;
    desc.height = img.height;
    desc.format = (img.channels == 3) ? TextureFormat::RGB16F : TextureFormat::RGBA16F;
    desc.generateMips = false;
    desc.initialData = img.pixels.data();
    desc.initialDataSize = img.pixels.size() * sizeof(float);

    TextureHandle hdrTexture = m_rhi->createTexture(desc);
    return hdrTexture;
}

bool IBLSystem::loadHDREnvironment(const std::string& hdrPath)
{
    using namespace glint3d;

    if (!m_initialized) {
        std::cerr << "IBL System not initialized" << std::endl;
        return false;
    }

    // Load HDR equirectangular map via RHI
    TextureHandle hdrTexture = loadHDRTexture(hdrPath);
    if (hdrTexture == INVALID_HANDLE) {
        return false;
    }

    // Create environment cubemap via RHI
    TextureDesc envCubemapDesc{};
    envCubemapDesc.type = TextureType::TextureCube;
    envCubemapDesc.width = 512;
    envCubemapDesc.height = 512;
    envCubemapDesc.format = TextureFormat::RGB16F;
    envCubemapDesc.generateMips = true;  // Will generate after rendering
    envCubemapDesc.mipLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(512, 512))));

    m_environmentMap = m_rhi->createTexture(envCubemapDesc);
    if (m_environmentMap == INVALID_HANDLE) {
        std::cerr << "Failed to create environment cubemap texture" << std::endl;
        m_rhi->destroyTexture(hdrTexture);
        return false;
    }

    // Create render targets for each cubemap face
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // Render to each cubemap face
    m_rhi->setViewport(0, 0, 512, 512);

    for (unsigned int faceIndex = 0; faceIndex < 6; ++faceIndex) {
        // Create render target for this cubemap face
        RenderTargetDesc faceRTDesc{};
        faceRTDesc.width = 512;
        faceRTDesc.height = 512;

        RenderTargetAttachment colorAttachment{};
        colorAttachment.type = AttachmentType::Color0;
        colorAttachment.texture = m_environmentMap;
        colorAttachment.mipLevel = 0;
        colorAttachment.arrayLayer = faceIndex;  // Cubemap face index
        faceRTDesc.colorAttachments.push_back(colorAttachment);

        RenderTargetHandle faceRT = m_rhi->createRenderTarget(faceRTDesc);
        if (faceRT == INVALID_HANDLE) {
            std::cerr << "Failed to create render target for cubemap face " << faceIndex << std::endl;
            m_rhi->destroyTexture(m_environmentMap);
            m_rhi->destroyTexture(hdrTexture);
            m_environmentMap = INVALID_HANDLE;
            return false;
        }

        // Bind render target
        m_rhi->bindRenderTarget(faceRT);
        m_rhi->clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);

        // Set uniforms via RHI
        m_rhi->bindPipeline(m_cubePipeline);
        m_rhi->bindTexture(hdrTexture, 0);
        m_rhi->setUniformMat4("projection", captureProjection);
        m_rhi->setUniformMat4("view", captureViews[faceIndex]);

        // Draw cube
        renderCube();

        // Cleanup face render target
        m_rhi->destroyRenderTarget(faceRT);
    }

    // Unbind render target
    m_rhi->bindRenderTarget(INVALID_HANDLE);

    // Generate mipmaps via RHI
    m_rhi->generateMipmaps(m_environmentMap);

    // Cleanup HDR texture
    m_rhi->destroyTexture(hdrTexture);

    return true;
}

void IBLSystem::generateIrradianceMap()
{
    if (!m_rhi) {
        std::cerr << "IBLSystem::generateIrradianceMap - RHI not initialized" << std::endl;
        return;
    }

    // Create 32x32 RGB16F cubemap texture via RHI
    TextureDesc irradianceDesc;
    irradianceDesc.type = TextureType::TextureCube;
    irradianceDesc.format = TextureFormat::RGB16F;
    irradianceDesc.width = 32;
    irradianceDesc.height = 32;
    irradianceDesc.mipLevels = 1;
    m_irradianceMap = m_rhi->createTexture(irradianceDesc);

    // Projection matrix for cubemap rendering
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    // View matrices for 6 cubemap faces
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // Render to each cubemap face
    for (unsigned int face = 0; face < 6; ++face) {
        // Create render target for this face
        RenderTargetDesc rtDesc;
        rtDesc.width = 32;
        rtDesc.height = 32;
        RenderTargetAttachment colorAttachment;
        colorAttachment.texture = m_irradianceMap;
        colorAttachment.mipLevel = 0;
        colorAttachment.arrayLayer = face;  // Face index 0-5
        rtDesc.colorAttachments.push_back(colorAttachment);

        RenderTargetHandle rt = m_rhi->createRenderTarget(rtDesc);

        // Bind render target and setup viewport
        m_rhi->bindRenderTarget(rt);
        m_rhi->setViewport(0, 0, 32, 32);
        m_rhi->clear(glm::vec4(0.0f), 1.0f, 0);

        // Bind environment map texture
        m_rhi->bindTexture(m_environmentMap, 0);

        // Set shader uniforms via RHI
        // Create temporary pipeline for irradiance generation
        PipelineDesc irradPipelineDesc{};
        irradPipelineDesc.shader = m_irradianceShader;
        irradPipelineDesc.debugName = "IBL_IrradiancePipeline";

        VertexAttribute posAttr{};
        posAttr.location = 0;
        posAttr.binding = 0;
        posAttr.format = TextureFormat::RGB32F;
        posAttr.offset = 0;
        irradPipelineDesc.vertexAttributes.push_back(posAttr);

        VertexBinding binding{};
        binding.binding = 0;
        binding.stride = 3 * sizeof(float);
        binding.perInstance = false;
        binding.buffer = m_cubeBuffer;
        irradPipelineDesc.vertexBindings.push_back(binding);

        irradPipelineDesc.topology = PrimitiveTopology::Triangles;
        irradPipelineDesc.depthTestEnable = true;
        irradPipelineDesc.depthWriteEnable = true;

        PipelineHandle irradPipeline = m_rhi->createPipeline(irradPipelineDesc);

        m_rhi->bindPipeline(irradPipeline);
        m_rhi->setUniformInt("environmentMap", 0);
        m_rhi->setUniformMat4("projection", captureProjection);
        m_rhi->setUniformMat4("view", captureViews[face]);

        // Render cube
        DrawDesc drawDesc{};
        drawDesc.pipeline = irradPipeline;
        drawDesc.vertexBuffer = m_cubeBuffer;
        drawDesc.vertexCount = 36;
        m_rhi->draw(drawDesc);

        // Clean up temporary pipeline
        m_rhi->destroyPipeline(irradPipeline);

        // Clean up render target immediately (on-demand pattern)
        m_rhi->destroyRenderTarget(rt);
    }

    // Restore default framebuffer
    m_rhi->bindRenderTarget(INVALID_HANDLE);
}

void IBLSystem::generatePrefilterMap()
{
    if (!m_rhi) {
        std::cerr << "IBLSystem::generatePrefilterMap - RHI not initialized" << std::endl;
        return;
    }

    // Create 128x128 RGB16F cubemap texture with mipmaps via RHI
    TextureDesc prefilterDesc;
    prefilterDesc.type = TextureType::TextureCube;
    prefilterDesc.format = TextureFormat::RGB16F;
    prefilterDesc.width = 128;
    prefilterDesc.height = 128;
    prefilterDesc.mipLevels = 5;
    m_prefilterMap = m_rhi->createTexture(prefilterDesc);

    // Generate mipmaps for the cubemap
    m_rhi->generateMipmaps(m_prefilterMap);

    // Projection matrix for cubemap rendering
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    // View matrices for 6 cubemap faces
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // Render all mip levels (5 mips × 6 faces = 30 render passes)
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        float roughness = (float)mip / (float)(maxMipLevels - 1);

        for (unsigned int face = 0; face < 6; ++face) {
            // Create render target for this mip level and face
            RenderTargetDesc rtDesc;
            rtDesc.width = mipWidth;
            rtDesc.height = mipHeight;
            RenderTargetAttachment colorAttachment;
            colorAttachment.texture = m_prefilterMap;
            colorAttachment.mipLevel = mip;
            colorAttachment.arrayLayer = face;  // Face index 0-5
            rtDesc.colorAttachments.push_back(colorAttachment);

            RenderTargetHandle rt = m_rhi->createRenderTarget(rtDesc);

            // Bind render target and setup viewport
            m_rhi->bindRenderTarget(rt);
            m_rhi->setViewport(0, 0, mipWidth, mipHeight);
            m_rhi->clear(glm::vec4(0.0f), 1.0f, 0);

            // Bind environment map texture
            m_rhi->bindTexture(m_environmentMap, 0);

            // Set shader uniforms via RHI
            // Create temporary pipeline for prefilter generation
            PipelineDesc prefilterPipelineDesc{};
            prefilterPipelineDesc.shader = m_prefilterShader;
            prefilterPipelineDesc.debugName = "IBL_PrefilterPipeline";

            VertexAttribute posAttr{};
            posAttr.location = 0;
            posAttr.binding = 0;
            posAttr.format = TextureFormat::RGB32F;
            posAttr.offset = 0;
            prefilterPipelineDesc.vertexAttributes.push_back(posAttr);

            VertexBinding binding{};
            binding.binding = 0;
            binding.stride = 3 * sizeof(float);
            binding.perInstance = false;
            binding.buffer = m_cubeBuffer;
            prefilterPipelineDesc.vertexBindings.push_back(binding);

            prefilterPipelineDesc.topology = PrimitiveTopology::Triangles;
            prefilterPipelineDesc.depthTestEnable = true;
            prefilterPipelineDesc.depthWriteEnable = true;

            PipelineHandle prefilterPipeline = m_rhi->createPipeline(prefilterPipelineDesc);

            m_rhi->bindPipeline(prefilterPipeline);
            m_rhi->setUniformInt("environmentMap", 0);
            m_rhi->setUniformMat4("projection", captureProjection);
            m_rhi->setUniformMat4("view", captureViews[face]);
            m_rhi->setUniformFloat("roughness", roughness);

            // Render cube
            DrawDesc drawDesc{};
            drawDesc.pipeline = prefilterPipeline;
            drawDesc.vertexBuffer = m_cubeBuffer;
            drawDesc.vertexCount = 36;
            m_rhi->draw(drawDesc);

            // Clean up temporary pipeline
            m_rhi->destroyPipeline(prefilterPipeline);

            // Clean up render target immediately (on-demand pattern)
            m_rhi->destroyRenderTarget(rt);
        }
    }

    // Restore default framebuffer
    m_rhi->bindRenderTarget(INVALID_HANDLE);
}

void IBLSystem::generateBRDFLUT()
{
    if (!m_rhi) {
        std::cerr << "IBLSystem::generateBRDFLUT - RHI not initialized" << std::endl;
        return;
    }

    // Create 512x512 RG16F 2D texture via RHI
    TextureDesc brdfDesc;
    brdfDesc.type = TextureType::Texture2D;
    brdfDesc.format = TextureFormat::RG16F;
    brdfDesc.width = 512;
    brdfDesc.height = 512;
    brdfDesc.mipLevels = 1;
    m_brdfLUT = m_rhi->createTexture(brdfDesc);

    // Create render target for BRDF LUT
    RenderTargetDesc rtDesc;
    rtDesc.width = 512;
    rtDesc.height = 512;
    RenderTargetAttachment colorAttachment;
    colorAttachment.texture = m_brdfLUT;
    colorAttachment.mipLevel = 0;
    rtDesc.colorAttachments.push_back(colorAttachment);

    RenderTargetHandle rt = m_rhi->createRenderTarget(rtDesc);

    // Bind render target and setup viewport
    m_rhi->bindRenderTarget(rt);
    m_rhi->setViewport(0, 0, 512, 512);
    m_rhi->clear(glm::vec4(0.0f), 1.0f, 0);

    // Set shader and render full-screen quad via RHI
    m_rhi->bindPipeline(m_quadPipeline);
    renderQuad();

    // Clean up render target
    m_rhi->destroyRenderTarget(rt);

    // Restore default framebuffer
    m_rhi->bindRenderTarget(INVALID_HANDLE);
}

void IBLSystem::bindIBLTextures() const
{
    // Bind IBL textures to standard slots
    m_rhi->bindTexture(m_irradianceMap, Slots::IrradianceMap);
    m_rhi->bindTexture(m_prefilterMap, Slots::PrefilterMap);
    m_rhi->bindTexture(m_brdfLUT, Slots::BrdfLut);
}

void IBLSystem::setupCube()
{
    using namespace glint3d;

    // Create vertex buffer for cube
    BufferDesc bufferDesc{};
    bufferDesc.type = BufferType::Vertex;
    bufferDesc.usage = BufferUsage::Static;
    bufferDesc.size = sizeof(cubeVertices);
    bufferDesc.initialData = cubeVertices;
    m_cubeBuffer = m_rhi->createBuffer(bufferDesc);

    // Create pipeline for cube rendering (used for environment map conversion)
    PipelineDesc pipelineDesc{};
    pipelineDesc.shader = m_equirectToCubemapShader;
    pipelineDesc.debugName = "IBL_CubePipeline";

    // Vertex attribute: location 0, binding 0, RGB32F (3 floats), offset 0
    VertexAttribute posAttr{};
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = TextureFormat::RGB32F;
    posAttr.offset = 0;
    pipelineDesc.vertexAttributes.push_back(posAttr);

    // Vertex binding: binding 0, stride 3 floats, per-vertex
    VertexBinding binding{};
    binding.binding = 0;
    binding.stride = 3 * sizeof(float);
    binding.perInstance = false;
    binding.buffer = m_cubeBuffer;
    pipelineDesc.vertexBindings.push_back(binding);

    pipelineDesc.topology = PrimitiveTopology::Triangles;
    pipelineDesc.depthTestEnable = true;
    pipelineDesc.depthWriteEnable = true;

    m_cubePipeline = m_rhi->createPipeline(pipelineDesc);
}

void IBLSystem::setupQuad()
{
    using namespace glint3d;

    // Create vertex buffer for quad
    BufferDesc bufferDesc{};
    bufferDesc.type = BufferType::Vertex;
    bufferDesc.usage = BufferUsage::Static;
    bufferDesc.size = sizeof(quadVertices);
    bufferDesc.initialData = quadVertices;
    m_quadBuffer = m_rhi->createBuffer(bufferDesc);

    // Create pipeline for quad rendering (BRDF LUT)
    PipelineDesc pipelineDesc{};
    pipelineDesc.shader = m_brdfShader;

    // Vertex attribute 0: position (RGB32F - 3 floats)
    VertexAttribute posAttr{};
    posAttr.location = 0;
    posAttr.binding = 0;
    posAttr.format = TextureFormat::RGB32F;
    posAttr.offset = 0;
    pipelineDesc.vertexAttributes.push_back(posAttr);

    // Vertex attribute 1: texcoord (RG32F - 2 floats)
    VertexAttribute texAttr{};
    texAttr.location = 1;
    texAttr.binding = 0;
    texAttr.format = TextureFormat::RG32F;
    texAttr.offset = 3 * sizeof(float);
    pipelineDesc.vertexAttributes.push_back(texAttr);

    // Vertex binding: binding 0, stride 5 floats, per-vertex
    VertexBinding binding{};
    binding.binding = 0;
    binding.stride = 5 * sizeof(float);
    binding.perInstance = false;
    pipelineDesc.vertexBindings.push_back(binding);

    pipelineDesc.topology = PrimitiveTopology::TriangleStrip;
    pipelineDesc.depthTestEnable = false;
    pipelineDesc.depthWriteEnable = false;

    m_quadPipeline = m_rhi->createPipeline(pipelineDesc);
}

void IBLSystem::renderCube()
{
    using namespace glint3d;

    DrawDesc drawDesc{};
    drawDesc.pipeline = m_cubePipeline;
    drawDesc.vertexBuffer = m_cubeBuffer;
    drawDesc.vertexCount = 36;

    m_rhi->draw(drawDesc);
}

void IBLSystem::renderQuad()
{
    using namespace glint3d;

    DrawDesc drawDesc{};
    drawDesc.pipeline = m_quadPipeline;
    drawDesc.vertexBuffer = m_quadBuffer;
    drawDesc.vertexCount = 4;

    m_rhi->draw(drawDesc);
}

void IBLSystem::cleanup()
{
    using namespace glint3d;

    if (!m_rhi) return;

    // Destroy textures
    if (m_environmentMap != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_environmentMap);
        m_environmentMap = INVALID_HANDLE;
    }
    if (m_irradianceMap != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_irradianceMap);
        m_irradianceMap = INVALID_HANDLE;
    }
    if (m_prefilterMap != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_prefilterMap);
        m_prefilterMap = INVALID_HANDLE;
    }
    if (m_brdfLUT != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_brdfLUT);
        m_brdfLUT = INVALID_HANDLE;
    }

    // Note: Legacy capture framebuffer removed - no longer needed with on-demand render targets

    // Destroy buffers
    if (m_cubeBuffer != INVALID_HANDLE) {
        m_rhi->destroyBuffer(m_cubeBuffer);
        m_cubeBuffer = INVALID_HANDLE;
    }
    if (m_quadBuffer != INVALID_HANDLE) {
        m_rhi->destroyBuffer(m_quadBuffer);
        m_quadBuffer = INVALID_HANDLE;
    }

    // Destroy pipelines
    if (m_cubePipeline != INVALID_HANDLE) {
        m_rhi->destroyPipeline(m_cubePipeline);
        m_cubePipeline = INVALID_HANDLE;
    }
    if (m_quadPipeline != INVALID_HANDLE) {
        m_rhi->destroyPipeline(m_quadPipeline);
        m_quadPipeline = INVALID_HANDLE;
    }

    // Destroy shaders
    if (m_equirectToCubemapShader != INVALID_HANDLE) {
        m_rhi->destroyShader(m_equirectToCubemapShader);
        m_equirectToCubemapShader = INVALID_HANDLE;
    }
    if (m_irradianceShader != INVALID_HANDLE) {
        m_rhi->destroyShader(m_irradianceShader);
        m_irradianceShader = INVALID_HANDLE;
    }
    if (m_prefilterShader != INVALID_HANDLE) {
        m_rhi->destroyShader(m_prefilterShader);
        m_prefilterShader = INVALID_HANDLE;
    }
    if (m_brdfShader != INVALID_HANDLE) {
        m_rhi->destroyShader(m_brdfShader);
        m_brdfShader = INVALID_HANDLE;
    }

    m_initialized = false;
}
