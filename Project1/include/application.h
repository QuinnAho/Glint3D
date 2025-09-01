#pragma once
#include <string>
#include <vector>
#include <memory>
#include <future>                   
#include <unordered_map>
#include <utility>
#include "gl_platform.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ray.h"
#include "material.h"
#include "objloader.h"
#include "axisrenderer.h"
#include "Texture.h"
#include "light.h"
#include "raytracer.h"
#include "userinput.h"
#include "shader.h"
#include "grid.h"
#include "nl_executor.h"
#include "ai_bridge.h"
#include "gizmo.h"

struct SceneObject
{
    std::string name;
    GLuint   VAO = 0, VBO_positions = 0, VBO_normals = 0, VBO_uvs = 0, VBO_tangents = 0, EBO = 0;
    glm::mat4 modelMatrix{ 1.0f };

    ObjLoader objLoader;
    Texture* texture = nullptr;       // legacy diffuse
    Texture* baseColorTex = nullptr;  // PBR
    Texture* normalTex = nullptr;     // PBR
    Texture* mrTex = nullptr;         // PBR (metallic-roughness)
    Shader* shader = nullptr;

    bool      isStatic = false;
    glm::vec3 color{ 1.0f };
    Material  material;
    // Basic PBR factors
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};

class Application
{
public:
    Application();
    ~Application();

    bool  init(const std::string& title, int w, int h, bool headless = false);
    void  run();
    void  frame(); // single frame step (for Emscripten main loop)

    /* getters used by UserInput */
    float       getMouseSensitivity() const { return m_sensitivity; }
    int         getWindowWidth()      const { return m_windowWidth; }
    int         getWindowHeight()     const { return m_windowHeight; }
    glm::mat4   getProjectionMatrix() const { return m_projectionMatrix; }
    glm::mat4   getViewMatrix()       const { return m_viewMatrix; }
    glm::vec3   getCameraPosition()   const { return m_cameraPos; }
    glm::vec3   getCameraFront()      const { return m_cameraFront; }
    glm::vec3   getCameraUp()         const { return m_cameraUp; }
    float       getYaw()              const { return m_yaw; }
    float       getPitch()            const { return m_pitch; }

    // Scene/runtime API for NL executor
    bool        loadObjAt(const std::string& name,
                          const std::string& path,
                          const glm::vec3& position,
                          const glm::vec3& scale = glm::vec3(1.0f));
    bool        loadObjInFrontOfCamera(const std::string& name,
                                       const std::string& path,
                                       float metersForward = 2.0f,
                                       const glm::vec3& scale = glm::vec3(1.0f));
    bool        addPointLightAt(const glm::vec3& position,
                                const glm::vec3& color = glm::vec3(1.0f),
                                float intensity = 1.0f);
    bool        createMaterialNamed(const std::string& name, const Material& m);
    bool        assignMaterialToObject(const std::string& objectName, const std::string& materialName);
    std::string sceneToJson() const; // camera, lights, objects, materials snapshot
    void        toggleFullscreen();   // public so NL executor can call
    // Headless helpers / CLI
    bool        renderToPNG(const std::string& path, int width, int height);
    bool        applyJsonOpsV1(const std::string& json, std::string& error);
    std::string buildShareLink() const; // encodes ops history into URL with ?state=
    // Denoiser
    bool        denoise(std::vector<glm::vec3>& color,
                        const std::vector<glm::vec3>* normal = nullptr,
                        const std::vector<glm::vec3>* albedo = nullptr);
    // Alias to match requested API naming
    bool        Denoise(std::vector<glm::vec3>& color,
                        const std::vector<glm::vec3>* normal = nullptr,
                        const std::vector<glm::vec3>* albedo = nullptr) { return denoise(color, normal, albedo); }
    void        setDenoiseEnabled(bool v) { m_denoise = v; }
    bool        isDenoiseEnabled() const { return m_denoise; }
    bool        duplicateObject(const std::string& sourceName, const std::string& newName,
                                const glm::vec3* deltaPos = nullptr,
                                const glm::vec3* deltaScale = nullptr,
                                const glm::vec3* deltaRotDeg = nullptr);
    void        setCameraTarget(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
    void        setCameraFrontUp(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up);
    void        setCameraLens(float fovDeg, float nearZ, float farZ);
    std::string getSelectedObjectName() const;
    bool        moveObjectByName(const std::string& name, const glm::vec3& delta);
    bool        removeObjectByName(const std::string& name);
    // Light selection
    int         getSelectedLightIndex() const { return m_selectedLightIndex; }
    void        setSelectedLightIndex(int i) { m_selectedLightIndex = i; if (i >= 0) m_selectedObjectIndex = -1; }
    bool        removeLightAtIndex(int i);
    int         getLightCount() const;
    glm::vec3   getLightPosition(int i) const;
    glm::vec3   getSelectedObjectCenterWorld() const;
    // Gizmo helpers
    GizmoMode   getGizmoMode() const { return m_gizmoMode; }
    void        setGizmoMode(GizmoMode m) { m_gizmoMode = m; }
    GizmoAxis   getGizmoAxis() const { return m_gizmoAxis; }
    void        setGizmoAxis(GizmoAxis a) { m_gizmoAxis = a; }
    Gizmo&      getGizmo() { return m_gizmo; }
    bool        isGizmoLocalSpace() const { return m_gizmoLocalSpace; }
    void        toggleGizmoLocalSpace() { m_gizmoLocalSpace = !m_gizmoLocalSpace; }
    bool        isSnapEnabled() const { return m_snapEnabled; }
    void        toggleSnap() { m_snapEnabled = !m_snapEnabled; }
    float       snapTranslateStep() const { return m_snapTranslate; }
    float       snapRotateStepDeg() const { return m_snapRotateDeg; }
    float       snapScaleStep() const { return m_snapScale; }

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
    void renderChatPanelNL();
    void renderChatPanel(); // wrapper calling NL version (keeps old callsites valid)
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
    static void emscriptenFrame(void* arg);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

private:
    // Diagnostics helpers (Explain-my-render)
    void recomputeAngleWeightedNormalsForObject(int index);
    void refreshNormalBuffer(SceneObject& obj);
    void refreshIndexBuffer(SceneObject& obj);
    bool objectMostlyBackfacing(const SceneObject& obj) const;

    // Perf coach helpers
    struct PerfStats {
        int drawCalls = 0;
        size_t totalTriangles = 0;
        int uniqueMaterialKeys = 0;
        size_t uniqueTextures = 0;
        double texturesMB = 0.0;
        double geometryMB = 0.0;
        double vramMB = 0.0; // textures + geometry (approx)
        std::string topSharedKey;
        int topSharedCount = 0;
    };
    void computePerfStats(PerfStats& out) const;
    static std::string materialKeyFor(const SceneObject& obj, const Shader* pbrShader);

    GLFWwindow* m_window = nullptr;
    int   m_windowWidth = 800, m_windowHeight = 600;
    bool  m_headless = false;

    std::vector<SceneObject> m_sceneObjects;
    int   m_selectedObjectIndex = -1;
    int   m_selectedLightIndex = -1;

    glm::vec3 m_cameraPos{ 0,0,10 }, m_cameraFront{ 0,0,-1 }, m_cameraUp{ 0,1,0 };
    float m_fov = 45.f, m_nearClip = 0.1f, m_farClip = 100.f;
    float m_cameraSpeed = 0.5f, m_sensitivity = 0.1f;
    float m_yaw = -90.f, m_pitch = 0.f;

    glm::mat4 m_modelMatrix{ 1 }, m_viewMatrix{ 1 }, m_projectionMatrix{ 1 };

    ObjLoader m_objLoader;
    Shader* m_standardShader = nullptr, * m_gridShader = nullptr, * m_rayScreenShader = nullptr, * m_outlineShader = nullptr, * m_pbrShader = nullptr;
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

    // Named materials repository
    std::unordered_map<std::string, Material> m_namedMaterials;

    // Chat UI state + NL command plumbing
    char m_chatInput[2048] = {0};
    std::vector<std::string> m_chatScrollback; // parsed outputs and errors
    bool m_previewOnly = false; // preview no-op for NL text (kept for UI parity)
    bool m_useAI = true;      // default ON: AI planner enabled by default
    nl::Executor m_nl{ *this };
    AIConfig m_aiConfig{};
    AIPlanner m_ai{ m_aiConfig };
    // Async AI planning
    std::future<std::pair<std::string, std::string>> m_aiFuture; // (plan, error)
    bool m_aiBusy = false;
    bool m_aiSuggestOpenDiag = false; // pending suggestion to open diagnostics
    bool m_bootMessageShown = false;   // log welcome/instructions once

    // Fullscreen toggle state
    bool m_fullscreen = false;
    bool m_f11Held = false;
    int  m_windowPosX = 100, m_windowPosY = 100;
    int  m_windowedWidth = 800, m_windowedHeight = 600;

    // Gizmo
    Gizmo m_gizmo;
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoAxis m_gizmoAxis = GizmoAxis::None; // highlighted/active axis
    bool m_gizmoLocalSpace = true; // local (true) vs world (false)
    // Snapping
    bool  m_snapEnabled = false;
    float m_snapTranslate = 0.5f;
    float m_snapRotateDeg = 15.0f;
    float m_snapScale = 0.1f;
    // key edge toggles
    bool m_lHeld = false; // local/world toggle key
    bool m_nHeld = false; // snap toggle key
    bool m_deleteHeld = false; // delete key edge

    // Share-state: store JSON Ops v1 history (raw JSON snippets)
    std::vector<std::string> m_opsHistory;

    // UI toggles
    bool m_showSettingsPanel = true; // show Settings initially
    bool m_showDiagnosticsPanel = false; // legacy flag; diagnostics now in Settings panel
    bool m_showPerfHUD = true;           // Perf HUD with counters

    // Denoise toggle
    bool m_denoise = false;

    // sRGB framebuffer toggle (runtime)
    bool m_framebufferSRGBEnabled = true;
};
