#include "app_state.h"
#include "application.h"

static std::string safeSelectedName(const Application& app) {
    auto idx = app.getSelectedObjectIndex();
    if (idx >= 0 && idx < (int)app.getSceneObjects().size())
        return app.getSceneObjects()[(size_t)idx].name;
    return std::string();
}

void BuildUiStateFromApp(Application& app) {
    // Fill the private snapshot struct living in Application
    // (This function assumes Application declared: AppStateView uiState_;)
    AppStateView v{};
    // Visibility / toggles
    v.showSettingsPanel       = true;            // you can adapt if you track another flag
    v.showPerfHUD             = true;            // adapt if needed
    v.framebufferSRGBEnabled  = true;            // adapt if needed
    v.headless                = false;           // adapt if needed
    v.useAI                   = true;            // adapt if needed
    v.aiBusy                  = false;           // adapt if you wire async AI
    v.denoise                 = app.isDenoiseEnabled();

    // Rendering
    // Application doesn't expose render/shading setters publicly,
    // but ui snapshot is allowed to read via friend or internal wiring.
    // If you want exact values, expose getters in Application and use them here.
    // Below we use public knowledge of defaults.
    v.renderMode  = 2;
    v.shadingMode = 1;

    // Camera
    v.camPos   = app.getCameraPosition();
    v.camFront = app.getCameraFront();
    v.camUp    = app.getCameraUp();
    v.fov      = app.getProjectionMatrix()[1][1] != 0.0f ?  app.getYaw() /* placeholder */ : 45.f; // not strictly needed
    v.fov      = 45.f; // use your stored value if you add a getter
    v.nearZ    = 0.1f;
    v.farZ     = 100.f;

    // Selection / counts
    v.selectedObjectIndex = app.getSelectedObjectIndex();
    v.selectedObjectName  = safeSelectedName(app);
    v.selectedLightIndex  = app.getSelectedLightIndex();
    v.objectCount         = (int)app.getSceneObjects().size();
    v.lightCount          = app.getLightCount();

    // Gizmo
    v.gizmoMode  = app.getGizmoMode();
    v.gizmoAxis  = app.getGizmoAxis();
    v.gizmoLocal = app.isGizmoLocalSpace();
    v.snap       = app.isSnapEnabled();
    v.snapT      = app.snapTranslateStep();
    v.snapRDeg   = app.snapRotateStepDeg();
    v.snapS      = app.snapScaleStep();

    // Console snapshot: keep the last ~200 lines if you want. We’ll leave empty here,
    // since Application already owns and renders the scrollback directly.
    // If you’d like to expose it: add a public getter for m_chatScrollback.

    // Write back into Application’s snapshot (friend or method).
    // Provide a small helper method in Application to assign:
    //   uiState_ = v;
    // Because we can't touch private fields here directly, we call a helper.
    // If you prefer, just inline this function into Application and remove this file.
    // For now we emulate the helper by a function on Application:
    extern void AssignUiStateSnapshot(Application& app, const AppStateView& v);
    AssignUiStateSnapshot(app, v);
}
