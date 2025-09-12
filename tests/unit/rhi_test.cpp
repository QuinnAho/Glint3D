#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>

#include "../../engine/include/glint3d/rhi.h"
#include "../../engine/include/glint3d/rhi_types.h"

using namespace glint3d;

static bool isValidHandle(uint32_t h) { return h != INVALID_HANDLE; }

int main() {
    // Basic type defaults
    RhiInit init{};
    assert(init.windowWidth == 800);
    assert(init.windowHeight == 600);
    assert(init.enableSRGB == true);
    assert(init.samples == 1);

    // Descriptor defaults
    TextureDesc tex{};
    assert(tex.type == TextureType::Texture2D);
    assert(tex.format == TextureFormat::RGBA8);
    assert(tex.width == 0 && tex.height == 0);
    assert(tex.mipLevels == 1);
    assert(tex.arrayLayers == 1);
    assert(tex.initialData == nullptr);
    assert(tex.initialDataSize == 0);

    BufferDesc buf{};
    assert(buf.type == BufferType::Vertex);
    assert(buf.usage == BufferUsage::Static);
    assert(buf.size == 0);
    assert(buf.initialData == nullptr);

    ShaderDesc sh{};
    assert(sh.stages == 0);
    assert(sh.vertexSource.empty());
    assert(sh.fragmentSource.empty());

    PipelineDesc pipe{};
    assert(pipe.shader == INVALID_HANDLE);
    assert(pipe.depthTestEnable == true);
    assert(pipe.depthWriteEnable == true);
    assert(pipe.blendEnable == false);

    DrawDesc draw{};
    assert(draw.pipeline == INVALID_HANDLE);
    assert(draw.vertexCount == 0);
    assert(draw.indexCount == 0);
    assert(draw.instanceCount == 1);

    // ShaderStage bitwise ops
    auto stages = (ShaderStage::Vertex | ShaderStage::Fragment);
    assert((static_cast<uint32_t>(stages) & static_cast<uint32_t>(ShaderStage::Vertex)) != 0u);

    // Handles are opaque integers; verify INVALID_HANDLE semantics
    TextureHandle th = INVALID_HANDLE;
    assert(!isValidHandle(th));

    // New API: presence and signature checks for uniform buffer binding
    using BindUBO = void (RHI::*)(BufferHandle, uint32_t);
    static_assert(std::is_same<BindUBO, decltype(&RHI::bindUniformBuffer)>::value, "bindUniformBuffer signature changed");

    using UpdateBuf = void (RHI::*)(BufferHandle, const void*, size_t, size_t);
    static_assert(std::is_same<UpdateBuf, decltype(&RHI::updateBuffer)>::value, "updateBuffer signature changed");

    // Ensure BufferDesc can describe a uniform buffer
    BufferDesc uboDesc{}; uboDesc.type = BufferType::Uniform; uboDesc.size = 64;
    assert(uboDesc.type == BufferType::Uniform && uboDesc.size == 64);

    // Note: Do not instantiate an RHI backend here; creation is backend-specific.
    // This test only validates header API/ABI and defaults.

    return 0;
}
