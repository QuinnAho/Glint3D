#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "ray.h"

class Triangle {
public:
    glm::vec3 v0, v1, v2; // Triangle vertices
    glm::vec3 normal;
    float reflectivity; // Reflectivity (0 = matte, 1 = mirror)

    Triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, float reflect = 0.0f)
        : v0(p0), v1(p1), v2(p2), reflectivity(reflect) {
        normal = glm::normalize(glm::cross(v1 - v0, v2 - v0)); // Compute normal
    }

    // Möller-Trumbore Algorithm for Ray-Triangle Intersection
    bool intersect(const Ray& ray, float& t, glm::vec3& hitNormal) const {
        const float EPSILON = 0.000001f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) return false; // Parallel, no intersection

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);
        if (v < 0.0f || u + v > 1.0f) return false;

        t = f * glm::dot(edge2, q);
        if (t > EPSILON) {
            hitNormal = normal;
            return true;
        }
        return false;
    }
};

#endif
