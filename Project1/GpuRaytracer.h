#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

/**
 * @brief Simple GPU based ray tracer using an OpenGL compute shader.
 *
 * The implementation is intentionally minimal: it renders a checkerboard
 * ground plane by casting rays in a compute shader.  The class is designed so
 * it can be expanded later to handle full triangle meshes and lighting using
 * shader storage buffers or textures.
 */
class GpuRaytracer {
public:
    GpuRaytracer();
    ~GpuRaytracer();

    /**
     * Compile the compute shader used for ray tracing.
     * @return true on success, false otherwise.
     */
    bool init();

    /**
     * Dispatch the compute shader to render into the provided texture.
     *
     * @param outputTex OpenGL texture with format RGBA32F bound as image unit 0.
     * @param width     Width of the texture in pixels.
     * @param height    Height of the texture in pixels.
     * @param camPos    Camera position in world space.
     * @param camFront  Camera forward vector.
     * @param camUp     Camera up vector.
     * @param fovDeg    Field of view in degrees.
     */
    void render(GLuint outputTex,
                int width,
                int height,
                const glm::vec3& camPos,
                const glm::vec3& camFront,
                const glm::vec3& camUp,
                float fovDeg);

private:
    GLuint m_program; ///< handle to compute shader program
};

