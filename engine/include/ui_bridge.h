// Machine Summary Block (ndjson)
// {"file":"engine/include/ui_bridge.h","purpose":"Abstracts UI rendering and command routing for Glint3D's editor","exports":["UIState","UICommand","UICommandData","IUILayer","UIBridge"],"depends_on":["SceneManager","RenderSystem","CameraController","Light","JsonOpsExecutor"],"notes":["Builds engine state snapshots consumable by UI layers","Translates UI commands into scene and renderer actions","Persists MRU file lists and bridges JSON ops"]}
#pragma once

/**
 * @file ui_bridge.h
 * @brief UI abstraction that builds engine state snapshots and executes UI-driven commands.
 *
 * UIBridge mediates between core systems and whichever immediate-mode or platform UI is active,
 * producing a serializable UIState for rendering while translating user commands back into
 * scene, renderer, and JSON-ops mutations.
 */

#include <string>
#include <vector>
#include <functional>
#include "gizmo.h"
#include "render_system.h"
#include "camera_controller.h"

// forward declarations
class SceneManager;
class RenderSystem; 
class Light;

// include needed for unique_ptr
#include "json_ops.h"

// ui-independent state snapshot for any ui implementation
struct UIState {
    // visibility toggles
    bool showSettingsPanel = true;
    bool showPerfHUD = false;
    bool showGrid = true;
    bool showAxes = true;
    bool showSkybox = false;
    
    // rendering state
    RenderMode renderMode = RenderMode::Solid;
    ShadingMode shadingMode = ShadingMode::Gouraud;
    bool framebufferSRGBEnabled = true;
    bool denoiseEnabled = false;
    int msaaSamples = 1; // ui exposure of msaa samples
    
    // environment / lighting state
    RenderSystem::BackgroundMode backgroundMode = RenderSystem::BackgroundMode::Solid;
    glm::vec3 backgroundSolid{0.10f,0.11f,0.12f};
    glm::vec3 backgroundTop{0.10f,0.11f,0.12f};
    glm::vec3 backgroundBottom{0.10f,0.11f,0.12f};
    std::string backgroundHDRPath;
    float skyboxIntensity = 1.0f;
    float iblIntensity = 1.0f;
    std::string environmentPath;
    
    // camera state
    CameraState camera;
    float cameraSpeed = 0.5f;
    float sensitivity = 0.1f;
    bool requireRMBToMove = true;
    
    // selection
    int selectedObjectIndex = -1;
    std::string selectedObjectName;
    int selectedLightIndex = -1;
    int objectCount = 0;
    int lightCount = 0;
    
    // objects list for hierarchy ui
    std::vector<std::string> objectNames;
    // optional: parent indices for tree ui (-1 = root). if empty or size mismatch, ui renders flat.
    std::vector<int> objectParentIndex;
    
    // light details for ui editing
    struct LightUI {
        int type = 0;              // 0=point, 1=directional
        glm::vec3 position{0.0f};  // point
        glm::vec3 direction{0.0f, -1.0f, 0.0f}; // directional
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        bool enabled = true;
        float innerConeDeg = 15.0f;
        float outerConeDeg = 25.0f;
    };
    std::vector<LightUI> lights;   // mirrors lights for editing
    
    // gizmo state
    GizmoMode gizmoMode = GizmoMode::Translate;
    GizmoAxis gizmoAxis = GizmoAxis::None;
    bool gizmoLocalSpace = true;
    bool snapEnabled = false;
    float snapTranslate = 0.5f;
    float snapRotateDeg = 15.0f;
    float snapScale = 0.1f;
    
    // statistics
    RenderStats renderStats;
    
    // console / log
    std::vector<std::string> consoleLog;

    // recent files (mru)
    std::vector<std::string> recentFiles;

};

// command interface for ui actions
enum class UICommand {
    LoadObject,
    RemoveObject,
    DuplicateObject,
    RenameObject,
    SelectObject,
    SetRenderMode,
    SetShadingMode,
    ToggleFramebufferSRGB,
    SetCameraSpeed,
    SetMouseSensitivity,
    AddLight,
    AddPointLight,
    AddDirectionalLight,
    AddSpotLight,
    RemoveLight,
    SelectLight,
    DeleteLight,
    SetLightEnabled,
    SetLightIntensity,
    SetLightDirection,
    SetLightPosition,
    SetLightInnerCone,
    SetLightOuterCone,
    SetGizmoMode,
    ToggleGizmoSpace,
    ToggleSnap,
    ExecuteConsoleCommand,
    ApplyJsonOps,
    RenderToPNG,
    SetMSAASamples,
    SetRequireRMBToMove,
    
    // ui visibility toggles
    ToggleSettingsPanel,
    TogglePerfHUD,
    ToggleGrid,
    ToggleAxes,
    ToggleSkybox,
    
    // ibl / environment controls
    LoadHDREnvironment,
    SetSkyboxIntensity,
    SetIBLIntensity,
    
    // scene operations
    CenterCamera,
    SetCameraPreset,
    ResetScene,
    
    // application control
    CopyShareLink,
    ExitApplication,
    
    // file operations
    ImportAsset,          // unified import for models and JSON scenes
    ExportScene,
    OpenFile
    ,
    // hierarchy operations
    ReparentObject
};

struct UICommandData {
    UICommand command;
    std::string stringParam;
    float floatParam = 0.0f;
    int intParam = 0;
    int intParam2 = 0;
    glm::vec3 vec3Param{0.0f};
    bool boolParam = false;
};

// abstract ui layer interface
class IUILayer {
public:
    virtual ~IUILayer() = default;
    
    virtual bool init(int windowWidth, int windowHeight) = 0;
    virtual void shutdown() = 0;
    virtual void render(const UIState& state) = 0;
    virtual void handleResize(int width, int height) = 0;
    virtual void handleCommand(const UICommandData& cmd) = 0;
    
    // command callback - ui implementations call this to execute actions
    std::function<void(const UICommandData&)> onCommand;
};

// Bridge class that coordinates between core systems and UI
class UIBridge {
public:
    UIBridge(SceneManager& scene, RenderSystem& renderer, 
             CameraController& camera, Light& lights);
    ~UIBridge() = default;
    
    // ui lifecycle
    void setUILayer(std::unique_ptr<IUILayer> ui) { m_ui = std::move(ui); }
    bool initUI(int windowWidth, int windowHeight);
    void shutdownUI();
    void renderUI();
    void handleResize(int width, int height);
    
    // state management
    UIState buildUIState() const;
    void handleUICommand(const UICommandData& command);
    
    // console / logging
    void addConsoleMessage(const std::string& message);
    void clearConsoleLog();

    // light selection for ui state
    void setSelectedLightIndex(int idx) { m_selectedLightIndex = idx; }
    int  getSelectedLightIndex() const { return m_selectedLightIndex; }

    // settings queried by core
    bool getRequireRMBToMove() const { return m_requireRMBToMove; }
    
    // json ops integration
    bool applyJsonOps(const std::string& json, std::string& error);
    std::string buildShareLink() const;
    std::string sceneToJson() const;
    
private:
    // system references
    SceneManager& m_scene;
    RenderSystem& m_renderer;
    CameraController& m_camera;
    Light& m_lights;
    
    // ui layer
    std::unique_ptr<IUILayer> m_ui;
    
    // state
    std::vector<std::string> m_consoleLog;
    bool m_previewOnly = false;
    bool m_requireRMBToMove = true;
    int  m_selectedLightIndex = -1;

    // json ops executor (modularized from ui)
    std::unique_ptr<JsonOpsExecutor> m_ops;
    
    // mru recent files
    std::vector<std::string> m_recentFiles;
    size_t m_recentMax = 10;
    void loadRecentFiles();
    void saveRecentFiles() const;
    void addRecentFile(const std::string& path);
    bool openFilePath(const std::string& path);
    
    // command handlers
    void handleLoadObject(const UICommandData& cmd);
    void handleRenderMode(const UICommandData& cmd);
    void handleCameraSettings(const UICommandData& cmd);
    void handleGizmoSettings(const UICommandData& cmd);
    void handleConsoleCommand(const UICommandData& cmd);
};
