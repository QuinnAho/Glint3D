#include "texture_cache.h"
#include <glint3d/rhi.h>
#include <glint3d/rhi_types.h>
#include "image_io.h"
#include <functional>
#include <filesystem>
#include <system_error>

TextureCache& TextureCache::instance() { static TextureCache inst; return inst; }

size_t TextureCache::KeyHash::operator()(Key const& k) const {
    return std::hash<std::string>()(k.path) ^ (k.flip ? 0x9e3779b97f4a7c15ULL : 0ULL);
}
bool TextureCache::KeyEq::operator()(Key const& a, Key const& b) const {
    return a.flip == b.flip && a.path == b.path;
}

Texture* TextureCache::get(const std::string& path, bool flipY)
{
    // Resolve to .ktx2 if present so cache keys reflect the actual loaded asset
    std::string resolved = path;
    try {
        std::error_code ec;
        std::filesystem::path p(path);
        std::filesystem::path ktx2 = p; ktx2.replace_extension(".ktx2");
        if (std::filesystem::exists(ktx2, ec) && !ec) {
            resolved = ktx2.string();
        }
    } catch (...) {
        // ignore and use original path
    }

    Key key{resolved, flipY};
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second.get();
    auto tex = std::make_unique<Texture>();
    if (!tex->loadFromFile(resolved, flipY)) return nullptr;

    // If an RHI is registered, create a matching RHI texture from CPU pixels
    if (Texture::getRHI() && tex->rhiHandle() == glint3d::INVALID_HANDLE) {
        ImageIO::ImageData8 img;
        if (ImageIO::LoadImage8(resolved, img, flipY)) {
            glint3d::TextureDesc td{};
            td.type = glint3d::TextureType::Texture2D;
            // Choose format by channel count (1..4). Use RGBA8 as default for 4, RGB8 for 3, R8/RG8 otherwise.
            switch (img.channels) {
                case 4: td.format = glint3d::TextureFormat::RGBA8; break;
                case 3: td.format = glint3d::TextureFormat::RGB8; break;
                case 2: td.format = glint3d::TextureFormat::RG8; break;
                default: td.format = glint3d::TextureFormat::R8; break;
            }
            td.width = img.width; td.height = img.height; td.mipLevels = 1;
            td.initialData = img.pixels.data();
            td.initialDataSize = img.pixels.size();
            td.debugName = resolved;
            auto* rhi = Texture::getRHI();
            if (rhi) {
                auto handle = rhi->createTexture(td);
                if (handle != glint3d::INVALID_HANDLE) {
                    tex->setRHIHandle(handle);
                }
            }
        }
    }
    Texture* out = tex.get();
    m_cache.emplace(std::move(key), std::move(tex));
    return out;
}

void TextureCache::clear()
{
    m_cache.clear();
}
