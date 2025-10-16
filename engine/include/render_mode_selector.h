// Machine Summary Block (ndjson)
// {"file":"engine/include/render_mode_selector.h","purpose":"Chooses raster, ray, or hybrid pipelines using material heuristics","exports":["RenderPipelineMode","RenderConfig","SceneAnalysis","RenderPipelineModeSelector","PipelineBuilder"],"depends_on":["SceneManager","RenderGraph","glint3d::RHI","MaterialCore"],"notes":["SceneAnalysis caches transparency/refraction stats","needsRayTracing and canAffordRayTracing gate mode selection","PipelineBuilder seeds predefined render graphs"]}
#pragma once

/**
 * @file render_mode_selector.h
 * @brief Heuristics for choosing raster, ray, or hybrid render pipelines.
 *
 * RenderPipelineModeSelector analyzes scene materials alongside RenderConfig preferences to decide
 * which render graph to execute. It scores transparency, refraction, volumetrics, triangle count,
 * and performance budgets, then caches the results with a readable reason string.
 *
 * SceneAnalysis aggregates the material statistics and cost estimates that power helper checks
 * such as needsRayTracing() and canAffordRayTracing(), letting the selector balance quality and
 * real-time constraints. Convenience builders configure ready-to-use graphs for preview, final,
 * or real-time output.
 *
 * @see render_pass.h
 * @see render_system.h
 */

#include <vector>
#include <memory>
#include "material_core.h"

// forward declarations
class SceneManager;
class RenderGraph;

namespace glint3d {
    class RHI;
}
using glint3d::RHI;

// rendering pipeline modes
enum class RenderPipelineMode {
    Raster,     // OpenGL rasterization (fast, SSR approximation)
    Ray,        // CPU ray tracing (slow, physically accurate)
    Auto        // Smart selection based on scene content
};

// render configuration
struct RenderConfig {
    RenderPipelineMode mode = RenderPipelineMode::Auto;
    bool isPreview = false;          // preview vs final quality
    bool forceRealtime = false;      // enforce real-time constraints
    int maxRayDepth = 8;             // max ray bounces for ray tracing
    int minSamples = 1;              // min samples per pixel
    int maxSamples = 64;             // max samples per pixel
    float qualityThreshold = 0.95f;  // quality threshold for auto mode
    bool enableDenoising = true;     // enable ai denoising for ray tracing
};

// scene analysis results
struct SceneAnalysis {
    bool hasTransparentMaterials = false;
    bool hasRefractiveGlass = false;
    bool hasComplexGeometry = false;
    bool hasVolumetricEffects = false;
    int totalTriangles = 0;
    int materialCount = 0;
    float estimatedRenderTime = 0.0f; // in seconds
    
    // detailed material analysis
    struct MaterialStats {
        int transparentCount = 0;
        int refractiveCount = 0;
        int emissiveCount = 0;
        int metallicCount = 0;
        float avgTransmission = 0.0f;
        float avgRoughness = 0.5f;
        float maxIOR = 1.0f;
    } materials;
};

// intelligent render mode selection system
class RenderPipelineModeSelector {
public:
    RenderPipelineModeSelector();
    ~RenderPipelineModeSelector() = default;
    
    // main selection interface
    RenderPipelineMode selectMode(const SceneManager& scene, const RenderConfig& config);
    RenderPipelineMode selectMode(const std::vector<MaterialCore>& materials, const RenderConfig& config);
    
    // scene analysis
    SceneAnalysis analyzeScene(const SceneManager& scene) const;
    SceneAnalysis analyzeMaterials(const std::vector<MaterialCore>& materials) const;
    
    // configuration
    void setPerformanceBudget(float maxRenderTimeSeconds) { m_maxRenderTime = maxRenderTimeSeconds; }
    void setQualityPriority(bool prioritizeQuality) { m_prioritizeQuality = prioritizeQuality; }
    void setHardwareCapabilities(bool hasRTCores, int coreCount) { 
        m_hasRTCores = hasRTCores; 
        m_coreCount = coreCount; 
    }
    
    // statistics and feedback
    const SceneAnalysis& getLastAnalysis() const { return m_lastAnalysis; }
    std::string getSelectionReason() const { return m_selectionReason; }
    
    // threshold configuration
    void setTransparencyThreshold(float threshold) { m_transparencyThreshold = threshold; }
    void setComplexityThreshold(int triangles) { m_complexityThreshold = triangles; }
    void setIORThreshold(float ior) { m_iorThreshold = ior; }
    
private:
    // analysis state
    SceneAnalysis m_lastAnalysis;
    std::string m_selectionReason;
    
    // configuration
    float m_maxRenderTime = 30.0f;       // max acceptable render time in seconds
    bool m_prioritizeQuality = false;    // quality vs performance priority
    bool m_hasRTCores = false;           // hardware has rt cores
    int m_coreCount = 8;                 // cpu core count
    
    // thresholds for auto mode decisions
    float m_transparencyThreshold = 0.01f;  // min transmission to consider transparent
    float m_iorThreshold = 1.05f;           // min IOR to consider refractive
    int m_complexityThreshold = 100000;     // triangle count threshold
    float m_volumeThreshold = 0.001f;       // min thickness for volumetric effects
    
    // helper methods
    bool needsRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const;
    bool canAffordRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const;
    float estimateRayTracingTime(const SceneAnalysis& analysis) const;
    float estimateRasterTime(const SceneAnalysis& analysis) const;
    
    // decision heuristics
    bool hasSignificantRefraction(const SceneAnalysis& analysis) const;
    bool hasComplexVolumetrics(const SceneAnalysis& analysis) const;
    bool isRealTimeConstrained(const RenderConfig& config) const;
    
    void updateSelectionReason(RenderPipelineMode selected, const SceneAnalysis& analysis, const RenderConfig& config);
};

// pipeline builder that creates appropriate render graphs based on mode
class PipelineBuilder {
public:
    static std::unique_ptr<RenderGraph> createRasterPipeline(RHI* rhi);
    static std::unique_ptr<RenderGraph> createRayPipeline(RHI* rhi);
    static std::unique_ptr<RenderGraph> createHybridPipeline(RHI* rhi);
    
    // configure pipeline for specific scenarios
    static void configureForPreview(RenderGraph* graph);
    static void configureForFinalQuality(RenderGraph* graph);
    static void configureForRealTime(RenderGraph* graph);
};

// utility functions for cli integration
namespace RenderPipelineModeUtils {
    RenderPipelineMode parseRenderPipelineMode(const std::string& modeStr);
    std::string renderModeToString(RenderPipelineMode mode);
    std::vector<std::string> getAvailableModes();
    
    // cli help text
    std::string getUsageText();
    std::string getModeDescriptions();
}
