// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/transform_manager.h","purpose":"Maintains transform UBO data for objects and global matrices","exports":["TransformManager"],"depends_on":["glm","SceneManager","glint3d::uniform blocks"],"notes":["Uploads model/view/projection data","Supports object picking and gizmo helpers"]}
#pragma once

/**
 * @file transform_manager.h
 * @brief Assembles model/view/projection transforms and writes transform UBOs.
 */

#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using glint3d::RHI;
using glint3d::TransformBlock;
using glint3d::UniformAllocation;

class TransformManager {
public:
    TransformManager();
    ~TransformManager();

    // Initialization and cleanup
    bool init(RHI* rhi);
    void shutdown();

    // Transform data management
    void updateTransforms(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    void updateModel(const glm::mat4& model);
    void updateView(const glm::mat4& view);
    void updateProjection(const glm::mat4& projection);
    void updateLightSpaceMatrix(const glm::mat4& lightSpaceMatrix);

    // UBO binding
    void bindTransformUniforms();

    // Getters for current transform state
    const glm::mat4& getModel() const { return m_transformData.model; }
    const glm::mat4& getView() const { return m_transformData.view; }
    const glm::mat4& getProjection() const { return m_transformData.projection; }

private:
    RHI* m_rhi = nullptr;

    // UBO management
    UniformAllocation m_transformBlock = {};
    TransformBlock m_transformData = {};

    void allocateUBO();
    void updateUBO();
};
