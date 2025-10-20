// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/pipeline_manager.h","purpose":"Creates and caches GPU pipeline objects for render passes","exports":["PipelineManager"],"depends_on":["glint3d::RHI","ShaderHandle"],"notes":["Caches RHI pipelines keyed by geometry layout","Owns reusable shader handles for default materials"]}
#pragma once

/**
 * @file pipeline_manager.h
 * @brief Factory/cache for render pipelines built through the RHI.
 */

#include <glint3d/rhi.h>
#include <unordered_map>
#include <string>

using glint3d::PipelineHandle;
using glint3d::RHI;
using glint3d::ShaderHandle;

// forward declarations
struct SceneObject;

/**
 * PipelineManager centralizes shader and pipeline creation/management for the RHI.
 * It ensures objects acquire appropriate pipelines and manages shared shader handles.
 */
class PipelineManager {
public:
    PipelineManager();
    ~PipelineManager();

    // Initialize/shutdown
    bool init(RHI* rhi);
    void shutdown();

    // Core pipeline operations
    void ensureObjectPipeline(SceneObject& obj, bool usePbr = true);
    PipelineHandle getOrCreatePipeline(const std::string& pipelineName, bool usePbr = true);

    // Shader management
    ShaderHandle getBasicShader() const { return m_basicShaderRhi; }
    ShaderHandle getPbrShader() const { return m_pbrShaderRhi; }

    // Pipeline querying
    bool hasValidPipeline(const SceneObject& obj) const;
    PipelineHandle getObjectPipeline(const SceneObject& obj, bool usePbr = true) const;

    // Configuration
    void setDefaultShaderPaths(const std::string& basicVert, const std::string& basicFrag,
                              const std::string& pbrVert, const std::string& pbrFrag);

private:
    RHI* m_rhi = nullptr;

    // RHI shaders and pipelines
    ShaderHandle m_basicShaderRhi = INVALID_HANDLE;
    ShaderHandle m_pbrShaderRhi = INVALID_HANDLE;
    PipelineHandle m_basicPipeline = INVALID_HANDLE;
    PipelineHandle m_pbrPipeline = INVALID_HANDLE;

    // Pipeline cache
    std::unordered_map<std::string, PipelineHandle> m_pipelineCache;

    // Shader paths
    std::string m_basicVertPath = "engine/shaders/basic.vert";
    std::string m_basicFragPath = "engine/shaders/basic.frag";
    std::string m_pbrVertPath = "engine/shaders/pbr.vert";
    std::string m_pbrFragPath = "engine/shaders/pbr.frag";

    // Helper methods
    bool createRhiShaders();
    PipelineHandle createBasicPipeline(const SceneObject& obj);
    PipelineHandle createPbrPipeline(const SceneObject& obj);
    std::string generatePipelineKey(const SceneObject& obj, bool usePbr) const;
    static std::string loadTextFile(const std::string& path);
};
