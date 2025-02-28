#pragma once

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "objloader.h"
#include "AxisRenderer.h"

// Forward declare ImGui setup or include ImGui headers if desired
// #include "imgui.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h"

class Application
{
public:
    Application();
    ~Application();

    // Initialize GLFW, create window, load OpenGL, set callbacks, etc.
    bool init(const std::string& windowTitle, int width, int height);

    // Main run loop: process input, render, etc.
    void run();

private:
    // GLFWwindow pointer
    GLFWwindow* m_window;

    // Window dimensions
    int m_windowWidth;
    int m_windowHeight;

    // Shaders & Buffers
    GLuint m_shaderProgram;
    GLuint m_VAO, m_VBO, m_EBO;

    // Rendering mode: 0 = Points, 1 = Wireframe, 2 = Solid
    int m_renderMode;

    // AxisRenderer instance
    AxisRenderer m_axisRenderer;

    // OBJ loading
    ObjLoader m_objLoader;

    // Matrices
    glm::mat4 m_modelMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    // Camera parameters
    glm::vec3 m_cameraPos;
    glm::vec3 m_cameraFront;
    glm::vec3 m_cameraUp;

    float m_fov;
    float m_nearClip;
    float m_farClip;
    float m_cameraSpeed;
    float m_sensitivity;

    // Mouse input states
    bool m_firstMouse;
    bool m_rightMousePressed;
    bool m_leftMousePressed;
    double m_lastX, m_lastY;
    float m_pitch, m_yaw;

    // Model center for pivot rotations
    glm::vec3 m_modelCenter;

private:
    // Internal setup methods
    bool initGLFW(const std::string& windowTitle, int width, int height);
    bool initGLAD();
    void initImGui();
    void setupOpenGL();
    void cleanup();

    // Main loop steps
    void processInput();
    void renderScene();
    void renderGUI();
    void renderAxisIndicator();

    // GLFW callbacks (must be static or free functions)
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Helpers to retrieve a pointer to this class from GLFW
    static Application* getApplication(GLFWwindow* window);
};
