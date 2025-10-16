// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/pipeline_manager.h","purpose":"Creates and caches GPU pipeline objects for render passes","exports":["PipelineManager"],"depends_on":["glint3d::RHI","ShaderHandle"],"notes":["Provides reuse of shader/pipeline combos","Handles rebuilds on shader or config changes"]}
#pragma once

/**
 * @file pipeline_manager.h
 * @brief Factory/cache for render pipelines built through the RHI.
 */

#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <memory>
#include <unordered_map>
#include <string>

using glint3d::PipelineHandle;
using glint3d::RHI;
using glint3d::ShaderHandle;

// forward declarations
struct SceneObject;
class Shader;

/**
 * PipelineManager centralizes shader and pipeline creation/management.
 * This handles the complex logic around ensuring objects have appropriate pipelines
 * and manages shader resources.
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
    bool loadShaders();
    ShaderHandle getBasicShader() const { return m_basicShaderRhi; }
    ShaderHandle getPbrShader() const { return m_pbrShaderRhi; }
    Shader* getBasicShaderLegacy() const { return m_basicShader ? m_basicShader.get() : m_pbrShader.get(); }
    Shader* getPbrShaderLegacy() const { return m_pbrShader.get(); }

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

    // Legacy shader objects (for compatibility)
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_pbrShader;

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
