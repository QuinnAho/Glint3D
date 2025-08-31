#include "texture_cache.h"
#include <functional>

TextureCache& TextureCache::instance() { static TextureCache inst; return inst; }

size_t TextureCache::KeyHash::operator()(Key const& k) const {
    return std::hash<std::string>()(k.path) ^ (k.flip ? 0x9e3779b97f4a7c15ULL : 0ULL);
}
bool TextureCache::KeyEq::operator()(Key const& a, Key const& b) const {
    return a.flip == b.flip && a.path == b.path;
}

Texture* TextureCache::get(const std::string& path, bool flipY)
{
    Key key{path, flipY};
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second.get();
    auto tex = std::make_unique<Texture>();
    if (!tex->loadFromFile(path, flipY)) return nullptr;
    Texture* out = tex.get();
    m_cache.emplace(std::move(key), std::move(tex));
    return out;
}

void TextureCache::clear()
{
    m_cache.clear();
}

