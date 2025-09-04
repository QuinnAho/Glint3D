#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "gizmo.h"   // for GizmoMode / GizmoAxis

// A read-only snapshot of the bits of Application state the UI needs.
struct AppStateView {
    // Visibility / toggles
    bool showSettingsPanel = true;
    bool showPerfHUD = false;
    bool framebufferSRGBEnabled = true;
    bool headless = false;
    bool useAI = true;
    bool aiBusy = false;
    bool denoise = false;

    // Rendering
    int  renderMode  = 2;   // 0=point,1=wire,2=solid,3=raytrace
    int  shadingMode = 1;   // 0=flat, 1=gouraud

    // Camera
    glm::vec3 camPos{0};
    glm::vec3 camFront{0,0,-1};
    glm::vec3 camUp{0,1,0};
    float fov = 45.f, nearZ = 0.1f, farZ = 100.f;

    // Selection / counts
    int   selectedObjectIndex = -1;
    std::string selectedObjectName;
    int   selectedLightIndex  = -1;
    int   objectCount = 0;
    int   lightCount  = 0;

    // Gizmo
    GizmoMode gizmoMode = GizmoMode::Translate;
    GizmoAxis gizmoAxis = GizmoAxis::None;
    bool  gizmoLocal = true;
    bool  snap = false;
    float snapT = 0.5f;
    float snapRDeg = 15.f;
    float snapS = 0.1f;

    // A small scrollback snapshot (last N lines)
    std::vector<std::string> console;
};

class Application; // fwd

// Populates Application::uiState_ from Application's private members.
// Implemented in app_state.cpp
void BuildUiStateFromApp(Application& app);
