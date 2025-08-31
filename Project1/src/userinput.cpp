#include "userinput.h"
#include "application.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// Constructor
UserInput::UserInput(Application* app)
    : m_app(app), m_firstMouse(true), m_lastX(0), m_lastY(0)
{
}

Ray UserInput::makeRayFromScreen(double mouseX, double mouseY) const
{
    float xNDC = (2.0f * (float)mouseX) / m_app->getWindowWidth() - 1.0f;
    float yNDC = 1.0f - (2.0f * (float)mouseY) / m_app->getWindowHeight();
    glm::vec4 rayClip(xNDC, yNDC, -1.0f, 1.0f);
    glm::mat4 invProj = glm::inverse(m_app->getProjectionMatrix());
    glm::vec4 rayEye = invProj * rayClip; rayEye.z = -1.0f; rayEye.w = 0.0f;
    glm::mat4 invView = glm::inverse(m_app->getViewMatrix());
    glm::vec4 rayWorld4 = invView * rayEye;
    glm::vec3 rayWorld = glm::normalize(glm::vec3(rayWorld4));
    return Ray(m_app->getCameraPosition(), rayWorld);
}

// Mouse movement callback
void UserInput::mouseCallback(double xpos, double ypos)
{
    if (m_firstMouse)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xOffset = static_cast<float>(xpos - m_lastX);
    float yOffset = static_cast<float>(m_lastY - ypos);
    m_lastX = xpos;
    m_lastY = ypos;

    // Multiply by sensitivity from Application
    xOffset *= m_app->getMouseSensitivity();
    yOffset *= m_app->getMouseSensitivity();

    // Gizmo drag (left mouse) has priority over old object-rotate behavior
    if (m_gizmoDragging && m_dragObjectIndex >= 0 && m_dragObjectIndex < (int)m_app->getSceneObjects().size())
    {
        Ray ray = makeRayFromScreen(xpos, ypos);
        // Recompute s along the same axis from the same origin
        float sNow = m_axisStartS;
        glm::vec3 tmpDir; GizmoAxis tmpAxis;
        // Fake a local pick strictly along the drag axis (avoid selecting another axis mid-drag)
        // Project ray to the line and measure parameter s
        auto closestParams = [](const glm::vec3& r0, const glm::vec3& rd, const glm::vec3& s0, const glm::vec3& sd, float& t, float& s)->bool{
            const float a = glm::dot(rd, rd);
            const float b = glm::dot(rd, sd);
            const float c = glm::dot(sd, sd);
            const glm::vec3 w0 = r0 - s0;
            const float d = glm::dot(rd, w0);
            const float e = glm::dot(sd, w0);
            const float denom = a*c - b*b; if (std::abs(denom) < 1e-6f) return false; t = (b*e - c*d) / denom; s = (a*e - b*d) / denom; return true; };
        float tParam=0.0f; (void)tmpAxis; (void)tmpDir;
        if (closestParams(ray.origin, ray.direction, m_dragOriginWorld, m_dragAxisDir, tParam, sNow))
        {
            // Clamp s to positive range (triad length ~ scale; allow a bit extra during drag)
            sNow = glm::max(0.0f, sNow);
            float deltaS = sNow - m_axisStartS;

            auto& obj = m_app->getSceneObjects()[m_dragObjectIndex];
            glm::vec3 pivot = m_dragOriginWorld; // world pivot
            if (m_app->getGizmoMode() == GizmoMode::Translate)
            {
                // snapping
                if (m_app->isSnapEnabled()) {
                    float step = m_app->snapTranslateStep();
                    deltaS = step * std::round(deltaS / step);
                }
                glm::vec3 delta = m_dragAxisDir * deltaS;
                obj.modelMatrix = glm::translate(glm::mat4(1.0f), delta) * m_modelStart;
            }
            else if (m_app->getGizmoMode() == GizmoMode::Rotate)
            {
                float degreesPerUnit = 30.0f; // sensitivity
                float angleDeg = deltaS * degreesPerUnit;
                if (m_app->isSnapEnabled()) {
                    float step = m_app->snapRotateStepDeg();
                    angleDeg = step * std::round(angleDeg / step);
                }
                float angleRad = glm::radians(angleDeg);
                glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -pivot);
                glm::mat4 back = glm::translate(glm::mat4(1.0f), pivot);
                obj.modelMatrix = back * glm::rotate(glm::mat4(1.0f), angleRad, m_dragAxisDir) * toOrigin * m_modelStart;
            }
            else if (m_app->getGizmoMode() == GizmoMode::Scale)
            {
                float scalePerUnit = 0.3f; // sensitivity
                float f = glm::max(0.05f, 1.0f + deltaS * scalePerUnit);
                if (m_app->isSnapEnabled()) {
                    float step = m_app->snapScaleStep();
                    f = step * std::round(f / step);
                }
                glm::vec3 s(1.0f);
                if (m_activeAxis == GizmoAxis::X) s.x = f;
                if (m_activeAxis == GizmoAxis::Y) s.y = f;
                if (m_activeAxis == GizmoAxis::Z) s.z = f;
                glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -pivot);
                glm::mat4 back = glm::translate(glm::mat4(1.0f), pivot);
                obj.modelMatrix = back * glm::scale(glm::mat4(1.0f), s) * toOrigin * m_modelStart;
            }
        }
        return; // handled
    }

    // Right mouse drag rotates the camera
    if (m_app->isRightMousePressed())
    {
        float newYaw = m_app->getYaw() + xOffset;
        float newPitch = m_app->getPitch() + yOffset;
        newPitch = glm::clamp(newPitch, -89.0f, 89.0f);
        m_app->setCameraAngles(newYaw, newPitch);
    }
}

// Mouse button callback
void UserInput::mouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            m_app->setRightMousePressed(true);
        }
        else if (action == GLFW_RELEASE)
        {
            m_app->setRightMousePressed(false);
            m_firstMouse = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_app->setLeftMousePressed(true);
            // If an object is selected, try to grab gizmo axis first
            int sel = m_app->getSelectedObjectIndex();
            bool grabbed = false;
            if (sel >= 0 && sel < (int)m_app->getSceneObjects().size())
            {
                glm::vec3 center = m_app->getSelectedObjectCenterWorld();
                float dist = glm::length(m_app->getCameraPosition() - center);
                float gscale = glm::clamp(dist * 0.15f, 0.5f, 10.0f);
                // Orientation basis (local or world)
                glm::mat3 R(1.0f);
                if (m_app->isGizmoLocalSpace()) {
                    const auto& obj = m_app->getSceneObjects()[sel];
                    glm::mat3 M3(obj.modelMatrix);
                    R[0] = glm::normalize(glm::vec3(M3[0]));
                    R[1] = glm::normalize(glm::vec3(M3[1]));
                    R[2] = glm::normalize(glm::vec3(M3[2]));
                }
                Ray ray = makeRayFromScreen(m_lastX, m_lastY);
                GizmoAxis ax; float s0; glm::vec3 axisDir;
                if (m_app->getGizmo().pickAxis(ray, center, R, gscale, ax, s0, axisDir))
                {
                    m_app->setGizmoAxis(ax);
                    m_activeAxis = ax;
                    m_axisStartS = s0;
                    m_dragOriginWorld = center;
                    m_dragAxisDir = axisDir;
                    m_dragObjectIndex = sel;
                    m_modelStart = m_app->getSceneObjects()[sel].modelMatrix;
                    m_gizmoDragging = true;
                    grabbed = true;
                }
            }
            if (!grabbed)
            {
                // Fall back to selection picking
                pickObject(m_lastX, m_lastY);
                m_activeAxis = GizmoAxis::None;
                m_app->setGizmoAxis(GizmoAxis::None);
            }
        }
        else if (action == GLFW_RELEASE)
        {
            m_app->setLeftMousePressed(false);
            m_firstMouse = true;
            m_gizmoDragging = false;
        }
    }
}

// Picking function to select an object based on the mouse coordinates
void UserInput::pickObject(double mouseX, double mouseY)
{
    Ray ray = makeRayFromScreen(mouseX, mouseY);

    // Use a non-const reference to the scene objects so we can call non-const getters
    std::vector<SceneObject>& sceneObjects = m_app->getSceneObjects();
    int selectedObj = -1;
    int selectedLight = -1;
    float closestT = std::numeric_limits<float>::max();

    // Test objects AABB
    for (size_t i = 0; i < sceneObjects.size(); i++)
    {
        SceneObject& obj = sceneObjects[i];
        // Get object's axis-aligned bounding box (AABB) in object space
        glm::vec3 aabbMin = obj.objLoader.getMinBounds();
        glm::vec3 aabbMax = obj.objLoader.getMaxBounds();

        // Transform AABB to world space by computing the transformed vertices
        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());
        for (int j = 0; j < 8; j++)
        {
            glm::vec3 vertex(
                (j & 1) ? aabbMax.x : aabbMin.x,
                (j & 2) ? aabbMax.y : aabbMin.y,
                (j & 4) ? aabbMax.z : aabbMin.z
            );
            glm::vec4 transformed = obj.modelMatrix * glm::vec4(vertex, 1.0f);
            glm::vec3 tvertex = glm::vec3(transformed);
            worldMin = glm::min(worldMin, tvertex);
            worldMax = glm::max(worldMax, tvertex);
        }

        float t;
        if (m_app->rayIntersectsAABB(ray, worldMin, worldMax, t))
        {
            if (t < closestT)
            {
                closestT = t;
                selectedObj = static_cast<int>(i);
                selectedLight = -1;
            }
        }
    }

    // Test lights (small cube AABB around light position)
    for (int i = 0; i < m_app->getLightCount(); ++i)
    {
        glm::vec3 pos = m_app->getLightPosition(i);
        glm::vec3 he(0.12f);
        glm::vec3 mn = pos - he;
        glm::vec3 mx = pos + he;
        float t;
        if (m_app->rayIntersectsAABB(ray, mn, mx, t))
        {
            if (t < closestT)
            {
                closestT = t;
                selectedLight = i;
                selectedObj = -1;
            }
        }
    }

    // Apply selection (exclusive)
    if (selectedObj >= 0) {
        m_app->setSelectedObjectIndex(selectedObj);
        m_app->setSelectedLightIndex(-1);
    } else if (selectedLight >= 0) {
        m_app->setSelectedLightIndex(selectedLight);
        m_app->setSelectedObjectIndex(-1);
    } else {
        m_app->setSelectedObjectIndex(-1);
        m_app->setSelectedLightIndex(-1);
    }
}
