#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "ray.h"
#include "glint3d/rhi_types.h"

// Forward declarations
namespace glint3d {
    class RHI;
}

enum class GizmoMode { Translate = 0, Rotate = 1, Scale = 2 };
enum class GizmoAxis { None = 0, X = 1, Y = 2, Z = 3 };

class Gizmo {
public:
    Gizmo() {}
    void init(glint3d::RHI* rhi);
    void cleanup();

    // Render a triad centered at origin, scaled to 'scale' length (axes are [0..1] scaled by 'scale').
    // Active axis is highlighted. Mode can be used to vary center color (optional).
    void render(const glm::mat4& view,
                const glm::mat4& proj,
                const glm::vec3& origin,
                const glm::mat3& orientation, // columns are world X/Y/Z unit axes for gizmo
                float scale,
                GizmoAxis active,
                GizmoMode mode);

    // Very small hit test: returns true if ray is close to an axis segment.
    // Outputs which axis was hit, the param 's' along that axis (0..scale), and world axis direction.
    bool pickAxis(const Ray& ray,
                  const glm::vec3& origin,
                  const glm::mat3& orientation,
                  float scale,
                  GizmoAxis& outAxis,
                  float& outS,
                  glm::vec3& outAxisDir) const;

private:
    glint3d::RHI* m_rhi = nullptr;
    glint3d::BufferHandle m_vertexBuffer;
    glint3d::PipelineHandle m_pipeline;
};
