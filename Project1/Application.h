#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "objloader.h"
#include "AxisRenderer.h"
#include "texture.h"
#include "light.h"
#include "raytracer.h"
#include "userinput.h"
#include "ray.h"  
#include "material.h"
#include "shader.h"
#include "grid.h"

// Structure to store per-object data
struct SceneObject {
    unsigned int VAO, VBO, EBO;
    glm::mat4 modelMatrix;
    ObjLoader objLoader;
    Texture* texture = nullptr;
    Shader* shader = nullptr;
    bool isStatic = false;
    glm::vec3 color = glm::vec3(1.0f);
    Material material;
    GLuint VBO_positions; 
    GLuint VBO_normals;
    std::string name;
};

class Application
{
public:
    // Accessor methods for input data
    float getMouseSensitivity() const { return m_sensitivity; }
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    glm::mat4 getProjectionMatrix() const { return m_projectionMatrix; }
    glm::mat4 getViewMatrix() const { return m_viewMatrix; }
    glm::vec3 getCameraPosition() const { return m_cameraPos; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

    bool isLeftMousePressed() const { return m_leftMousePressed; }
    bool isRightMousePressed() const { return m_rightMousePressed; }
    int getSelectedObjectIndex() const { return m_selectedObjectIndex; }
    // Provide both const and non-const access to the scene objects:
    const std::vector<SceneObject>& getSceneObjects() const { return m_sceneObjects; }
    std::vector<SceneObject>& getSceneObjects() { return m_sceneObjects; }

    // Setters to update state from input callbacks
    void setLeftMousePressed(bool pressed) { m_leftMousePressed = pressed; }
    void setRightMousePressed(bool pressed) { m_rightMousePressed = pressed; }
    void setSelectedObjectIndex(int index) { m_selectedObjectIndex = index; }
    void setCameraAngles(float yaw, float pitch);

    // Provide a method to test ray-AABB intersection
    bool rayIntersectsAABB(const Ray& ray, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float& t);

    // Expose the UserInput pointer so that GLFW callbacks can call it
    UserInput* getUserInput() { return m_userInput; }

public:
    Application();
    ~Application();

    // Initialize GLFW, create window, load OpenGL, set callbacks, etc.
    bool init(const std::string& windowTitle, int width, int height);

    // Main run loop: process input, render, etc.
    void run();

private:
    // GLFW window pointer
    GLFWwindow* m_window;

    // Window dimensions
    int m_windowWidth;
    int m_windowHeight;

    // Shaders & Buffers
    Shader* m_standardShader = nullptr;
    GLuint m_VAO, m_VBO, m_EBO;

    // Rendering mode: 0 = Points, 1 = Wireframe, 2 = Solid, 3 = Raytracing
    int m_renderMode;

    // AxisRenderer instance
    AxisRenderer m_axisRenderer;

    // OBJ loading (for fallback or initial object)
    ObjLoader m_objLoader;

    // Transformation matrices
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

    // Camera angles
    float m_yaw, m_pitch;
    // Mouse input states (m_firstMouse, m_lastX, and m_lastY now reside in UserInput)
    bool m_leftMousePressed, m_rightMousePressed;
    int m_selectedObjectIndex;

    // Container for scene objects
    std::vector<SceneObject> m_sceneObjects;

    // Pointer to the input manager
    UserInput* m_userInput;

    // Model center for pivot rotations
    glm::vec3 m_modelCenter;

    // Texture for the cow model
    Texture m_cowTexture;

    // Lighting
    Light m_lights;

    // Shading Mode (e.g., 0 = Flat, 1 = Gouraud, 2 = Phong)
    int m_shadingMode = 2;

    // Screen quad data for raytracing
    GLuint quadVAO, quadVBO, quadEBO;
    GLuint m_screenShaderProgram;
    GLuint m_raytracedTexture = 0;

    // Grid shader
    Grid m_grid;
    Shader* m_gridShader = nullptr; 

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

    void createScreenQuad();

    // Add a new object to the scene; texturePath and scale are optional.
    void addObject(const std::string& modelPath,
        const glm::vec3& initialPosition,
        const std::string& texturePath = "",
        const glm::vec3& scale = glm::vec3(1.0f),
        bool isStatic = false,
        const glm::vec3& color = glm::vec3(1.0f),
        std::string name = "");

    // GLFW callbacks (static functions)
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Helper to retrieve a pointer to this class from a GLFW window
    static Application* getApplication(GLFWwindow* window);
};
