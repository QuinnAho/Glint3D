#include "texture.h"
#include <glint3d/rhi.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <fstream>
#include <filesystem>
#ifdef KTX2_ENABLED
#  include <ktx.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::string toLowerExt(const std::string& path)
{
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return ext;
}

glint3d::RHI* Texture::s_rhi = nullptr;

Texture::Texture() {}

Texture::~Texture()
{
    using namespace glint3d;
    if (m_rhiTex != INVALID_HANDLE && s_rhi) {
        s_rhi->destroyTexture(m_rhiTex);
        m_rhiTex = INVALID_HANDLE;
    }
}

bool Texture::loadFromFile(const std::string& filepath, bool flipY)
{
    using namespace glint3d;

    if (!s_rhi) {
        std::cerr << "[Texture] RHI not initialized. Cannot load texture." << std::endl;
        return false;
    }

    // Prefer a sibling .ktx2 when present, or direct .ktx2 request
    std::error_code ec;
    std::filesystem::path inPath(filepath);
    const std::string ext = toLowerExt(filepath);
    const bool requestIsKTX2 = (ext == ".ktx2");
    std::filesystem::path ktx2Path = requestIsKTX2 ? inPath : inPath;
    if (!requestIsKTX2)
    {
        ktx2Path.replace_extension(".ktx2");
    }
    const bool ktx2Exists = std::filesystem::exists(ktx2Path, ec) && !ec;
    if (requestIsKTX2 || ktx2Exists)
    {
        if (loadFromKTX2(ktx2Path.string()))
        {
            return true;
        }
        // If KTX2 load failed (e.g., not enabled), fall through to STB
        std::cerr << "[Texture] KTX2 load failed or unsupported for '" << ktx2Path.string() << "'. Falling back to STB." << std::endl;
    }

    // Load image data
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        std::cerr << "[Texture] Failed to load texture: " << filepath << std::endl;
        return false;
    }

    // Store dims for perf HUD
    m_width = width; m_height = height; m_channels = channels;

    // Create texture via RHI
    TextureDesc desc{};
    desc.type = TextureType::Texture2D;
    desc.width = width;
    desc.height = height;

    switch (channels) {
        case 4: desc.format = TextureFormat::RGBA8; break;
        case 3: desc.format = TextureFormat::RGB8; break;
        case 2: desc.format = TextureFormat::RG8; break;
        default: desc.format = TextureFormat::R8; break;
    }

    desc.generateMips = true;
    desc.initialData = data;
    desc.initialDataSize = width * height * channels;
    desc.debugName = filepath;

    m_rhiTex = s_rhi->createTexture(desc);
    stbi_image_free(data);

    if (m_rhiTex == INVALID_HANDLE) {
        std::cerr << "[Texture] RHI texture creation failed for '" << filepath << "'." << std::endl;
        return false;
    }

    return true;
}

bool Texture::loadFromKTX2(const std::string& filepath)
{
#ifdef KTX2_ENABLED
    using namespace glint3d;

    if (!s_rhi) {
        std::cerr << "[Texture] RHI not initialized. Cannot load KTX2 texture." << std::endl;
        return false;
    }

    ktxTexture2* k2 = nullptr;
    KTX_error_code rc = ktxTexture2_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &k2);
    if (rc != KTX_SUCCESS || !k2)
    {
        std::cerr << "[Texture] ktxTexture2_CreateFromNamedFile failed for '" << filepath << "' rc=" << rc << std::endl;
        return false;
    }

    ktxTexture* kt = reinterpret_cast<ktxTexture*>(k2);

    // Dimensions
    m_width  = static_cast<int>(ktxTexture_GetWidth(kt));
    m_height = static_cast<int>(ktxTexture_GetHeight(kt));
    m_channels = 4; // Approximate for compressed formats

    // Create RHI texture from KTX2 data
    TextureDesc desc{};
    desc.type = TextureType::Texture2D;
    desc.width = m_width;
    desc.height = m_height;
    desc.format = TextureFormat::RGBA8; // TODO: Map KTX2 format to RHI format
    desc.generateMips = (kt->numLevels > 1);
    desc.initialData = ktxTexture_GetData(kt);
    desc.initialDataSize = ktxTexture_GetDataSize(kt);
    desc.debugName = filepath;

    m_rhiTex = s_rhi->createTexture(desc);
    ktxTexture_Destroy(kt);

    if (m_rhiTex == INVALID_HANDLE) {
        std::cerr << "[Texture] RHI texture creation failed for KTX2 '" << filepath << "'." << std::endl;
        return false;
    }

    return true;
#else
    (void)filepath;
    std::cerr << "[Texture] KTX2 support not enabled." << std::endl;
    return false;
#endif
}

void Texture::bind(GLuint unit) const
{
    // Legacy method: bind via RHI
    if (s_rhi && m_rhiTex != glint3d::INVALID_HANDLE) {
        s_rhi->bindTexture(m_rhiTex, unit);
    }
}

