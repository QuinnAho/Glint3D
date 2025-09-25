#pragma once

#include <glm/glm.hpp>

struct CameraState {
    glm::vec3 position{0.0f, 0.0f, 10.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fov = 45.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
};
