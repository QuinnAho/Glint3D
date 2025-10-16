#include "managers/pipeline_manager.h"
#include "managers/scene_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>

PipelineManager::PipelineManager()
{
}

PipelineManager::~PipelineManager()
{
    shutdown();
}

bool PipelineManager::init(RHI* rhi)
{
    if (!rhi) {
        std::cerr << "PipelineManager::init: RHI is null" << std::endl;
        return false;
    }

    m_rhi = rhi;

    // Load legacy shader objects first
    if (!loadShaders()) {
        std::cerr << "PipelineManager: Failed to load legacy shaders" << std::endl;
        return false;
    }

    // Create RHI shader objects
    if (!createRhiShaders()) {
        std::cerr << "PipelineManager: Failed to create RHI shaders" << std::endl;
        return false;
    }

    return true;
}

void PipelineManager::shutdown()
{
    // Clear pipeline cache
    for (auto& [key, pipeline] : m_pipelineCache) {
        if (m_rhi && pipeline != INVALID_HANDLE) {
            m_rhi->destroyPipeline(pipeline);
        }
    }
    m_pipelineCache.clear();

    // Destroy RHI resources
    if (m_rhi) {
        if (m_basicShaderRhi != INVALID_HANDLE) {
            m_rhi->destroyShader(m_basicShaderRhi);
            m_basicShaderRhi = INVALID_HANDLE;
        }
        if (m_pbrShaderRhi != INVALID_HANDLE) {
            m_rhi->destroyShader(m_pbrShaderRhi);
            m_pbrShaderRhi = INVALID_HANDLE;
        }
        if (m_basicPipeline != INVALID_HANDLE) {
            m_rhi->destroyPipeline(m_basicPipeline);
            m_basicPipeline = INVALID_HANDLE;
        }
        if (m_pbrPipeline != INVALID_HANDLE) {
            m_rhi->destroyPipeline(m_pbrPipeline);
            m_pbrPipeline = INVALID_HANDLE;
        }
    }

    // Reset legacy shaders
    m_basicShader.reset();
    m_pbrShader.reset();

    m_rhi = nullptr;
}

bool PipelineManager::loadShaders()
{
    // Load PBR shader (required)
    m_pbrShader = std::make_unique<Shader>();
    if (!m_pbrShader->load(m_pbrVertPath, m_pbrFragPath)) {
        std::cerr << "PipelineManager: Failed to load PBR shader from "
                  << m_pbrVertPath << ", " << m_pbrFragPath << std::endl;
        return false;
    }

    // Try to load basic shader (optional - fallback to PBR if missing)
    m_basicShader = std::make_unique<Shader>();
    if (!m_basicShader->load(m_basicVertPath, m_basicFragPath)) {
        std::cerr << "PipelineManager: Basic shaders not found, using PBR shaders for all rendering" << std::endl;
        // Use PBR shader as fallback for basic shader
        m_basicShader.reset();
    }

    std::cerr << "PipelineManager: Successfully loaded legacy shaders" << std::endl;
    return true;
}

bool PipelineManager::createRhiShaders()
{
    if (!m_rhi) return false;

    // Load PBR shader source (required)
    std::string pbrVertSrc = loadTextFile(m_pbrVertPath);
    std::string pbrFragSrc = loadTextFile(m_pbrFragPath);

    if (pbrVertSrc.empty() || pbrFragSrc.empty()) {
        std::cerr << "PipelineManager: Failed to load PBR shader source files" << std::endl;
        return false;
    }

    // Try to load basic shader source (optional)
    std::string basicVertSrc = loadTextFile(m_basicVertPath);
    std::string basicFragSrc = loadTextFile(m_basicFragPath);
    bool hasBasicShaders = !basicVertSrc.empty() && !basicFragSrc.empty();

    if (hasBasicShaders) {
        // Create basic shader
        ShaderDesc basicDesc{};
        basicDesc.stages = shaderStageBits(ShaderStage::Vertex) | shaderStageBits(ShaderStage::Fragment);
        basicDesc.vertexSource = basicVertSrc;
        basicDesc.fragmentSource = basicFragSrc;
        basicDesc.debugName = "BasicShader";

        m_basicShaderRhi = m_rhi->createShader(basicDesc);
        if (m_basicShaderRhi == INVALID_HANDLE) {
            std::cerr << "PipelineManager: Failed to create basic RHI shader, using PBR fallback" << std::endl;
            hasBasicShaders = false;
        }
    }

    // Create PBR shader
    ShaderDesc pbrDesc{};
    pbrDesc.stages = shaderStageBits(ShaderStage::Vertex) | shaderStageBits(ShaderStage::Fragment);
    pbrDesc.vertexSource = pbrVertSrc;
    pbrDesc.fragmentSource = pbrFragSrc;
    pbrDesc.debugName = "PbrShader";

    m_pbrShaderRhi = m_rhi->createShader(pbrDesc);
    if (m_pbrShaderRhi == INVALID_HANDLE) {
        std::cerr << "PipelineManager: Failed to create PBR RHI shader" << std::endl;
        return false;
    }

    // If no basic shader was loaded, use PBR shader as fallback
    if (!hasBasicShaders) {
        std::cerr << "PipelineManager: Using PBR shader for basic shader calls" << std::endl;
        m_basicShaderRhi = m_pbrShaderRhi;
    }

    return true;
}

void PipelineManager::ensureObjectPipeline(SceneObject& obj, bool usePbr)
{
    if (!m_rhi) return;

    // Check if object already has the appropriate pipeline
    PipelineHandle currentPipeline = getObjectPipeline(obj, usePbr);
    if (currentPipeline != INVALID_HANDLE) {
        return; // Already has valid pipeline
    }

    // Generate pipeline key for caching
    std::string pipelineKey = generatePipelineKey(obj, usePbr);

    // Check cache first
    auto it = m_pipelineCache.find(pipelineKey);
    if (it != m_pipelineCache.end()) {
        // Use cached pipeline
        if (usePbr) {
            obj.rhiPipelinePbr = it->second;
        } else {
            obj.rhiPipelineBasic = it->second;
        }
        return;
    }

    // Create new pipeline
    PipelineHandle newPipeline = INVALID_HANDLE;
    if (usePbr) {
        newPipeline = createPbrPipeline(obj);
        obj.rhiPipelinePbr = newPipeline;
    } else {
        newPipeline = createBasicPipeline(obj);
        obj.rhiPipelineBasic = newPipeline;
    }

    // Cache the pipeline
    if (newPipeline != INVALID_HANDLE) {
        m_pipelineCache[pipelineKey] = newPipeline;
    }
}

PipelineHandle PipelineManager::getOrCreatePipeline(const std::string& pipelineName, bool usePbr)
{
    auto it = m_pipelineCache.find(pipelineName);
    if (it != m_pipelineCache.end()) {
        return it->second;
    }

    // For generic pipeline creation, we need default pipeline configurations
    // This is a simplified version - in practice you'd want more parameters
    PipelineDesc pd{};
    pd.topology = PrimitiveTopology::Triangles;
    pd.shader = usePbr ? m_pbrShaderRhi : m_basicShaderRhi;
    pd.debugName = pipelineName;

    PipelineHandle pipeline = m_rhi->createPipeline(pd);
    if (pipeline != INVALID_HANDLE) {
        m_pipelineCache[pipelineName] = pipeline;
    }

    return pipeline;
}

bool PipelineManager::hasValidPipeline(const SceneObject& obj) const
{
    return (obj.rhiPipelineBasic != INVALID_HANDLE) || (obj.rhiPipelinePbr != INVALID_HANDLE);
}

PipelineHandle PipelineManager::getObjectPipeline(const SceneObject& obj, bool usePbr) const
{
    return usePbr ? obj.rhiPipelinePbr : obj.rhiPipelineBasic;
}

void PipelineManager::setDefaultShaderPaths(const std::string& basicVert, const std::string& basicFrag,
                                           const std::string& pbrVert, const std::string& pbrFrag)
{
    m_basicVertPath = basicVert;
    m_basicFragPath = basicFrag;
    m_pbrVertPath = pbrVert;
    m_pbrFragPath = pbrFrag;
}

PipelineHandle PipelineManager::createBasicPipeline(const SceneObject& obj)
{
    if (!m_rhi || m_basicShaderRhi == INVALID_HANDLE) {
        return INVALID_HANDLE;
    }

    PipelineDesc pd{};
    pd.topology = PrimitiveTopology::Triangles;
    pd.shader = m_basicShaderRhi;
    pd.debugName = obj.name + ":pipeline_basic";

    // Set up vertex bindings based on available buffers
    if (obj.rhiVboPositions != INVALID_HANDLE) {
        VertexBinding bPos{};
        bPos.binding = 0;
        bPos.stride = 3 * sizeof(float);
        bPos.buffer = obj.rhiVboPositions;
        pd.vertexBindings.push_back(bPos);
    }

    if (obj.rhiVboNormals != INVALID_HANDLE) {
        VertexBinding bNormal{};
        bNormal.binding = 1;
        bNormal.stride = 3 * sizeof(float);
        bNormal.buffer = obj.rhiVboNormals;
        pd.vertexBindings.push_back(bNormal);
    }

    if (obj.rhiVboTexCoords != INVALID_HANDLE) {
        VertexBinding bTexCoord{};
        bTexCoord.binding = 2;
        bTexCoord.stride = 2 * sizeof(float);
        bTexCoord.buffer = obj.rhiVboTexCoords;
        pd.vertexBindings.push_back(bTexCoord);
    }

    // Set index buffer if available
    if (obj.rhiEbo != INVALID_HANDLE) {
        pd.indexBuffer = obj.rhiEbo;
    }

    return m_rhi->createPipeline(pd);
}

PipelineHandle PipelineManager::createPbrPipeline(const SceneObject& obj)
{
    if (!m_rhi || m_pbrShaderRhi == INVALID_HANDLE) {
        return INVALID_HANDLE;
    }

    PipelineDesc pd{};
    pd.topology = PrimitiveTopology::Triangles;
    pd.shader = m_pbrShaderRhi;
    pd.debugName = obj.name + ":pipeline_pbr";

    // Set up vertex bindings based on available buffers
    if (obj.rhiVboPositions != INVALID_HANDLE) {
        VertexBinding bPos{};
        bPos.binding = 0;
        bPos.stride = 3 * sizeof(float);
        bPos.buffer = obj.rhiVboPositions;
        pd.vertexBindings.push_back(bPos);
    }

    if (obj.rhiVboNormals != INVALID_HANDLE) {
        VertexBinding bNormal{};
        bNormal.binding = 1;
        bNormal.stride = 3 * sizeof(float);
        bNormal.buffer = obj.rhiVboNormals;
        pd.vertexBindings.push_back(bNormal);
    }

    if (obj.rhiVboTexCoords != INVALID_HANDLE) {
        VertexBinding bTexCoord{};
        bTexCoord.binding = 2;
        bTexCoord.stride = 2 * sizeof(float);
        bTexCoord.buffer = obj.rhiVboTexCoords;
        pd.vertexBindings.push_back(bTexCoord);
    }

    // PBR shaders might use tangent data
    if (obj.rhiVboTangents != INVALID_HANDLE) {
        VertexBinding bTangent{};
        bTangent.binding = 3;
        bTangent.stride = 4 * sizeof(float);
        bTangent.buffer = obj.rhiVboTangents;
        pd.vertexBindings.push_back(bTangent);
    }

    // Set index buffer if available
    if (obj.rhiEbo != INVALID_HANDLE) {
        pd.indexBuffer = obj.rhiEbo;
    }

    return m_rhi->createPipeline(pd);
}

std::string PipelineManager::generatePipelineKey(const SceneObject& obj, bool usePbr) const
{
    std::ostringstream key;
    key << (usePbr ? "pbr" : "basic");
    key << "_pos:" << (obj.rhiVboPositions != INVALID_HANDLE ? "1" : "0");
    key << "_norm:" << (obj.rhiVboNormals != INVALID_HANDLE ? "1" : "0");
    key << "_tex:" << (obj.rhiVboTexCoords != INVALID_HANDLE ? "1" : "0");
    key << "_tang:" << (obj.rhiVboTangents != INVALID_HANDLE ? "1" : "0");
    key << "_idx:" << (obj.rhiEbo != INVALID_HANDLE ? "1" : "0");
    return key.str();
}

std::string PipelineManager::loadTextFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
