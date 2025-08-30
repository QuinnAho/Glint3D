#include "userinput.h"
#include "application.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Constructor
UserInput::UserInput(Application* app)
    : m_app(app), m_firstMouse(true), m_lastX(0), m_lastY(0)
{
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

    // Right mouse drag rotates the camera
    if (m_app->isRightMousePressed())
    {
        float newYaw = m_app->getYaw() + xOffset;
        float newPitch = m_app->getPitch() + yOffset;
        newPitch = glm::clamp(newPitch, -89.0f, 89.0f);
        m_app->setCameraAngles(newYaw, newPitch);
    }
    // Left mouse drag rotates the selected object
    if (m_app->isLeftMousePressed() && m_app->getSelectedObjectIndex() != -1)
    {
        SceneObject& selectedObj = m_app->getSceneObjects()[m_app->getSelectedObjectIndex()];

        // Skip static objects
        if (selectedObj.isStatic)
            return;

        // Compute the object's center from its bounding box (in object space)
        glm::vec3 objMin = selectedObj.objLoader.getMinBounds();
        glm::vec3 objMax = selectedObj.objLoader.getMaxBounds();
        glm::vec3 objCenter = (objMin + objMax) * 0.5f;

        glm::vec3 worldCenter = glm::vec3(selectedObj.modelMatrix * glm::vec4(objCenter, 1.0f));

        glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -worldCenter);
        glm::mat4 back = glm::translate(glm::mat4(1.0f), worldCenter);

        float angleX = -glm::radians(yOffset);
        float angleY = glm::radians(xOffset);

        selectedObj.modelMatrix = back
            * glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f))
            * toOrigin
            * selectedObj.modelMatrix;
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
            // Use current mouse coordinates for picking
            pickObject(m_lastX, m_lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            m_app->setLeftMousePressed(false);
            m_firstMouse = true;
        }
    }
}

// Picking function to select an object based on the mouse coordinates
void UserInput::pickObject(double mouseX, double mouseY)
{
    // Convert screen coordinates to normalized device coordinates (NDC)
    float xNDC = (2.0f * mouseX) / m_app->getWindowWidth() - 1.0f;
    float yNDC = 1.0f - (2.0f * mouseY) / m_app->getWindowHeight();

    // Create a clip-space position (assuming near plane z = -1)
    glm::vec4 rayClip(xNDC, yNDC, -1.0f, 1.0f);

    // Transform to eye space (inverse projection)
    glm::mat4 invProj = glm::inverse(m_app->getProjectionMatrix());
    glm::vec4 rayEye = invProj * rayClip;
    rayEye.z = -1.0f;
    rayEye.w = 0.0f;

    // Transform to world space (inverse view)
    glm::mat4 invView = glm::inverse(m_app->getViewMatrix());
    glm::vec4 rayWorld4 = invView * rayEye;
    glm::vec3 rayWorld = glm::normalize(glm::vec3(rayWorld4));

    // Construct a Ray using the centralized Ray class
    Ray ray(m_app->getCameraPosition(), rayWorld);

    // Use a non-const reference to the scene objects so we can call non-const getters
    std::vector<SceneObject>& sceneObjects = m_app->getSceneObjects();
    int selectedIndex = -1;
    float closestT = std::numeric_limits<float>::max();

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
                selectedIndex = static_cast<int>(i);
            }
        }
    }
    m_app->setSelectedObjectIndex(selectedIndex);
}
