#include "rhi/rhi_null.h"

std::unique_ptr<CommandEncoder> RhiNull::createCommandEncoder(const char*) {
    return std::make_unique<NullEncoder>();
}
