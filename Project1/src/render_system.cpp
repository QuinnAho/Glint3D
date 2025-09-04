#include "render_system.h"
#include "scene_manager.h"
#include "light.h"
#include "axisrenderer.h"
#include "grid.h" 
#include "gizmo.h"
#include "raytracer.h"
#include "shader.h"
#include "gl_platform.h"
#include <iostream>

RenderSystem::RenderSystem()
{
    // Initialize renderer components
    m_axisRenderer = std::make_unique<AxisRenderer>();
    m_grid = std::make_unique<Grid>();
    m_raytracer = std::make_unique<Raytracer>();
    m_gizmo = std::make_unique<Gizmo>();
}

RenderSystem::~RenderSystem()
{
    shutdown();
}

bool RenderSystem::init(int windowWidth, int windowHeight)
{
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    // Initialize sub-systems
    updateProjectionMatrix(windowWidth, windowHeight);
    updateViewMatrix();
    
    return true;
}

void RenderSystem::shutdown()
{
    // Cleanup OpenGL resources
    m_axisRenderer.reset();
    m_grid.reset();
    m_raytracer.reset();
    m_basicShader.reset();
    m_pbrShader.reset();
}

void RenderSystem::render(const SceneManager& scene, const Light& lights)
{
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Choose render path based on mode
    switch (m_renderMode) {
        case RenderMode::Raytrace:
            renderRaytraced(scene, lights);
            break;
        default:
            renderRasterized(scene, lights);
            break;
    }
    
    // Render debug elements
    if (m_showGrid && m_grid) {
        // Grid rendering would go here
    }
    
    if (m_showAxes && m_axisRenderer) {
        // Axis rendering would go here  
    }
    
    updateRenderStats(scene);
}

bool RenderSystem::renderToTexture(const SceneManager& scene, const Light& lights,
                                  GLuint textureId, int width, int height)
{
    // Implement offscreen rendering
    // This is a placeholder - full implementation would create FBO, render to texture, etc.
    return false;
}

bool RenderSystem::renderToPNG(const SceneManager& scene, const Light& lights,
                               const std::string& path, int width, int height)
{
    // Implement PNG rendering
    // This is a placeholder - full implementation would render to framebuffer then save
    std::cout << "PNG rendering not implemented yet: " << path << "\n";
    return false;
}

void RenderSystem::updateViewMatrix()
{
    glm::vec3 target = m_camera.position + m_camera.front;
    m_viewMatrix = glm::lookAt(m_camera.position, target, m_camera.up);
}

void RenderSystem::updateProjectionMatrix(int windowWidth, int windowHeight)
{
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    m_projectionMatrix = glm::perspective(glm::radians(m_camera.fov), aspect, 
                                         m_camera.nearClip, m_camera.farClip);
}

bool RenderSystem::denoise(std::vector<glm::vec3>& color,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
    // Placeholder for denoising implementation
    return false;
}

void RenderSystem::renderRasterized(const SceneManager& scene, const Light& lights)
{
    // Render all scene objects using OpenGL
    const auto& objects = scene.getObjects();
    
    for (const auto& obj : objects) {
        renderObject(obj, lights);
    }
}

void RenderSystem::renderRaytraced(const SceneManager& scene, const Light& lights)
{
    // Use CPU raytracer
    if (m_raytracer) {
        // Raytracing implementation would go here
        std::cout << "Raytracing not fully implemented\n";
    }
}

void RenderSystem::renderObject(const SceneObject& obj, const Light& lights)
{
    // Basic object rendering
    if (obj.VAO == 0) return;
    
    glBindVertexArray(obj.VAO);
    
    // Set render mode
    switch (m_renderMode) {
        case RenderMode::Points:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case RenderMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }
    
    // Draw the object
    if (obj.EBO != 0) {
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
    }
    
    glBindVertexArray(0);
    
    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderSystem::updateRenderStats(const SceneManager& scene)
{
    // Update rendering statistics
    m_stats = {}; // Reset stats
    
    const auto& objects = scene.getObjects();
    m_stats.drawCalls = (int)objects.size();
    
    for (const auto& obj : objects) {
        m_stats.totalTriangles += obj.objLoader.getIndexCount() / 3;
    }
}