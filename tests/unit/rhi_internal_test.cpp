#include <type_traits>
#include <cstdint>
#include <cassert>

#include "../../engine/include/rhi/rhi.h"
#include "../../engine/include/rhi/rhi_types.h"

int main() {
    // Uniform buffer API signature checks
    using BindUBO = void (RHI::*)(BufferHandle, uint32_t);
    static_assert(std::is_same<BindUBO, decltype(&RHI::bindUniformBuffer)>::value, "bindUniformBuffer signature changed");

    using UpdateBuf = void (RHI::*)(BufferHandle, const void*, size_t, size_t);
    static_assert(std::is_same<UpdateBuf, decltype(&RHI::updateBuffer)>::value, "updateBuffer signature changed");

    // Render target descriptor presence
    RenderTargetDesc rtd{};
    assert(rtd.width == 0 && rtd.height == 0 && rtd.samples == 1);
    (void)rtd;
    return 0;
}

