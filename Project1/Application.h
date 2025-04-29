#pragma once
#include <string>
#include <vector>
#include <memory>
#include <future>                   
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Ray.h"
#include "Material.h"
#include "ObjLoader.h"
#include "AxisRenderer.h"
#include "Texture.h"
#include "Light.h"
#include "Raytracer.h"
#include "UserInput.h"
#include "Shader.h"
#include "Grid.h"

struct SceneObject
{
    std::string name;
    GLuint   VAO = 0, VBO_positions = 0, VBO_normals = 0, EBO = 0;
    glm::mat4 modelMatrix{ 1.0f };

    ObjLoader objLoader;
    Texture* texture = nullptr;
    Shader* shader = nullptr;

    bool      isStatic = false;
    glm::vec3 color{ 1.0f };
    Material  material;
};

class Application
{
public:
    Application();
    ~Application();

    bool  init(const std::string& title, int w, int h);
    void  run();

    /* getters used by UserInput */
    float       getMouseSensitivity() const { return m_sensitivity; }
    int         getWindowWidth()      const { return m_windowWidth; }
    int         getWindowHeight()     const { return m_windowHeight; }
    glm::mat4   getProjectionMatrix() const { return m_projectionMatrix; }
    glm::mat4   getViewMatrix()       const { return m_viewMatrix; }
    glm::vec3   getCameraPosition()   const { return m_cameraPos; }
    float       getYaw()              const { return m_yaw; }
    float       getPitch()            const { return m_pitch; }

    bool  isLeftMousePressed()  const { return m_leftMousePressed; }
    bool  isRightMousePressed() const { return m_rightMousePressed; }
    int   getSelectedObjectIndex() const { return m_selectedObjectIndex; }

    const std::vector<SceneObject>& getSceneObjects() const { return m_sceneObjects; }
    std::vector<SceneObject>& getSceneObjects() { return m_sceneObjects; }

    /* setters called by UserInput */
    void setLeftMousePressed(bool v) { m_leftMousePressed = v; }
    void setRightMousePressed(bool v) { m_rightMousePressed = v; }
    void setSelectedObjectIndex(int i) { m_selectedObjectIndex = i; }
    void setCameraAngles(float yaw, float pitch);

    /* helpers */
    bool rayIntersectsAABB(const Ray& ray,
        const glm::vec3& mn,
        const glm::vec3& mx,
        float& t);

    UserInput* getUserInput() { return m_userInput; }

private:
    bool initGLFW(const std::string&, int, int);
    bool initGLAD();
    void initImGui();
    void setupOpenGL();
    void cleanup();

    void processInput();
    void renderScene();                  
    void renderGUI();
    void renderAxisIndicator();
    void createScreenQuad();

    void addObject(std::string name = "",
        const std::string& model = "",
        const glm::vec3& pos = glm::vec3(1),
        const std::string& texture = "",
        const glm::vec3& scale = glm::vec3(1),
        bool  isStat = false,
        const glm::vec3& color = glm::vec3(1));

    static void mouseCallback(GLFWwindow*, double, double);
    static void mouseButtonCallback(GLFWwindow*, int, int, int);
    static Application* getApplication(GLFWwindow*);

private:
    GLFWwindow* m_window = nullptr;
    int   m_windowWidth = 800, m_windowHeight = 600;

    std::vector<SceneObject> m_sceneObjects;
    int   m_selectedObjectIndex = -1;

    glm::vec3 m_cameraPos{ 0,0,10 }, m_cameraFront{ 0,0,-1 }, m_cameraUp{ 0,1,0 };
    float m_fov = 45.f, m_nearClip = 0.1f, m_farClip = 100.f;
    float m_cameraSpeed = 0.5f, m_sensitivity = 0.1f;
    float m_yaw = -90.f, m_pitch = 0.f;

    glm::mat4 m_modelMatrix{ 1 }, m_viewMatrix{ 1 }, m_projectionMatrix{ 1 };

    ObjLoader m_objLoader;
    Shader* m_standardShader = nullptr, * m_gridShader = nullptr, * m_rayScreenShader = nullptr;
    GLuint  m_VAO = 0, m_VBO = 0, m_EBO = 0;
    Grid    m_grid;
    AxisRenderer m_axisRenderer;
    Light   m_lights;
    int     m_shadingMode = 2;

    std::unique_ptr<Raytracer> m_raytracer;
    GLuint rayTexID = 0, quadVAO = 0, quadVBO = 0, quadEBO = 0;

    std::future<void>         m_traceJob;   // background task
    std::vector<glm::vec3>    m_framebuffer;
    bool                      m_traceDone = false;

    bool  m_leftMousePressed = false, m_rightMousePressed = false;
    UserInput* m_userInput = nullptr;
    int   m_renderMode = 2;   
    glm::vec3 m_modelCenter{ 0 };
    Texture   m_cowTexture;

    GLuint m_shadowFBO;
    GLuint m_shadowDepthTexture;
    Shader* m_shadowShader;

    glm::mat4 m_lightSpaceMatrix;

};
