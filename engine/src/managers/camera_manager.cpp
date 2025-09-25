#include "managers/camera_manager.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

CameraManager::CameraManager()
{
    updateViewMatrix();
    updateProjectionMatrix(1, 1);
}

void CameraManager::setCamera(const CameraState& state)
{
    m_camera = state;
    updateViewMatrix();
}

const CameraState& CameraManager::camera() const
{
    return m_camera;
}

CameraState& CameraManager::camera()
{
    return m_camera;
}

void CameraManager::updateViewMatrix()
{
    const glm::vec3 target = m_camera.position + m_camera.front;
    m_view = glm::lookAt(m_camera.position, target, m_camera.up);
}

void CameraManager::updateProjectionMatrix(int width, int height)
{
    const int safeWidth = std::max(width, 1);
    const int safeHeight = std::max(height, 1);
    const float aspect = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);

    const float nearPlane = std::max(m_camera.nearClip, 0.0001f);
    const float farPlane = std::max(m_camera.farClip, nearPlane + 0.0001f);

    m_projection = glm::perspective(glm::radians(m_camera.fov), aspect, nearPlane, farPlane);
}

const glm::mat4& CameraManager::viewMatrix() const
{
    return m_view;
}

const glm::mat4& CameraManager::projectionMatrix() const
{
    return m_projection;
}

\r\nvoid CameraManager::setViewMatrix(const glm::mat4& view)\r\n{\r\n    m_view = view;\r\n}\r\n\r\nvoid CameraManager::setProjectionMatrix(const glm::mat4& proj)\r\n{\r\n    m_projection = proj;\r\n}\r\n
