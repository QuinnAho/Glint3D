#pragma once
#include <glint3d/rhi.h>
#include <cstdint>

using namespace glint3d;

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
    RenderTargetHandle createRenderTarget(const RenderTargetDesc& desc) override { (void)desc; return ++m_nextHandle; }
    BindGroupLayoutHandle createBindGroupLayout(const BindGroupLayoutDesc& desc) override { (void)desc; return ++m_nextHandle; }
    BindGroupHandle createBindGroup(const BindGroupDesc& desc) override { (void)desc; return ++m_nextHandle; }

    void destroyTexture(TextureHandle) override {}
    void destroyBuffer(BufferHandle) override {}
    void destroyShader(ShaderHandle) override {}
    void destroyPipeline(PipelineHandle) override {}
    void destroyRenderTarget(RenderTargetHandle) override {}
    void destroyBindGroupLayout(BindGroupLayoutHandle) override {}
    void destroyBindGroup(BindGroupHandle) override {}

    void setViewport(int, int, int, int) override {}
    void clear(const glm::vec4&, float, int) override {}
    void bindPipeline(PipelineHandle) override {}
    void bindTexture(TextureHandle, uint32_t) override {}
    void bindUniformBuffer(BufferHandle, uint32_t) override {}
    void updateBuffer(BufferHandle, const void*, size_t, size_t = 0) override {}
    void updateTexture(TextureHandle, const void*, int, int, TextureFormat, int = 0, int = 0, int = 0) override {}
    void bindRenderTarget(RenderTargetHandle) override {}
    void resolveRenderTarget(RenderTargetHandle, TextureHandle, const int* = nullptr, const int* = nullptr) override {}
    void resolveToDefaultFramebuffer(RenderTargetHandle, const int* = nullptr, const int* = nullptr) override {}

    // Legacy uniform helpers (no-ops for null backend)
    void setUniformMat4(const char*, const glm::mat4&) override {}
    void setUniformVec3(const char*, const glm::vec3&) override {}
    void setUniformVec4(const char*, const glm::vec4&) override {}
    void setUniformFloat(const char*, float) override {}
    void setUniformInt(const char*, int) override {}
    void setUniformBool(const char*, bool) override {}

    // Uniform Buffer Ring Allocator (FEAT-0249) - No-op implementations
    UniformAllocation allocateUniforms(const UniformAllocationDesc&) override {
        return {};
    }
    void freeUniforms(const UniformAllocation&) override {}
    ShaderReflection getShaderReflection(ShaderHandle) override {
        ShaderReflection reflection{};
        reflection.isValid = false;
        return reflection;
    }
    bool setUniformInBlock(const UniformAllocation&, ShaderHandle,
                          const char*, const char*, const void*, size_t) override {
        return false;
    }
    int setUniformsInBlock(const UniformAllocation&, ShaderHandle,
                          const char*, const UniformNameValue*, int) override {
        return 0;
    }
    bool bindUniformBlock(const UniformAllocation&, ShaderHandle, const char*) override {
        return true; // no-op success for testing
    }

    std::unique_ptr<CommandEncoder> createCommandEncoder(const char* = nullptr) override;
    Queue& getQueue() override { return m_queue; }

    bool supportsCompute() const override { return false; }
    bool supportsGeometryShaders() const override { return false; }
    bool supportsTessellation() const override { return false; }
    int getMaxTextureUnits() const override { return 0; }
    int getMaxSamples() const override { return 0; }

    Backend getBackend() const override { return Backend::Null; }
    const char* getBackendName() const override { return "NullRHI"; }
    std::string getDebugInfo() const override { return "NullRHI for testing"; }

    uint32_t getDrawCallCount() const { return m_drawCalls; }

private:
    class NullPass : public RenderPassEncoder {
    public:
        void setPipeline(PipelineHandle) override {}
        void setBindGroup(uint32_t, BindGroupHandle) override {}
        void setViewport(int, int, int, int) override {}
        void draw(const DrawDesc&) override {}
        void end() override {}
    };

    class NullEncoder : public CommandEncoder {
    public:
        RenderPassEncoder* beginRenderPass(const RenderPassDesc&) override { return &m_pass; }
        void finish() override {}
    private:
        NullPass m_pass;
    };

    class NullQueue : public Queue {
    public:
        void submit(CommandEncoder& encoder) override { encoder.finish(); }
    };
    uint32_t m_nextHandle = 1;
    uint32_t m_drawCalls = 0;
    NullQueue m_queue;
};
