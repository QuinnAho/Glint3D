#pragma once
#include "rhi/rhi.h"
#include <cstdint>

class RhiNull : public RHI {
public:
    RhiNull() = default;
    ~RhiNull() override = default;

    bool init(const RhiInit& desc) override { (void)desc; return true; }
    void shutdown() override {}

    void beginFrame() override { m_drawCalls = 0; }
    void endFrame() override {}

    void draw(const DrawDesc& desc) override { (void)desc; ++m_drawCalls; }
    void readback(const ReadbackDesc& desc) override { (void)desc; }

    TextureHandle createTexture(const TextureDesc& desc) override { (void)desc; return ++m_nextHandle; }
    BufferHandle createBuffer(const BufferDesc& desc) override { (void)desc; return ++m_nextHandle; }
    ShaderHandle createShader(const ShaderDesc& desc) override { (void)desc; return ++m_nextHandle; }
    PipelineHandle createPipeline(const PipelineDesc& desc) override { (void)desc; return ++m_nextHandle; }

    void destroyTexture(TextureHandle) override {}
    void destroyBuffer(BufferHandle) override {}
    void destroyShader(ShaderHandle) override {}
    void destroyPipeline(PipelineHandle) override {}

    void setViewport(int, int, int, int) override {}
    void clear(const glm::vec4&, float, int) override {}
    void bindPipeline(PipelineHandle) override {}
    void bindTexture(TextureHandle, uint32_t) override {}

    bool supportsCompute() const override { return false; }
    bool supportsGeometryShaders() const override { return false; }
    bool supportsTessellation() const override { return false; }
    int getMaxTextureUnits() const override { return 0; }
    int getMaxSamples() const override { return 0; }

    Backend getBackend() const override { return Backend::Null; }
    const char* getBackendName() const override { return "NullRHI"; }

    uint32_t getDrawCallCount() const { return m_drawCalls; }

private:
    uint32_t m_nextHandle = 1;
    uint32_t m_drawCalls = 0;
};
