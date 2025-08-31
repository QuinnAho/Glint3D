#ifndef USERINPUT_H
#define USERINPUT_H


#include "gl_platform.h"
#include <glm/glm.hpp>
#include <limits>
#include <vector>
#include <GLFW/glfw3.h>

// Include the centralized Ray class
#include "ray.h"
#include "gizmo.h"

// Forward declaration of Application
class Application;

class UserInput {
public:
    // Constructor takes a pointer to the Application to access scene objects, camera, etc.
    UserInput(Application* app);

    // Callback functions to be called from GLFW callbacks
    void mouseCallback(double xpos, double ypos);
    void mouseButtonCallback(int button, int action, int mods);

    // Performs picking based on the current mouse position
    void pickObject(double mouseX, double mouseY);

private:
    Application* m_app;

    // Mouse state for this input handler
    bool m_firstMouse;
    double m_lastX, m_lastY;

    // Gizmo state (interaction only; render state lives in Application)
    bool m_gizmoDragging = false;
    GizmoAxis m_activeAxis = GizmoAxis::None;
    float m_axisStartS = 0.0f;
    glm::vec3 m_dragOriginWorld{0};
    glm::vec3 m_dragAxisDir{0};
    glm::mat4 m_modelStart{1.0f};
    int m_dragObjectIndex = -1;

    Ray makeRayFromScreen(double mouseX, double mouseY) const;
};

#endif
