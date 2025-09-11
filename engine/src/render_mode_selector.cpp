#include "render_mode_selector.h"
#include "scene_manager.h"
#include "render_pass.h"
#include "rhi/rhi.h"
#include <algorithm>
#include <sstream>
#include <iostream>

RenderModeSelector::RenderModeSelector() = default;

RenderMode RenderModeSelector::selectMode(const SceneManager& scene, const RenderConfig& config) {
    // Analyze the scene first
    m_lastAnalysis = analyzeScene(scene);
    
    // If mode is explicitly set, respect it
    if (config.mode != RenderMode::Auto) {
        updateSelectionReason(config.mode, m_lastAnalysis, config);
        return config.mode;
    }
    
    // Auto mode logic
    RenderMode selected = RenderMode::Raster; // Default to faster option
    
    // Check if ray tracing is beneficial
    if (needsRayTracing(m_lastAnalysis, config)) {
        // Check if we can afford ray tracing
        if (canAffordRayTracing(m_lastAnalysis, config)) {
            selected = RenderMode::Ray;
        } else {
            // Fall back to raster with SSR
            selected = RenderMode::Raster;
        }
    }
    
    // Override for preview mode - prefer speed
    if (config.isPreview && selected == RenderMode::Ray) {
        float rayTime = estimateRayTracingTime(m_lastAnalysis);
        if (rayTime > 5.0f) { // More than 5 seconds for preview
            selected = RenderMode::Raster;
        }
    }
    
    updateSelectionReason(selected, m_lastAnalysis, config);
    return selected;
}

RenderMode RenderModeSelector::selectMode(const std::vector<MaterialCore>& materials, const RenderConfig& config) {
    m_lastAnalysis = analyzeMaterials(materials);
    
    if (config.mode != RenderMode::Auto) {
        updateSelectionReason(config.mode, m_lastAnalysis, config);
        return config.mode;
    }
    
    RenderMode selected = RenderMode::Raster;
    
    if (needsRayTracing(m_lastAnalysis, config)) {
        if (canAffordRayTracing(m_lastAnalysis, config)) {
            selected = RenderMode::Ray;
        }
    }
    
    updateSelectionReason(selected, m_lastAnalysis, config);
    return selected;
}

SceneAnalysis RenderModeSelector::analyzeScene(const SceneManager& scene) const {
    SceneAnalysis analysis;
    
    // TODO: Integrate with actual SceneManager API
    // For now, this is a placeholder implementation
    
    // Analyze geometry complexity
    analysis.totalTriangles = 50000; // Placeholder
    analysis.hasComplexGeometry = analysis.totalTriangles > m_complexityThreshold;
    
    // This would need to iterate through scene objects and their materials
    std::vector<MaterialCore> materials; // Placeholder - would come from scene
    
    auto materialAnalysis = analyzeMaterials(materials);
    analysis.materials = materialAnalysis.materials;
    analysis.hasTransparentMaterials = materialAnalysis.hasTransparentMaterials;
    analysis.hasRefractiveGlass = materialAnalysis.hasRefractiveGlass;
    analysis.hasVolumetricEffects = materialAnalysis.hasVolumetricEffects;
    
    // Estimate render times
    analysis.estimatedRenderTime = estimateRayTracingTime(analysis);
    
    return analysis;
}

SceneAnalysis RenderModeSelector::analyzeMaterials(const std::vector<MaterialCore>& materials) const {
    SceneAnalysis analysis;
    analysis.materialCount = static_cast<int>(materials.size());
    
    if (materials.empty()) {
        return analysis;
    }
    
    SceneAnalysis::MaterialStats& stats = analysis.materials;
    
    float totalTransmission = 0.0f;
    float totalRoughness = 0.0f;
    
    for (const auto& material : materials) {
        // Count material types
        if (material.isTransparent()) {
            stats.transparentCount++;
            totalTransmission += material.transmission;
            
            // Check for refraction
            if (material.ior > m_iorThreshold && material.thickness > m_volumeThreshold) {
                stats.refractiveCount++;
                analysis.hasRefractiveGlass = true;
            }
        }
        
        if (material.isEmissive()) {
            stats.emissiveCount++;
        }
        
        if (material.isMetal()) {
            stats.metallicCount++;
        }
        
        // Check for volumetric effects
        if (material.thickness > m_volumeThreshold && material.transmission > 0.1f) {
            analysis.hasVolumetricEffects = true;
        }
        
        totalRoughness += material.roughness;
        stats.maxIOR = std::max(stats.maxIOR, material.ior);
    }
    
    // Calculate averages
    if (stats.transparentCount > 0) {
        stats.avgTransmission = totalTransmission / stats.transparentCount;
    }
    stats.avgRoughness = totalRoughness / materials.size();
    
    // Overall flags
    analysis.hasTransparentMaterials = stats.transparentCount > 0;
    
    return analysis;
}

bool RenderModeSelector::needsRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const {
    // Strong indicators for ray tracing
    if (hasSignificantRefraction(analysis)) {
        return true;
    }
    
    if (hasComplexVolumetrics(analysis)) {
        return true;
    }
    
    // Quality priority overrides
    if (m_prioritizeQuality && analysis.hasTransparentMaterials) {
        return true;
    }
    
    return false;
}

bool RenderModeSelector::canAffordRayTracing(const SceneAnalysis& analysis, const RenderConfig& config) const {
    if (isRealTimeConstrained(config)) {
        return false;
    }
    
    float estimatedTime = estimateRayTracingTime(analysis);
    
    // Check against time budget
    if (estimatedTime > m_maxRenderTime) {
        return false;
    }
    
    // Hardware considerations
    if (!m_hasRTCores && analysis.totalTriangles > 500000) {
        return false;
    }
    
    return true;
}

float RenderModeSelector::estimateRayTracingTime(const SceneAnalysis& analysis) const {
    float baseTime = 1.0f; // Base time in seconds
    
    // Scale by geometry complexity
    float geometryFactor = std::max(1.0f, analysis.totalTriangles / 10000.0f);
    
    // Scale by material complexity
    float materialFactor = 1.0f;
    if (analysis.hasRefractiveGlass) {
        materialFactor *= 3.0f; // Refraction is expensive
    }
    if (analysis.hasVolumetricEffects) {
        materialFactor *= 2.0f; // Volume rendering is expensive
    }
    
    // Hardware scaling
    float hardwareFactor = 8.0f / m_coreCount; // Assume 8-core baseline
    if (m_hasRTCores) {
        hardwareFactor *= 0.3f; // RT cores significantly faster
    }
    
    return baseTime * geometryFactor * materialFactor * hardwareFactor;
}

float RenderModeSelector::estimateRasterTime(const SceneAnalysis& analysis) const {
    // Rasterization is generally much faster and more predictable
    float baseTime = 0.016f; // 60 FPS baseline
    
    float geometryFactor = std::max(1.0f, analysis.totalTriangles / 100000.0f);
    float materialFactor = 1.0f + (analysis.materials.transparentCount * 0.1f);
    
    return baseTime * geometryFactor * materialFactor;
}

bool RenderModeSelector::hasSignificantRefraction(const SceneAnalysis& analysis) const {
    return analysis.hasRefractiveGlass && 
           analysis.materials.refractiveCount > 0 && 
           analysis.materials.avgTransmission > 0.5f;
}

bool RenderModeSelector::hasComplexVolumetrics(const SceneAnalysis& analysis) const {
    return analysis.hasVolumetricEffects && 
           analysis.materials.refractiveCount > 2;
}

bool RenderModeSelector::isRealTimeConstrained(const RenderConfig& config) const {
    return config.forceRealtime || config.isPreview;
}

void RenderModeSelector::updateSelectionReason(RenderMode selected, const SceneAnalysis& analysis, const RenderConfig& config) {
    std::ostringstream reason;
    
    switch (selected) {
        case RenderMode::Raster:
            reason << "Rasterization selected: ";
            if (config.mode == RenderMode::Raster) {
                reason << "explicitly requested";
            } else if (config.forceRealtime) {
                reason << "real-time constraint";
            } else if (!analysis.hasRefractiveGlass) {
                reason << "no complex refraction";
            } else {
                reason << "performance budget (" << estimateRayTracingTime(analysis) << "s > " << m_maxRenderTime << "s)";
            }
            break;
            
        case RenderMode::Ray:
            reason << "Ray tracing selected: ";
            if (config.mode == RenderMode::Ray) {
                reason << "explicitly requested";
            } else if (hasSignificantRefraction(analysis)) {
                reason << "significant refraction effects (" << analysis.materials.refractiveCount << " refractive materials)";
            } else if (hasComplexVolumetrics(analysis)) {
                reason << "complex volumetric effects";
            } else if (m_prioritizeQuality) {
                reason << "quality priority enabled";
            }
            break;
            
        case RenderMode::Auto:
            reason << "Auto mode (this shouldn't happen)";
            break;
    }
    
    m_selectionReason = reason.str();
}

// PipelineBuilder implementation
std::unique_ptr<RenderGraph> PipelineBuilder::createRasterPipeline(RHI* rhi) {
    auto graph = std::make_unique<RenderGraph>(rhi);
    
    // Raster pipeline: GBuffer → Lighting → SSR → Post → Readback
    graph->addPass(std::make_unique<GBufferPass>());
    graph->addPass(std::make_unique<LightingPass>());
    graph->addPass(std::make_unique<SSRRefractionPass>());
    graph->addPass(std::make_unique<PostPass>());
    graph->addPass(std::make_unique<ReadbackPass>());
    
    return graph;
}

std::unique_ptr<RenderGraph> PipelineBuilder::createRayPipeline(RHI* rhi) {
    auto graph = std::make_unique<RenderGraph>(rhi);
    
    // Ray pipeline: Integrator → Denoise → Tonemap → Readback
    graph->addPass(std::make_unique<IntegratorPass>());
    graph->addPass(std::make_unique<DenoisePass>());
    graph->addPass(std::make_unique<TonemapPass>());
    graph->addPass(std::make_unique<ReadbackPass>());
    
    return graph;
}

std::unique_ptr<RenderGraph> PipelineBuilder::createHybridPipeline(RHI* rhi) {
    // For now, hybrid just selects one or the other
    // Future implementation could mix techniques
    return createRasterPipeline(rhi);
}

void PipelineBuilder::configureForPreview(RenderGraph* graph) {
    // Reduce quality for faster preview
    if (auto* integrator = dynamic_cast<IntegratorPass*>(graph->getPass("IntegratorPass"))) {
        integrator->setSampleCount(4); // Very low sample count
        integrator->setMaxDepth(3);    // Shallow rays
    }
    
    // Disable expensive passes
    if (auto* denoise = graph->getPass("DenoisePass")) {
        denoise->setEnabled(false);
    }
}

void PipelineBuilder::configureForFinalQuality(RenderGraph* graph) {
    // Maximum quality settings
    if (auto* integrator = dynamic_cast<IntegratorPass*>(graph->getPass("IntegratorPass"))) {
        integrator->setSampleCount(64); // High sample count
        integrator->setMaxDepth(8);     // Deep rays
    }
    
    // Enable all quality passes
    if (auto* denoise = graph->getPass("DenoisePass")) {
        denoise->setEnabled(true);
    }
}

void PipelineBuilder::configureForRealTime(RenderGraph* graph) {
    // Real-time optimizations
    configureForPreview(graph); // Similar to preview but more aggressive
    
    if (auto* integrator = dynamic_cast<IntegratorPass*>(graph->getPass("IntegratorPass"))) {
        integrator->setSampleCount(1); // Single sample for real-time
        integrator->setMaxDepth(2);    // Minimal bounces
    }
}

// Utility functions
namespace RenderModeUtils {
    
RenderMode parseRenderMode(const std::string& modeStr) {
    std::string lower = modeStr;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "raster" || lower == "rasterize" || lower == "opengl") {
        return RenderMode::Raster;
    } else if (lower == "ray" || lower == "raytrace" || lower == "raytracing") {
        return RenderMode::Ray;
    } else if (lower == "auto" || lower == "automatic") {
        return RenderMode::Auto;
    } else {
        std::cerr << "[RenderModeUtils] Unknown render mode: " << modeStr << ", defaulting to Auto\n";
        return RenderMode::Auto;
    }
}

std::string renderModeToString(RenderMode mode) {
    switch (mode) {
        case RenderMode::Raster: return "raster";
        case RenderMode::Ray: return "ray";
        case RenderMode::Auto: return "auto";
        default: return "unknown";
    }
}

std::vector<std::string> getAvailableModes() {
    return {"raster", "ray", "auto"};
}

std::string getUsageText() {
    return "Usage: --mode <raster|ray|auto>\n"
           "  raster: Force OpenGL rasterization (fast, SSR approximation)\n"
           "  ray:    Force CPU ray tracing (slow, physically accurate)\n"
           "  auto:   Smart selection based on scene content (default)";
}

std::string getModeDescriptions() {
    return "Render Mode Descriptions:\n"
           "\n"
           "raster:\n"
           "  - OpenGL hardware rasterization\n"
           "  - Real-time performance\n"
           "  - Screen-space refraction approximation\n"
           "  - Good for: Preview, real-time interaction, opaque materials\n"
           "\n"
           "ray:\n"
           "  - CPU-based ray tracing\n"
           "  - Physically accurate\n"
           "  - Full refraction, reflection, volumetrics\n"
           "  - Good for: Final renders, glass materials, complex lighting\n"
           "\n"
           "auto:\n"
           "  - Intelligent pipeline selection\n"
           "  - Analyzes scene content and performance budget\n"
           "  - Chooses optimal quality/performance balance\n"
           "  - Good for: General use, when unsure which mode to use";
}

} // namespace RenderModeUtils