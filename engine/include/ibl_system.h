#pragma once

#include "glint3d/rhi.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class IBLSystem {
public:
    IBLSystem();
    ~IBLSystem();

    bool init(glint3d::RHI* rhi);
    bool loadHDREnvironment(const std::string& hdrPath);
    
    void generateIrradianceMap();
    void generatePrefilterMap();
    void generateBRDFLUT();
    
    void bindIBLTextures() const;
    
    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }
    
    glint3d::TextureHandle getEnvironmentMap() const { return m_environmentMap; }
    glint3d::TextureHandle getIrradianceMap() const { return m_irradianceMap; }
    glint3d::TextureHandle getPrefilterMap() const { return m_prefilterMap; }
    glint3d::TextureHandle getBRDFLUT() const { return m_brdfLUT; }
    
    void cleanup();

private:
    // RHI pointer
    glint3d::RHI* m_rhi;

    // Textures
    glint3d::TextureHandle m_environmentMap;
    glint3d::TextureHandle m_irradianceMap;
    glint3d::TextureHandle m_prefilterMap;
    glint3d::TextureHandle m_brdfLUT;

    // Render target for convolution
    glint3d::RenderTargetHandle m_captureFramebuffer;

    // Shaders (RHI handles)
    glint3d::ShaderHandle m_equirectToCubemapShader;
    glint3d::ShaderHandle m_irradianceShader;
    glint3d::ShaderHandle m_prefilterShader;
    glint3d::ShaderHandle m_brdfShader;

    // Geometry buffers
    glint3d::BufferHandle m_cubeBuffer;
    glint3d::BufferHandle m_quadBuffer;

    // Render pipelines
    glint3d::PipelineHandle m_cubePipeline;
    glint3d::PipelineHandle m_quadPipeline;
    
    float m_intensity;
    bool m_initialized;
    
    void setupCube();
    void setupQuad();
    void createShaders();
    glint3d::TextureHandle loadHDRTexture(const std::string& path);
    void renderCube();
    void renderQuad();
};