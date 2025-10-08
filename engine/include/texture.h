#ifndef TEXTURE_H
#define TEXTURE_H

#include "gl_platform.h"
#include <glint3d/rhi_types.h>

namespace glint3d { class RHI; }
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& filepath, bool flipY = false);

    // Legacy bind method: now forwards to RHI
    void bind(GLuint unit = 0) const;

    // RHI integration
    glint3d::TextureHandle rhiHandle() const { return m_rhiTex; }
    void setRHIHandle(glint3d::TextureHandle h) { m_rhiTex = h; }
    static void setRHI(glint3d::RHI* rhi) { s_rhi = rhi; }
    static glint3d::RHI* getRHI() { return s_rhi; }

    // Perf introspection
    int  width() const { return m_width; }
    int  height() const { return m_height; }
    int  channels() const { return m_channels; }

private:
    // Optional: KTX2/Basis loader when KTX2_ENABLED is defined
    bool loadFromKTX2(const std::string& filepath);

    int m_width{0}, m_height{0}, m_channels{0};

    // RHI texture handle
    glint3d::TextureHandle m_rhiTex{glint3d::INVALID_HANDLE};

    // Global RHI pointer; set by RenderSystem on init
    static glint3d::RHI* s_rhi;
};

#endif // TEXTURE_H
