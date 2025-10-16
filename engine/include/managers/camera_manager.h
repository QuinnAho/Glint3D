// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/camera_manager.h","purpose":"Owns camera state matrices and updates camera UBO data","exports":["CameraState","CameraManager"],"depends_on":["glm","glint3d::uniform blocks"],"notes":["Caches view/projection matrices","Publishes camera uniform buffer binding"]}
#pragma once

/**
 * @file camera_manager.h
 * @brief Maintains viewport camera transforms and UBO uploads.
 */

#include "camera_state.h"
#include <glm/glm.hpp>

class CameraManager {
public:
    CameraManager();

    void setCamera(const CameraState& state);

    const CameraState& camera() const;
    CameraState& camera();

    void updateViewMatrix();
    void updateProjectionMatrix(int width, int height);
    void setProjectionMatrix(const glm::mat4& projection);

    const glm::mat4& viewMatrix() const;
    const glm::mat4& projectionMatrix() const;

private:
    CameraState m_camera;
    glm::mat4 m_view{1.0f};
    glm::mat4 m_projection{1.0f};
};

