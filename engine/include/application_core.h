// Machine Summary Block (ndjson)
// {"file":"engine/include/application_core.h","purpose":"Coordinates Glint3D's window lifecycle, scene state, rendering, and user interaction","exports":["ApplicationCore"],"depends_on":["SceneManager","RenderSystem","CameraController","UIBridge","JsonOpsExecutor"],"notes":["Bootstraps GLFW windowing and callbacks","Bridges CLI/JSON operations to renderer","Owns deterministic clock and seeded RNG for reproducibility"]}
#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "gizmo.h"
#include "render_settings.h"
#include "clock.h"
#include "seeded_rng.h"

/**
 * @file application_core.h
 * @brief Central application driver that ties together windowing, scene management, and rendering.
 *
 * ApplicationCore bootstraps platform dependencies, advances the main loop, and exposes a compact
 * API for loading assets, dispatching renders, and routing input/state updates to the renderer,
 * camera controller, and UI bridge layers.
 */

// forward declarations
struct GLFWwindow;
class SceneManager;
class RenderSystem;
class CameraController;
class Light;
class UIBridge;
class user input;
class JsonOpsExecutor;

// streamlined application class focused on coordination
class ApplicationCore 
{
public:
    ApplicationCore(std::unique_ptr<IClock> clock = nullptr);
    ~ApplicationCore();

    bool init(const std::string& windowTitle, int width, int height, bool headless = false);
    void run();
    void frame(); // single frame for emscripten
    void shutdown();

    // public api for external usage (main.cpp, json ops, etc.)
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    bool renderToPNG(const std::string& path, int width, int height);
    bool applyJsonOpsV1(const std::string& json, std::string& error);
    std::string buildShareLink() const;
    std::string sceneToJson() const;
    
    // denoiser support
    void setDenoiseEnabled(bool enabled);
    bool isDenoiseEnabled() const;
    
    // raytracing mode support
    void setRaytraceMode(bool enabled);
    bool isRaytraceMode() const;
    
    // reflection samples per pixel support
    void setReflectionSpp(int spp);
    int getReflectionSpp() const;
    
    // schema validation support
    void setStrictSchema(bool enabled, const std::string& version = "v1.3");
    bool isStrictSchemaEnabled() const;
    
    // render settings support
    void setRenderSettings(const RenderSettings& settings);
    const RenderSettings& getRenderSettings() const;
    
    // input callbacks (called by glfw)
    void handleMouseMove(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);
    void handleFramebufferResize(int width, int height);
    void handleKey(int key, int scancode, int action, int mods);
    void handleFileDrop(int count, const char** paths);

    // getters for external access (user input, etc.)
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    GLFWwindow* getWindow() const { return m_window; }
    
    CameraController& getCameraController() { return *m_camera; }
    const CameraController& getCameraController() const { return *m_camera; }
    
    SceneManager& getSceneManager() { return *m_scene; }
    const SceneManager& getSceneManager() const { return *m_scene; }

private:
    // core systems
    std::unique_ptr<SceneManager> m_scene;
    std::unique_ptr<RenderSystem> m_renderer;
    std::unique_ptr<CameraController> m_camera; 
    std::unique_ptr<Light> m_lights;
    std::unique_ptr<UIBridge> m_uiBridge;
    std::unique_ptr<JsonOpsExecutor> m_ops;
    
    // platform / window management
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    bool m_headless = false;
    
    // input state
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;
    double m_lastFrameTime = 0.0;
    bool   m_requireRMBToMove = true;

    // selection
    int m_selectedLightIndex = -1;
    
    // render settings
    RenderSettings m_renderSettings;

    std::unique_ptr<IClock> m_clock;
    SeededRng m_rng;

    // gizmo state
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoAxis m_gizmoAxis = GizmoAxis::None;
    bool m_gizmoLocal = true;
    bool m_gizmoDragging = false;
    glm::vec3 m_dragOriginWorld{0.0f};
    glm::vec3 m_dragAxisDir{0.0f};
    float m_axisStartS = 0.0f;
    int   m_dragObjectIndex = -1;
    int   m_dragLightIndex = -1;
    glm::mat4 m_modelStart{1.0f};
    
    // initialization
    bool initGLFW(const std::string& windowTitle, int width, int height);
    bool initGLAD();
    void initCallbacks();
    void createDefaultScene();
    void setWindowIcon();
    
    // glfw callback wrappers
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    
    // cleanup
    void cleanupGL();
};
