#pragma once

/**
 * @file render_mode_selector.h
 * @brief Intelligent Render Pipeline Mode Selection System
 *
 * OVERVIEW
 * ========
 * This file defines the automatic pipeline selection system that chooses between
 * rasterization (fast OpenGL) and ray tracing (slow CPU) based on scene content.
 *
 * Glint3D supports three rendering modes:
 *   - Raster: Fast GPU rasterization with deferred shading, SSR approximation
 *   - Ray:    Slow CPU ray tracing with BVH, physically accurate refraction/transmission
 *   - Auto:   Intelligent selection based on material analysis and performance budgets
 *
 * WHY THIS EXISTS
 * ===============
 *
 * Problem: Some scenes NEED ray tracing (refractive glass, volumetric effects), while
 * others work fine with rasterization. Forcing users to choose manually is error-prone.
 *
 * Solution: Automatically analyze materials and select the best pipeline:
 *   - If scene has transmission + high IOR → Use Ray (refraction needs path tracing)
 *   - If scene has only opaque/simple materials → Use Raster (much faster)
 *   - If scene has complex geometry but no transmission → Use Raster + SSR
 *
 * DECISION HEURISTICS
 * ===================
 *
 * The selector analyzes each material for:
 *   1. Transmission > 0.01 (transparent/translucent)
 *   2. IOR > 1.05 (refractive - needs Snell's law calculations)
 *   3. Thickness > 0.001 (volumetric attenuation)
 *   4. Emissive != 0 (self-illuminating - better with ray)
 *
 * Key Decision Logic (see needsRayTracing() and canAffordRayTracing()):
 *
 *   if (material.transmission > 0.01 && material.ior > 1.05):
 *       → NEEDS ray tracing (raster can't do accurate refraction)
 *   elif (scene.triangles > 100k && config.forceRealtime):
 *       → Use raster (too slow for interactive)
 *   elif (config.prioritizeQuality):
 *       → Use ray if available (user wants best quality)
 *   else:
 *       → Use raster (default to fast path)
 *
 * SCENE ANALYSIS
 * ==============
 *
 * The SceneAnalysis struct captures scene characteristics:
 *
 *   struct SceneAnalysis {
 *       bool hasRefractiveGlass;           // Any material with transmission + IOR
 *       bool hasTransparentMaterials;      // Any material with transmission
 *       bool hasVolumetricEffects;         // Any material with thickness
 *       int totalTriangles;                // Geometry complexity
 *       MaterialStats materials;           // Aggregated material properties
 *       float estimatedRenderTime;         // Time budget estimation
 *   };
 *
 * This analysis happens once per scene change (not per frame) via analyzeScene().
 *
 * USAGE
 * =====
 *
 * CLI Integration (see main.cpp --mode flag):
 *
 *   RenderConfig config;
 *   config.mode = parseRenderPipelineMode(modeStr);  // "raster", "ray", "auto"
 *   config.enableDenoising = true;
 *   config.maxRayDepth = 8;
 *
 *   RenderPipelineModeSelector selector;
 *   RenderPipelineMode selectedMode = selector.selectMode(scene, config);
 *
 *   if (selectedMode == Ray) {
 *       std::cout << "Using ray tracing: " << selector.getSelectionReason() << "\n";
 *   }
 *
 * Programmatic Usage (see RenderSystem::renderUnified):
 *
 *   // Collect all materials from scene
 *   std::vector<MaterialCore> materials;
 *   for (const auto& obj : scene.getObjects()) {
 *       materials.push_back(obj.materialCore);
 *   }
 *
 *   // Select pipeline based on materials
 *   RenderConfig config;
 *   config.mode = RenderPipelineMode::Auto;
 *   RenderPipelineMode mode = m_pipelineSelector->selectMode(materials, config);
 *
 *   // Use appropriate render graph
 *   RenderGraph* graph = (mode == Raster) ? m_rasterGraph.get() : m_rayGraph.get();
 *   graph->execute(ctx);
 *
 * REAL-WORLD EXAMPLES
 * ===================
 *
 * Example 1: Opaque Stanford Bunny
 *   Materials: 1 opaque PBR (metallic=0.5, roughness=0.3, transmission=0)
 *   Decision: RASTER (no transmission, fast GPU path sufficient)
 *   Reason: "No refractive materials detected, using fast rasterization"
 *
 * Example 2: Glass Sphere
 *   Materials: 1 glass (transmission=1.0, ior=1.5, baseColor=white)
 *   Decision: RAY (transmission + IOR requires Snell's law path tracing)
 *   Reason: "Scene contains refractive materials (IOR=1.50), ray tracing required"
 *
 * Example 3: Mixed Scene (Opaque + Glass)
 *   Materials: 10 opaque + 1 glass
 *   Decision: RAY (even one refractive material forces ray pipeline)
 *   Reason: "1 of 11 materials are refractive, ray tracing required"
 *
 * Example 4: 10M Triangle Scene
 *   Materials: 1 glass, config.forceRealtime = true
 *   Decision: RASTER (performance budget exceeded despite needing ray)
 *   Reason: "Scene too complex for real-time ray tracing (estimated 120s render time)"
 *   Warning: Visual artifacts expected (SSR approximation for refraction)
 *
 * PERFORMANCE IMPLICATIONS
 * ========================
 *
 * Typical Frame Times (1920x1080, 64 spp):
 *   Raster Pipeline: 8-16ms  (60-120 FPS) - Interactive
 *   Ray Pipeline:    500-5000ms (0.2-2 FPS) - Offline
 *
 * The selector balances:
 *   - Visual Correctness (does raster approximate this material acceptably?)
 *   - Performance (can we render this in real-time?)
 *   - User Intent (preview vs final render quality)
 *
 * CONFIGURATION
 * =============
 *
 * You can tune the selector's behavior via:
 *
 *   selector.setPerformanceBudget(30.0f);          // Max 30s render time
 *   selector.setQualityPriority(true);             // Prefer quality over speed
 *   selector.setTransparencyThreshold(0.01f);      // Ignore transmission < 1%
 *   selector.setIORThreshold(1.05f);               // Ignore IOR < 1.05 (air=1.0)
 *   selector.setComplexityThreshold(100000);       // Max triangles for ray
 *
 * RELATION TO RENDER GRAPH
 * ========================
 *
 * The selector OUTPUT (Raster vs Ray) determines which RenderGraph is used:
 *
 *   RenderPipelineMode mode = selector.selectMode(scene, config);
 *
 *   if (mode == Raster) {
 *       // Use m_rasterGraph: GBuffer → DeferredLighting → SSR → Post
 *   } else {
 *       // Use m_rayGraph: RayIntegrator → Denoise → Post
 *   }
 *
 * See render_pass.h for pass details.
 *
 * FUTURE ENHANCEMENTS
 * ===================
 *
 * - Hybrid rendering: Raster opaque objects, ray trace glass only
 * - Learning-based time estimation: Train on actual render times
 * - Per-object mode selection: Mix raster + ray in single frame
 * - GPU ray tracing: Use RTX hardware when available (OptiX, DXR, Vulkan RT)
 *
 * @see RenderSystem::renderUnified() - Main selection point
 * @see render_pass.h - Pass definitions for raster/ray pipelines
 * @see MaterialCore - Material properties that drive selection
 */

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