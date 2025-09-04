#include "application_core.h"
#include "gl_platform.h"
#include <GLFW/glfw3.h>
#include "scene_manager.h"
#include "render_system.h" 
#include "camera_controller.h"
#include "light.h"
#include "ui_bridge.h"
#include "imgui_ui_layer.h"
#include <iostream>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

ApplicationCore::ApplicationCore()
{
    // Create core systems
    m_scene = std::make_unique<SceneManager>();
    m_renderer = std::make_unique<RenderSystem>();
    m_camera = std::make_unique<CameraController>();
    m_lights = std::make_unique<Light>();
    
    // Create UI bridge
    m_uiBridge = std::make_unique<UIBridge>(*m_scene, *m_renderer, *m_camera, *m_lights);
}

ApplicationCore::~ApplicationCore()
{
    shutdown();
}

bool ApplicationCore::init(const std::string& windowTitle, int width, int height, bool headless)
{
    m_windowWidth = width;
    m_windowHeight = height;
    m_headless = headless;
    
    // Initialize GLFW and create window
    if (!initGLFW(windowTitle, width, height)) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // Initialize OpenGL function loading
    if (!initGLAD()) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    
    // Initialize core systems
    if (!m_renderer->init(width, height)) {
        std::cerr << "Failed to initialize render system\n";
        return false;
    }
    
    // Set up callbacks
    initCallbacks();
    
    // Initialize UI (skip for headless or web HTML UI)
#ifndef WEB_USE_HTML_UI
    if (!m_headless) {
        // Create ImGui UI layer
        auto imguiLayer = std::make_unique<ImGuiUILayer>();
        m_uiBridge->setUILayer(std::move(imguiLayer));
        
        if (!m_uiBridge->initUI(width, height)) {
            std::cerr << "Failed to initialize UI layer\n";
            return false;
        }
    }
#endif
    
    // Create default scene content
    createDefaultScene();
    
    return true;
}

void ApplicationCore::run()
{
    while (!glfwWindowShouldClose(m_window)) {
        frame();
    }
}

void ApplicationCore::frame()
{
    glfwPollEvents();
    
    // Update camera
    m_camera->update(0.016f); // Assume ~60fps for now
    
    // Update render system camera
    m_renderer->setCamera(m_camera->getCameraState());
    m_renderer->updateViewMatrix();
    m_renderer->updateProjectionMatrix(m_windowWidth, m_windowHeight);
    
    // Clear and render scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_renderer->render(*m_scene, *m_lights);
    
    // Render UI
    if (!m_headless) {
        m_uiBridge->renderUI();
    }
    
    glfwSwapBuffers(m_window);
}

void ApplicationCore::shutdown()
{
    if (m_uiBridge) {
        m_uiBridge->shutdownUI();
    }
    
    if (m_renderer) {
        m_renderer->shutdown();
    }
    
    cleanupGL();
    
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    
    glfwTerminate();
}

bool ApplicationCore::loadObject(const std::string& name, const std::string& path,
                                const glm::vec3& position, const glm::vec3& scale)
{
    return m_scene->loadObject(name, path, position, scale);
}

bool ApplicationCore::renderToPNG(const std::string& path, int width, int height)
{
    return m_renderer->renderToPNG(*m_scene, *m_lights, path, width, height);
}

bool ApplicationCore::applyJsonOpsV1(const std::string& json, std::string& error)
{
    return m_uiBridge->applyJsonOps(json, error);
}

std::string ApplicationCore::buildShareLink() const
{
    return m_uiBridge->buildShareLink();
}

std::string ApplicationCore::sceneToJson() const
{
    return m_uiBridge->sceneToJson();
}

void ApplicationCore::setDenoiseEnabled(bool enabled) 
{
    m_renderer->setDenoiseEnabled(enabled);
}

bool ApplicationCore::isDenoiseEnabled() const 
{
    return m_renderer->isDenoiseEnabled();
}

void ApplicationCore::handleMouseMove(double xpos, double ypos)
{
    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
    }
    
    double deltaX = xpos - m_lastMouseX;
    double deltaY = m_lastMouseY - ypos; // Reversed for correct direction
    
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;
    
    // Only handle camera movement when right mouse is pressed
    if (m_rightMousePressed) {
        m_camera->rotate(deltaX * m_camera->getSensitivity(), deltaY * m_camera->getSensitivity());
    }
}

void ApplicationCore::handleMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_leftMousePressed = (action == GLFW_PRESS);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        m_rightMousePressed = (action == GLFW_PRESS);
        
        if (action == GLFW_PRESS) {
            // Disable cursor for camera movement
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            // Re-enable cursor
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void ApplicationCore::handleFramebufferResize(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, width, height);
    
    m_renderer->updateProjectionMatrix(width, height);
    
    if (m_uiBridge) {
        m_uiBridge->handleResize(width, height);
    }
}

void ApplicationCore::handleKey(int key, int scancode, int action, int mods)
{
    if (!m_rightMousePressed) return; // Only handle movement when camera is active
    
    const float speed = m_camera->getSpeed();
    
    // WASD movement
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveForward(speed);
    }
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveBackward(speed);
    }
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveLeft(speed);
    }
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveRight(speed);
    }
    
    // Vertical movement
    if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveUp(speed);
    }
    if (key == GLFW_KEY_LEFT_CONTROL && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        m_camera->moveDown(speed);
    }
    
    // Quick gizmo mode switching
    if (mods & GLFW_MOD_SHIFT) {
        if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
            // Set translate mode via UI command
            UICommandData cmd;
            cmd.command = UICommand::SetGizmoMode;
            cmd.intParam = (int)GizmoMode::Translate;
            m_uiBridge->handleUICommand(cmd);
        }
        else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            UICommandData cmd;
            cmd.command = UICommand::SetGizmoMode;
            cmd.intParam = (int)GizmoMode::Rotate;
            m_uiBridge->handleUICommand(cmd);
        }
        else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
            UICommandData cmd;
            cmd.command = UICommand::SetGizmoMode;
            cmd.intParam = (int)GizmoMode::Scale;
            m_uiBridge->handleUICommand(cmd);
        }
    }
}

bool ApplicationCore::initGLFW(const std::string& windowTitle, int width, int height)
{
    if (!glfwInit()) {
        return false;
    }
    
    // Set OpenGL context version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Enable MSAA and sRGB
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#endif
    
    // Create window
    if (m_headless) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    
    m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    
    return true;
}

bool ApplicationCore::initGLAD()
{
#ifdef __EMSCRIPTEN__
    // WebGL2 doesn't need GLAD
    return true;
#else
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
}

void ApplicationCore::initCallbacks()
{
    // Store this pointer in window user data for callbacks
    glfwSetWindowUserPointer(m_window, this);
    
    // Set callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
}

void ApplicationCore::createDefaultScene()
{
    // Add default lighting
    m_lights->addLight(
        glm::vec3(2.0f, 4.0f, 2.0f),     // position
        glm::vec3(0.8f, 0.8f, 0.7f),     // color
        1.0f                              // intensity
    );
    
    // Set default camera position
    CameraState defaultCam;
    defaultCam.position = glm::vec3(0.0f, 2.0f, 5.0f);
    defaultCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
    defaultCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_camera->setCameraState(defaultCam);
}

void ApplicationCore::cleanupGL()
{
    // OpenGL cleanup is handled by the systems themselves
}

// GLFW callback wrappers
void ApplicationCore::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleMouseMove(xpos, ypos);
    }
}

void ApplicationCore::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleMouseButton(button, action, mods);
    }
}

void ApplicationCore::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleFramebufferResize(width, height);
    }
}

void ApplicationCore::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleKey(key, scancode, action, mods);
    }
}