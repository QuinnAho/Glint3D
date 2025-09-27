#include "managers/transform_manager.h"
#include <cstring>
#include <iostream>

TransformManager::TransformManager()
{
    // Initialize transform matrices to identity
    m_transformData.model = glm::mat4(1.0f);
    m_transformData.view = glm::mat4(1.0f);
    m_transformData.projection = glm::mat4(1.0f);
    m_transformData.lightSpaceMatrix = glm::mat4(1.0f);
}

TransformManager::~TransformManager()
{
    shutdown();
}

bool TransformManager::init(RHI* rhi)
{
    if (!rhi) {
        std::cerr << "TransformManager::init: RHI is null" << std::endl;
        return false;
    }

    m_rhi = rhi;
    allocateUBO();
    return true;
}

void TransformManager::shutdown()
{
    if (m_rhi && m_transformBlock.handle != INVALID_HANDLE) {
        m_rhi->freeUniforms(m_transformBlock);
        m_transformBlock = {};
    }
    m_rhi = nullptr;
}

void TransformManager::updateTransforms(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_rhi) return;

    m_transformData.model = model;
    m_transformData.view = view;
    m_transformData.projection = projection;

    updateUBO();
}

void TransformManager::updateModel(const glm::mat4& model)
{
    if (!m_rhi) return;

    m_transformData.model = model;
    updateUBO();
}

void TransformManager::updateView(const glm::mat4& view)
{
    if (!m_rhi) return;

    m_transformData.view = view;
    updateUBO();
}

void TransformManager::updateProjection(const glm::mat4& projection)
{
    if (!m_rhi) return;

    m_transformData.projection = projection;
    updateUBO();
}

void TransformManager::updateLightSpaceMatrix(const glm::mat4& lightSpaceMatrix)
{
    if (!m_rhi) return;

    m_transformData.lightSpaceMatrix = lightSpaceMatrix;
    updateUBO();
}

void TransformManager::bindTransformUniforms()
{
    if (!m_rhi || m_transformBlock.handle == INVALID_HANDLE) {
        return;
    }

    // Bind the transform UBO to binding point 0 (as defined in uniform_blocks.h)
    m_rhi->bindUniformBuffer(m_transformBlock.handle, 0);
}

void TransformManager::allocateUBO()
{
    if (!m_rhi || m_transformBlock.handle != INVALID_HANDLE) {
        return;
    }

    UniformAllocationDesc desc{};
    desc.size = sizeof(TransformBlock);
    desc.alignment = 256; // UBO alignment requirement
    desc.debugName = "TransformBlock";

    m_transformBlock = m_rhi->allocateUniforms(desc);

    if (m_transformBlock.handle == INVALID_HANDLE) {
        std::cerr << "TransformManager: Failed to allocate transform UBO" << std::endl;
    }
}

void TransformManager::updateUBO()
{
    if (!m_rhi || m_transformBlock.handle == INVALID_HANDLE || !m_transformBlock.mappedPtr) {
        return;
    }

    // Copy transform data to the mapped UBO memory
    memcpy(m_transformBlock.mappedPtr, &m_transformData, sizeof(TransformBlock));
}