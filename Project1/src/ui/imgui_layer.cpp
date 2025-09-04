#include "imgui_layer.h"
#include "application.h"
#include "app_state.h"
#include "app_commands.h"

// Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// GLM helpers used by the UI
#include <glm/gtc/type_ptr.hpp>

// std
#include <sstream>
#include <algorithm>

// ---------- lifecycle ----------

void ImGuiLayer::init(Application& app) {
#ifndef WEB_USE_HTML_UI
    if (app.isHeadless()) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    ImGui_ImplGlfw_InitForOpenGL(app.glfwWindow(), true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    // ---- Theme (copied from your previous initImGui) ----
    ImGuiStyle& style = ImGui::GetStyle();
#ifdef IMGUI_HAS_DOCKING
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 6.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
    style.FrameRounding = 6.0f;
    style.GrabRounding  = 6.0f;
    style.TabRounding   = 6.0f;

    auto& c = style.Colors;
    c[ImGuiCol_WindowBg]        = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    c[ImGuiCol_Header]          = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_HeaderHovered]   = ImVec4(0.28f, 0.32f, 0.36f, 1.00f);
    c[ImGuiCol_HeaderActive]    = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    c[ImGuiCol_TitleBg]         = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
    c[ImGuiCol_Button]          = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_ButtonHovered]   = ImVec4(0.25f, 0.28f, 0.31f, 1.00f);
    c[ImGuiCol_ButtonActive]    = ImVec4(0.22f, 0.25f, 0.28f, 1.00f);
    c[ImGuiCol_Tab]             = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    c[ImGuiCol_TabHovered]      = ImVec4(0.28f, 0.32f, 0.36f, 1.00f);
    c[ImGuiCol_TabActive]       = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_FrameBg]         = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    c[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_FrameBgActive]   = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
#endif
}

void ImGuiLayer::beginFrame() {
#ifndef WEB_USE_HTML_UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#endif
}

static void DrawAllPanels(Application& app); // fwd

void ImGuiLayer::draw(Application& app) {
#ifndef WEB_USE_HTML_UI
    if (app.isHeadless()) return;
    DrawAllPanels(app);
#endif
}

void ImGuiLayer::endFrame() {
#ifndef WEB_USE_HTML_UI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#ifdef IMGUI_HAS_DOCKING
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        auto* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
#endif
#endif
}

void ImGuiLayer::shutdown() {
#ifndef WEB_USE_HTML_UI
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
#endif
}

// ---------- UI (moved from Application::renderGUI) ----------

static void DrawAllPanels(Application& app) {
#ifndef WEB_USE_HTML_UI

#ifdef IMGUI_HAS_DOCKING
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

    auto& ui    = app.uiState();
    auto& logs  = ui.chatScrollback;

    // ==== Main menu bar ====
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Cube")) {
                app.loadObjInFrontOfCamera("Cube", "assets/models/cube.obj", 2.0f, glm::vec3(1.0f));
            }
            if (ImGui::MenuItem("Load Plane")) {
                app.loadObjInFrontOfCamera("Plane", "assets/samples/models/plane.obj", 2.0f, glm::vec3(1.0f));
            }
            if (ImGui::MenuItem("Copy Share Link")) {
                std::string link = app.buildShareLink(); // assumes you have this
                ImGui::SetClipboardText(link.c_str());
                logs.push_back("Share link copied to clipboard.");
            }
            bool toggle = ui.showSettingsPanel;
            if (ImGui::MenuItem("Toggle Settings Panel", nullptr, toggle)) {
                ui.showSettingsPanel = !ui.showSettingsPanel;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            bool selected = (app.m_renderMode == 0); if (ImGui::MenuItem("Point",    nullptr, selected)) app.m_renderMode = 0;
            selected = (app.m_renderMode == 1);      if (ImGui::MenuItem("Wire",     nullptr, selected)) app.m_renderMode = 1;
            selected = (app.m_renderMode == 2);      if (ImGui::MenuItem("Solid",    nullptr, selected)) app.m_renderMode = 2;
            selected = (app.m_renderMode == 3);      if (ImGui::MenuItem("Raytrace", nullptr, selected)) app.m_renderMode = 3;
            ImGui::Separator();
            if (ImGui::MenuItem("Fullscreen")) app.toggleFullscreen();
            bool perfHud = ui.showPerfHUD; if (ImGui::MenuItem("Perf HUD", nullptr, perfHud)) ui.showPerfHUD = !ui.showPerfHUD;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ==== Settings & Diagnostics panel ====
    if (ui.showSettingsPanel) {
        ImGuiIO& io = ImGui::GetIO();
        float consoleH = io.DisplaySize.y * 0.25f;
        float rightW   = 420.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightW - 10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightW, io.DisplaySize.y - consoleH - 20.0f), ImGuiCond_Always);
        if (ImGui::Begin("Settings & Diagnostics")) {

            ImGui::SliderFloat("Camera Speed",       &app.m_cameraSpeed,   0.01f, 1.0f);
            ImGui::SliderFloat("Mouse Sensitivity",  &app.m_sensitivity,   0.01f, 1.0f);
            ImGui::SliderFloat("Field of View",      &app.m_fov,          30.0f, 120.0f);
            ImGui::SliderFloat("Near Clip",          &app.m_nearClip,      0.01f, 5.0f);
            ImGui::SliderFloat("Far Clip",           &app.m_farClip,       5.0f, 500.0f);

            if (ImGui::Button("Point Cloud Mode")) { app.m_renderMode = 0; }
            ImGui::SameLine();
            if (ImGui::Button("Wireframe Mode"))   { app.m_renderMode = 1; }
            ImGui::SameLine();
            if (ImGui::Button("Solid Mode"))       { app.m_renderMode = 2; }
            ImGui::SameLine();
            if (ImGui::Button("Raytrace"))         { app.m_renderMode = 3; }

            if (app.m_renderMode == 3) {
                if (app.m_traceJob.valid())
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Raytracing... please wait");
                else if (app.m_traceDone)
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Raytracer Done!");
            }

            // Diagnostics
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
                int  targetIdx  = app.m_selectedObjectIndex;
                if (targetIdx < 0 && !app.m_sceneObjects.empty()) targetIdx = 0;
                bool hasTarget  = (targetIdx >= 0 && targetIdx < (int)app.m_sceneObjects.size());

                bool anyPBR = false;
                for (auto& o : app.m_sceneObjects) { if (o.shader == app.m_pbrShader) { anyPBR = true; break; } }
                bool noLights = (app.m_lights.m_lights.empty());
                if (!noLights) {
                    bool allZero = true; for (auto& L : app.m_lights.m_lights) { if (L.enabled && L.intensity > 0.0f) { allZero = false; break; } }
                    noLights = allZero;
                }

                bool srgbMismatch = anyPBR && app.m_framebufferSRGBEnabled;

                ImGui::Text("Selected Object: %s", hasTarget ? app.m_sceneObjects[targetIdx].name.c_str() : "<none>");
                if (hasTarget) {
                    auto& obj = app.m_sceneObjects[targetIdx];
                    bool missingNormals = !obj.objLoader.hadNormalsFromSource();
                    bool backface       = app.objectMostlyBackfacing(obj);

                    if (missingNormals) {
                        ImGui::TextColored(ImVec4(1,1,0,1), "Missing normals: will use flat/poor shading."); ImGui::SameLine();
                        if (ImGui::SmallButton("Recompute (angle-weighted)")) {
                            app.recomputeAngleWeightedNormalsForObject(targetIdx);
                        }
                    } else {
                        ImGui::Text("Normals: OK (from source)");
                    }

                    if (backface) {
                        ImGui::TextColored(ImVec4(1,0.6f,0,1), "Bad winding: object faces away (backfaces)."); ImGui::SameLine();
                        if (ImGui::SmallButton("Flip Winding")) {
                            obj.objLoader.flipWindingAndNormals();
                            app.refreshIndexBuffer(obj);
                            app.refreshNormalBuffer(obj);
                        }
                    } else {
                        ImGui::Text("Winding: OK (mostly front-facing)");
                    }
                }

                if (noLights) {
                    ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "No effective lights -> scene is black."); ImGui::SameLine();
                    if (ImGui::SmallButton("Add Neutral Key Light")) {
                        app.addPointLightAt(app.getCameraPosition() + glm::normalize(app.getCameraFront()) * 2.0f, glm::vec3(1.0f), 1.0f);
                    }
                } else {
                    ImGui::Text("Lights: %d active", (int)app.m_lights.m_lights.size());
                }

                ImGui::Separator();
                ImGui::Text("Framebuffer sRGB: %s", app.m_framebufferSRGBEnabled ? "ON" : "OFF"); ImGui::SameLine();
                if (ImGui::SmallButton(app.m_framebufferSRGBEnabled ? "Disable" : "Enable")) { app.m_framebufferSRGBEnabled = !app.m_framebufferSRGBEnabled; }
                if (srgbMismatch) {
                    ImGui::TextColored(ImVec4(1,1,0,1), "sRGB mismatch: PBR already gamma-corrects; sRGB FB doubles it."); ImGui::SameLine();
                    if (ImGui::SmallButton("Fix (disable FB sRGB)")) { app.m_framebufferSRGBEnabled = false; }
                }
            }

            // Shading Mode
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Shading Mode:");
            const char* shadingModes[] = { "Flat", "Gouraud" };
            if (ImGui::Combo("##shadingMode", &app.m_shadingMode, shadingModes, IM_ARRAYSIZE(shadingModes))) {
                // optional: log change
            }

            // Lighting (selected only)
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4("Global Ambient", glm::value_ptr(app.m_lights.m_globalAmbient));
                int sel = app.m_selectedLightIndex;
                if (sel >= 0 && sel < (int)app.m_lights.m_lights.size()) {
                    auto& light = app.m_lights.m_lights[(size_t)sel];
                    ImGui::Checkbox("Enabled", &light.enabled);
                    ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
                    ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);
                } else {
                    ImGui::TextDisabled("No light selected.");
                }
            }

            // Gizmo settings
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Gizmo", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool local = app.m_gizmoLocalSpace;
                if (ImGui::Checkbox("Local space", &local)) app.m_gizmoLocalSpace = local;
                ImGui::SameLine();
                bool snap = app.m_snapEnabled;
                if (ImGui::Checkbox("Snap", &snap)) app.m_snapEnabled = snap;
                ImGui::DragFloat("Translate Snap",     &app.m_snapTranslate, 0.05f, 0.01f, 10.0f, "%.2f");
                ImGui::DragFloat("Rotate Snap (deg)",  &app.m_snapRotateDeg, 1.0f,  1.0f,  90.0f, "%.0f");
                ImGui::DragFloat("Scale Snap",         &app.m_snapScale,     0.01f, 0.01f, 10.0f, "%.2f");
            }
        }
        ImGui::End();
    }

    // ==== Perf HUD ====
    if (ui.showPerfHUD) {
        PerfStats stats{}; app.computePerfStats(stats);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Perf HUD", &ui.showPerfHUD, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Draw calls: %d", stats.drawCalls);
            ImGui::Text("Triangles: %zu", stats.totalTriangles);
            ImGui::Text("Materials: %d", stats.uniqueMaterialKeys);
            ImGui::Text("Textures: %zu (%.2f MB)", stats.uniqueTextures, stats.texturesMB);
            ImGui::Text("Geometry: %.2f MB", stats.geometryMB);
            ImGui::Separator();
            ImGui::Text("VRAM est: %.2f MB", stats.vramMB);

            if (stats.topSharedCount >= 2) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.8f,1.0f,0.2f,1.0f), "%d meshes share material: %s",
                                   stats.topSharedCount, stats.topSharedKey.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Instancing candidate")) {
                    logs.push_back("Perf coach: Consider instancing/batching meshes sharing material: " + stats.topSharedKey);
                }
            } else if (stats.drawCalls > 50) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "High draw calls (%d): merge static meshes or instance.", stats.drawCalls);
            } else if (stats.totalTriangles > 50000) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "High triangle count (%.0fK): consider LOD or decimation.", stats.totalTriangles / 1000.0);
            }

            if (ui.useAI) {
                if (ImGui::SmallButton("Ask AI for perf tips")) {
                    std::ostringstream summary;
                    summary << "Scene perf: drawCalls=" << stats.drawCalls
                            << ", tris=" << stats.totalTriangles
                            << ", materials=" << stats.uniqueMaterialKeys
                            << ", texMB=" << stats.texturesMB
                            << ", geoMB=" << stats.geometryMB
                            << ". TopShare='" << stats.topSharedKey << "' x" << stats.topSharedCount << ".";

                    std::string plan, err;
                    std::string scene = "{}"; // (optional) replace with app.sceneToJson() if you have it
                    std::string prompt = std::string("Give one actionable performance suggestion for this scene (instancing, merging, texture atlases, LOD). Keep it to 1 sentence. ") + summary.str();
                    if (!app.ai().plan(prompt, scene, plan, err))
                        logs.push_back(std::string("AI perf tip error: ") + err);
                    else
                        logs.push_back(std::string("AI perf tip: ") + plan);
                }
            }
        }
        ImGui::End();
    }

    // ==== Scene context menu (right-click) ====
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        ImGui::OpenPopup("SceneContext");
    if (ImGui::BeginPopup("SceneContext")) {
        ImGui::TextUnformatted("Gizmo");
        ImGui::Separator();
        bool sel = (app.m_gizmoMode == GizmoMode::Translate); if (ImGui::MenuItem("Translate", nullptr, sel)) app.m_gizmoMode = GizmoMode::Translate;
        sel = (app.m_gizmoMode == GizmoMode::Rotate);         if (ImGui::MenuItem("Rotate",    nullptr, sel)) app.m_gizmoMode = GizmoMode::Rotate;
        sel = (app.m_gizmoMode == GizmoMode::Scale);          if (ImGui::MenuItem("Scale",     nullptr, sel)) app.m_gizmoMode = GizmoMode::Scale;
        ImGui::Separator();
        bool local = app.m_gizmoLocalSpace; if (ImGui::MenuItem("Local Space", nullptr, local)) app.m_gizmoLocalSpace = !app.m_gizmoLocalSpace;
        bool snap  = app.m_snapEnabled;     if (ImGui::MenuItem("Snap",        nullptr, snap))  app.m_snapEnabled = !app.m_snapEnabled;
        ImGui::Separator();
        if (ImGui::MenuItem("Axis X", nullptr, app.m_gizmoAxis==GizmoAxis::X)) app.m_gizmoAxis = GizmoAxis::X;
        if (ImGui::MenuItem("Axis Y", nullptr, app.m_gizmoAxis==GizmoAxis::Y)) app.m_gizmoAxis = GizmoAxis::Y;
        if (ImGui::MenuItem("Axis Z", nullptr, app.m_gizmoAxis==GizmoAxis::Z)) app.m_gizmoAxis = GizmoAxis::Z;
        ImGui::Separator();
        bool hasObj   = (app.m_selectedObjectIndex >= 0 && app.m_selectedObjectIndex < (int)app.m_sceneObjects.size());
        bool hasLight = (app.m_selectedLightIndex >= 0 && app.m_selectedLightIndex < (int)app.m_lights.m_lights.size());
        if (ImGui::MenuItem("Delete Selected", nullptr, false, hasObj || hasLight)) {
            if (hasObj) {
                std::string name = app.m_sceneObjects[(size_t)app.m_selectedObjectIndex].name;
                if (app.removeObjectByName(name)) logs.push_back(std::string("Deleted object: ") + name);
            } else if (hasLight) {
                if (app.removeLightAtIndex(app.m_selectedLightIndex)) logs.push_back("Deleted selected light.");
            }
        }
        if (ImGui::MenuItem("Duplicate Selected", nullptr, false, hasObj || hasLight)) {
            if (hasObj) {
                const auto& src = app.m_sceneObjects[(size_t)app.m_selectedObjectIndex];
                std::string newName = src.name + std::string("_copy");
                glm::vec3 dpos(0.2f, 0.0f, 0.0f);
                if (app.duplicateObject(src.name, newName, &dpos, nullptr, nullptr))
                    logs.push_back(std::string("Duplicated object as: ") + newName);
            } else if (hasLight) {
                auto L = app.m_lights.m_lights[(size_t)app.m_selectedLightIndex];
                app.addPointLightAt(L.position + glm::vec3(0.2f, 0, 0), L.color, L.intensity);
                logs.push_back("Duplicated selected light.");
            }
        }
        ImGui::EndPopup();
    }

    // ==== Bottom console window ====
    {
        ImGuiIO& io = ImGui::GetIO();
        float H = io.DisplaySize.y * 0.25f;
        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - H), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, H), ImGuiCond_Always);
        if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            if (ImGui::BeginChild("##console_scrollback", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                for (const auto& line : logs) ImGui::TextUnformatted(line.c_str());
            }
            ImGui::EndChild();

            static char quick[256] = {0};
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##console_input", quick, IM_ARRAYSIZE(quick), ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::string cmd = quick; quick[0] = '\0';
                bool handled = false;

                if (ui.aiSuggestOpenDiag && !cmd.empty()) {
                    std::string s = cmd; for (auto& c : s) c = (char)std::tolower((unsigned char)c);
                    if (s == "yes" || s == "y") {
                        ui.showSettingsPanel = true; ui.aiSuggestOpenDiag = false; handled = true;
                        logs.push_back("Opening settings + diagnostics panel.");
                    }
                }

                if (!handled && !cmd.empty()) {
                    if (ui.useAI) {
                        std::string plan, err;
                        std::string scene = "{}"; // replace with app.sceneToJson() if you have it
                        if (!app.ai().plan(cmd, scene, plan, err)) {
                            logs.push_back(std::string("AI error: ") + err);
                        } else {
                            logs.push_back(std::string("AI plan:\n") + plan);
                            std::istringstream is(plan);
                            std::string line;
                            while (std::getline(is, line)) {
                                if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
                                std::vector<std::string> plogs;
                                bool ok = app.commands().execute(line, plogs);
                                for (auto& l : plogs) logs.push_back(l);
                                if (!ok) logs.push_back("(no changes)");
                            }
                        }
                    } else {
                        std::vector<std::string> plogs;
                        bool ok = app.commands().execute(cmd, plogs);
                        for (auto& l : plogs) logs.push_back(l);
                        if (!ok) logs.push_back("(no changes)");
                    }
                }
            }
        }
        ImGui::End();
    }

#endif // !WEB_USE_HTML_UI
}
