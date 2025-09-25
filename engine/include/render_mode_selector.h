#pragma once

#include <vector>
#include <memory>
#include "material_core.h"

// Forward declarations
class SceneManager;
class RenderGraph;

namespace glint3d {
    class RHI;
}

// Rendering pipeline modes
enum class RenderPipelineMode {
    Raster,     // OpenGL rasterization (fast, SSR approximation)
    Ray,        // CPU ray tracing (slow, physically accurate)
    Auto        // Smart selection based on scene content
};

// Render configuration
struct RenderConfig {
    RenderPipelineMode mode = RenderPipelineMode::Auto;
    bool isPreview = false;          // Preview vs final quality
    bool forceRealtime = false;      // Force real-time constraints
    int maxRayDepth = 8;             // Max ray bounces for ray tracing
    int minSamples = 1;              // Min samples per pixel
    int maxSamples = 64;             // Max samples per pixel
    float qualityThreshold = 0.95f;  // Quality threshold for auto mode
    bool enableDenoising = true;     // Enable AI denoising for ray tracing
};

// Scene analysis results
struct SceneAnalysis {
    bool hasTransparentMaterials = false;
    bool hasRefractiveGlass = false;
    bool hasComplexGeometry = false;
    bool hasVolumetricEffects = false;
    int totalTriangles = 0;
    int materialCount = 0;
    float estimatedRenderTime = 0.0f; // In seconds
    
    // Detailed material analysis
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

// Intelligent render mode selection system
class RenderPipelineModeSelector {
public:
    RenderPipelineModeSelector();
    ~RenderPipelineModeSelector() = default;
    
    // Main selection interface
    RenderPipelineMode selectMode(const SceneManager& scene, const RenderConfig& config);
    RenderPipelineMode selectMode(const std::vector<MaterialCore>& materials, const RenderConfig& config);
    
    // Scene analysis
    SceneAnalysis analyzeScene(const SceneManager& scene) const;
    SceneAnalysis analyzeMaterials(const std::vector<MaterialCore>& materials) const;
    
    // Configuration
    void setPerformanceBudget(float maxRenderTimeSeconds) { m_maxRenderTime = maxRenderTimeSeconds; }
    void setQualityPriority(bool prioritizeQuality) { m_prioritizeQuality = prioritizeQuality; }
    void setHardwareCapabilities(bool hasRTCores, int coreCount) { 
        m_hasRTCores = hasRTCores; 
        m_coreCount = coreCount; 
    }
    
    // Statistics and feedback
    const SceneAnalysis& getLastAnalysis() const { return m_lastAnalysis; }
    std::string getSelectionReason() const { return m_selectionReason; }
    
    // Threshold configuration
    void setTransparencyThreshold(float threshold) { m_transparencyThreshold = threshold; }
    void setComplexityThreshold(int triangles) { m_complexityThreshold = triangles; }
    void setIORThreshold(float ior) { m_iorThreshold = ior; }
    
private:
    // Analysis state
    SceneAnalysis m_lastAnalysis;
    std::string m_selectionReason;
    
    // Configuration
    float m_maxRenderTime = 30.0f;       // Max acceptable render time in seconds
    bool m_prioritizeQuality = false;    // Quality vs performance priority
    bool m_hasRTCores = false;           // Hardware has RT cores
    int m_coreCount = 8;                 // CPU core count
    
    // Thresholds for auto mode decisions
    float m_transparencyThreshold = 0.01f;  // Min transmission to consider transparent
    float m_iorThreshold = 1.05f;           // Min IOR to consider refractive
    int m_complexityThreshold = 100000;     // Triangle count threshold
    float m_volumeThreshold = 0.001f;       // Min thickness for volumetric effects
    
    // Helper methods
    bool needsRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const;
    bool canAffordRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const;
    float estimateRayTracingTime(const SceneAnalysis& analysis) const;
    float estimateRasterTime(const SceneAnalysis& analysis) const;
    
    // Decision heuristics
    bool hasSignificantRefraction(const SceneAnalysis& analysis) const;
    bool hasComplexVolumetrics(const SceneAnalysis& analysis) const;
    bool isRealTimeConstrained(const RenderConfig& config) const;
    
    void updateSelectionReason(RenderPipelineMode selected, const SceneAnalysis& analysis, const RenderConfig& config);
};

// Pipeline builder that creates appropriate render graphs based on mode
class PipelineBuilder {
public:
    static std::unique_ptr<RenderGraph> createRasterPipeline(glint3d::RHI* rhi);
    static std::unique_ptr<RenderGraph> createRayPipeline(glint3d::RHI* rhi);
    static std::unique_ptr<RenderGraph> createHybridPipeline(glint3d::RHI* rhi);
    
    // Configure pipeline for specific scenarios
    static void configureForPreview(RenderGraph* graph);
    static void configureForFinalQuality(RenderGraph* graph);
    static void configureForRealTime(RenderGraph* graph);
};

// Utility functions for CLI integration
namespace RenderPipelineModeUtils {
    RenderPipelineMode parseRenderPipelineMode(const std::string& modeStr);
    std::string renderModeToString(RenderPipelineMode mode);
    std::vector<std::string> getAvailableModes();
    
    // CLI help text
    std::string getUsageText();
    std::string getModeDescriptions();
}