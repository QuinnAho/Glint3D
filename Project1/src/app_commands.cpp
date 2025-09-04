#include "app_commands.h"
#include "application.h"
#include "material.h"
#include <sstream>
#include <algorithm>
#include <cctype>

void AppCommands::attach(Application& app) { app_ = &app; }

void AppCommands::add(const std::string& name, CommandHandler fn, const std::string& helpLine) {
    cmds_[name] = std::move(fn);
    help_[name] = helpLine;
}

std::vector<std::string> AppCommands::tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') { inQuotes = !inQuotes; continue; }
        if (!inQuotes && std::isspace((unsigned char)c)) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

bool AppCommands::strEq(const std::string& a, const char* b) {
    if (!b) return false;
    if (a.size() != std::strlen(b)) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

bool AppCommands::parseVec3(const std::vector<std::string>& t, size_t i, glm::vec3& out) {
    if (i + 2 >= t.size()) return false;
    try {
        out.x = std::stof(t[i+0]);
        out.y = std::stof(t[i+1]);
        out.z = std::stof(t[i+2]);
        return true;
    } catch (...) { return false; }
}

bool AppCommands::parseFloat(const std::vector<std::string>& t, size_t i, float& out) {
    if (i >= t.size()) return false;
    try { out = std::stof(t[i]); return true; } catch (...) { return false; }
}

bool AppCommands::parseInt(const std::vector<std::string>& t, size_t i, int& out) {
    if (i >= t.size()) return false;
    try { out = std::stoi(t[i]); return true; } catch (...) { return false; }
}

bool AppCommands::execute(const std::string& line, std::vector<std::string>& logs) {
    if (!app_) { logs.push_back("[cmd] No application attached."); return false; }
    auto toks = tokenize(line);
    if (toks.empty()) return false;

    // Find exact command match on first token; fall back to 'help'
    auto it = cmds_.find(toks[0]);
    CommandCtx ctx{ *app_, line, toks };

    if (it == cmds_.end()) {
        logs.push_back("[cmd] Unknown command: " + toks[0]);
        logs.push_back("Type: help");
        return false;
    }
    return it->second(ctx, logs);
}

std::vector<std::string> AppCommands::helpLines() const {
    std::vector<std::string> lines;
    lines.reserve(help_.size());
    for (auto& kv : help_) lines.push_back(kv.first + " — " + kv.second);
    std::sort(lines.begin(), lines.end());
    return lines;
}

// -------------------- Built-in command set --------------------

void RegisterDefaultCommands(AppCommands& R) {
    // help
    R.add("help",
        [&R](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            (void)ctx;
            auto lines = R.helpLines();
            logs.push_back("Available commands:");
            for (auto& s : lines) logs.push_back("  " + s);
            return false;
        },
        "List available commands");

    // load: load <name> <path> [--front <meters>] [--at x y z] [--scale sx sy sz]
    R.add("load",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() < 3) {
                logs.push_back("Usage: load <name> <path> [--front <d>] [--at x y z] [--scale sx sy sz]");
                return false;
            }
            auto& app = ctx.app;
            std::string name = ctx.tokens[1];
            std::string path = ctx.tokens[2];
            bool useFront = true; float frontMeters = 2.0f;
            glm::vec3 at{0}; bool hasAt = false;
            glm::vec3 sc{1}; bool hasScale = false;

            for (size_t i = 3; i < ctx.tokens.size(); ++i) {
                if (AppCommands::strEq(ctx.tokens[i], "--front")) {
                    float d=2.0f; if (i+1<ctx.tokens.size() && AppCommands::parseFloat(ctx.tokens, i+1, d)) { frontMeters = d; i++; }
                    useFront = true;
                } else if (AppCommands::strEq(ctx.tokens[i], "--at")) {
                    if (AppCommands::parseVec3(ctx.tokens, i+1, at)) { hasAt = true; useFront = false; i+=3; }
                } else if (AppCommands::strEq(ctx.tokens[i], "--scale")) {
                    if (AppCommands::parseVec3(ctx.tokens, i+1, sc)) { hasScale = true; i+=3; }
                }
            }

            bool ok = false;
            if (hasAt) ok = app.loadObjAt(name, path, at, hasScale?sc:glm::vec3(1.0f));
            else       ok = app.loadObjInFrontOfCamera(name, path, frontMeters, hasScale?sc:glm::vec3(1.0f));

            logs.push_back(ok ? "Loaded '" + name + "' from " + path
                              : "Failed to load '" + name + "' from " + path);
            return ok;
        },
        "Load a model: load <name> <path> [--front <d>] [--at x y z] [--scale sx sy sz]");

    // duplicate: duplicate <src> <dst> [dx dy dz]
    R.add("duplicate",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() < 3) {
                logs.push_back("Usage: duplicate <sourceName> <newName> [dx dy dz]");
                return false;
            }
            glm::vec3 dpos{0}; bool hasDelta = false;
            if (ctx.tokens.size() >= 6) {
                hasDelta = AppCommands::parseVec3(ctx.tokens, 3, dpos);
            }
            bool ok = ctx.app.duplicateObject(ctx.tokens[1], ctx.tokens[2], hasDelta?&dpos:nullptr, nullptr, nullptr);
            logs.push_back(ok ? ("Duplicated '" + ctx.tokens[1] + "' as '" + ctx.tokens[2] + "'")
                              : ("Failed to duplicate '" + ctx.tokens[1] + "'"));
            return ok;
        },
        "Duplicate object: duplicate <src> <dst> [dx dy dz]");

    // move: move <name> dx dy dz
    R.add("move",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 5) { logs.push_back("Usage: move <name> dx dy dz"); return false; }
            glm::vec3 d{0};
            if (!AppCommands::parseVec3(ctx.tokens, 2, d)) { logs.push_back("Bad delta."); return false; }
            bool ok = ctx.app.moveObjectByName(ctx.tokens[1], d);
            logs.push_back(ok ? ("Moved '" + ctx.tokens[1] + "'.") : ("No object named '" + ctx.tokens[1] + "'."));
            return ok;
        },
        "Move an object: move <name> dx dy dz");

    // delete: delete selected | delete object <name> | delete light <index>
    R.add("delete",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            auto& app = ctx.app;
            if (ctx.tokens.size() == 2 && AppCommands::strEq(ctx.tokens[1], "selected")) {
                int oi = app.getSelectedObjectIndex();
                int li = app.getSelectedLightIndex();
                if (oi >= 0) {
                    std::string n = app.getSceneObjects()[(size_t)oi].name;
                    bool ok = app.removeObjectByName(n);
                    logs.push_back(ok ? "Deleted object: " + n : "Failed to delete: " + n);
                    return ok;
                } else if (li >= 0) {
                    bool ok = app.removeLightAtIndex(li);
                    logs.push_back(ok ? "Deleted selected light." : "Failed to delete selected light.");
                    return ok;
                } else {
                    logs.push_back("Nothing selected.");
                    return false;
                }
            } else if (ctx.tokens.size() == 3 && AppCommands::strEq(ctx.tokens[1], "object")) {
                bool ok = app.removeObjectByName(ctx.tokens[2]);
                logs.push_back(ok ? ("Deleted object: " + ctx.tokens[2]) : ("No such object: " + ctx.tokens[2]));
                return ok;
            } else if (ctx.tokens.size() == 3 && AppCommands::strEq(ctx.tokens[1], "light")) {
                int idx=-1; if (!AppCommands::parseInt(ctx.tokens, 2, idx)) { logs.push_back("Bad light index."); return false; }
                bool ok = app.removeLightAtIndex(idx);
                logs.push_back(ok ? "Deleted light." : "No such light.");
                return ok;
            } else {
                logs.push_back("Usage: delete selected | delete object <name> | delete light <index>");
                return false;
            }
        },
        "Delete selection/object/light");

    // select: select object <name> | select light <index>
    R.add("select",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() < 3) { logs.push_back("Usage: select object <name> | select light <index>"); return false; }
            auto& app = ctx.app;
            if (AppCommands::strEq(ctx.tokens[1], "object")) {
                std::string name = ctx.tokens[2];
                const auto& objs = app.getSceneObjects();
                for (int i=0;i<(int)objs.size();++i) if (objs[(size_t)i].name == name) {
                    app.setSelectedObjectIndex(i);
                    app.setSelectedLightIndex(-1);
                    logs.push_back("Selected object: " + name);
                    return true;
                }
                logs.push_back("No object named: " + name);
                return false;
            } else if (AppCommands::strEq(ctx.tokens[1], "light")) {
                int idx=-1; if (!AppCommands::parseInt(ctx.tokens, 2, idx)) { logs.push_back("Bad light index."); return false; }
                if (idx < 0 || idx >= app.getLightCount()) { logs.push_back("No such light."); return false; }
                app.setSelectedLightIndex(idx);
                logs.push_back("Selected light #" + std::to_string(idx));
                return true;
            }
            logs.push_back("Usage: select object <name> | select light <index>");
            return false;
        },
        "Select an object/light");

    // light add [x y z] [r g b] [intensity]
    R.add("light.add",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            glm::vec3 p = ctx.app.getCameraPosition() + glm::normalize(ctx.app.getCameraFront()) * 2.0f;
            glm::vec3 c(1.0f);
            float I = 1.0f;

            size_t n = ctx.tokens.size();
            if (n >= 4) { AppCommands::parseVec3(ctx.tokens, 1, p); }
            if (n >= 7) { AppCommands::parseVec3(ctx.tokens, 4, c); }
            if (n >= 8) { AppCommands::parseFloat(ctx.tokens, 7, I); }

            bool ok = ctx.app.addPointLightAt(p, c, I);
            logs.push_back(ok ? "Added light." : "Failed to add light.");
            return ok;
        },
        "Add point light: light.add [x y z] [r g b] [intensity]");

    // material create <name> [diff r g b] [spec r g b] [amb r g b] [shin s] [rough r] [metal m]
    R.add("material.create",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() < 2) { logs.push_back("Usage: material.create <name> [diff r g b] [spec r g b] [amb r g b] [shin s] [rough r] [metal m]"); return false; }
            Material m;
            for (size_t i=2;i<ctx.tokens.size();++i) {
                if (AppCommands::strEq(ctx.tokens[i],"diff") && i+3<ctx.tokens.size()) { AppCommands::parseVec3(ctx.tokens, i+1, m.diffuse); i+=3; }
                else if (AppCommands::strEq(ctx.tokens[i],"spec") && i+3<ctx.tokens.size()) { AppCommands::parseVec3(ctx.tokens, i+1, m.specular); i+=3; }
                else if (AppCommands::strEq(ctx.tokens[i],"amb")  && i+3<ctx.tokens.size()) { AppCommands::parseVec3(ctx.tokens, i+1, m.ambient); i+=3; }
                else if (AppCommands::strEq(ctx.tokens[i],"shin") && i+1<ctx.tokens.size()) { AppCommands::parseFloat(ctx.tokens, i+1, m.shininess); i+=1; }
                else if (AppCommands::strEq(ctx.tokens[i],"rough")&& i+1<ctx.tokens.size()) { AppCommands::parseFloat(ctx.tokens, i+1, m.roughness); i+=1; }
                else if (AppCommands::strEq(ctx.tokens[i],"metal")&& i+1<ctx.tokens.size()) { AppCommands::parseFloat(ctx.tokens, i+1, m.metallic); i+=1; }
            }
            bool ok = ctx.app.createMaterialNamed(ctx.tokens[1], m);
            logs.push_back(ok ? ("Material created: " + ctx.tokens[1]) : "Failed to create material");
            return ok;
        },
        "Create material: material.create <name> [diff r g b] [spec r g b] [amb r g b] [shin s] [rough r] [metal m]");

    // material assign <object> <material>
    R.add("material.assign",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 3) { logs.push_back("Usage: material.assign <objectName> <materialName>"); return false; }
            bool ok = ctx.app.assignMaterialToObject(ctx.tokens[1], ctx.tokens[2]);
            logs.push_back(ok ? ("Assigned '" + ctx.tokens[2] + "' to '" + ctx.tokens[1] + "'") : "Failed to assign material.");
            return ok;
        },
        "Assign material: material.assign <object> <material>");

    // camera setpos x y z
    R.add("camera.setpos",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 4) { logs.push_back("Usage: camera.setpos x y z"); return false; }
            glm::vec3 p;
            if (!AppCommands::parseVec3(ctx.tokens, 1, p)) { logs.push_back("Bad position."); return false; }
            ctx.app.setCameraFrontUp(p, ctx.app.getCameraFront(), ctx.app.getCameraUp());
            logs.push_back("Camera position set.");
            return true;
        },
        "Move camera position");

    // camera.lookat x y z
    R.add("camera.lookat",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 4) { logs.push_back("Usage: camera.lookat x y z"); return false; }
            glm::vec3 t;
            if (!AppCommands::parseVec3(ctx.tokens, 1, t)) { logs.push_back("Bad target."); return false; }
            ctx.app.setCameraTarget(ctx.app.getCameraPosition(), t, ctx.app.getCameraUp());
            logs.push_back("Camera target set.");
            return true;
        },
        "Aim camera at a target");

    // camera.lens fov near far
    R.add("camera.lens",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 4) { logs.push_back("Usage: camera.lens <fovDeg> <near> <far>"); return false; }
            float f,n,z;
            if (!AppCommands::parseFloat(ctx.tokens, 1, f) ||
                !AppCommands::parseFloat(ctx.tokens, 2, n) ||
                !AppCommands::parseFloat(ctx.tokens, 3, z)) {
                logs.push_back("Bad lens values.");
                return false;
            }
            ctx.app.setCameraLens(f, n, z);
            logs.push_back("Camera lens updated.");
            return true;
        },
        "Set FOV/near/far");

    // gizmo.mode translate|rotate|scale
    R.add("gizmo.mode",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 2) { logs.push_back("Usage: gizmo.mode translate|rotate|scale"); return false; }
            if (AppCommands::strEq(ctx.tokens[1],"translate")) ctx.app.setGizmoMode(GizmoMode::Translate);
            else if (AppCommands::strEq(ctx.tokens[1],"rotate")) ctx.app.setGizmoMode(GizmoMode::Rotate);
            else if (AppCommands::strEq(ctx.tokens[1],"scale"))  ctx.app.setGizmoMode(GizmoMode::Scale);
            else { logs.push_back("Unknown mode."); return false; }
            logs.push_back("Gizmo mode set.");
            return true;
        },
        "Set gizmo mode");

    // gizmo.axis x|y|z|none
    R.add("gizmo.axis",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 2) { logs.push_back("Usage: gizmo.axis x|y|z|none"); return false; }
            if (AppCommands::strEq(ctx.tokens[1],"x"))   ctx.app.setGizmoAxis(GizmoAxis::X);
            else if (AppCommands::strEq(ctx.tokens[1],"y")) ctx.app.setGizmoAxis(GizmoAxis::Y);
            else if (AppCommands::strEq(ctx.tokens[1],"z")) ctx.app.setGizmoAxis(GizmoAxis::Z);
            else if (AppCommands::strEq(ctx.tokens[1],"none")) ctx.app.setGizmoAxis(GizmoAxis::None);
            else { logs.push_back("Unknown axis."); return false; }
            logs.push_back("Gizmo axis set.");
            return true;
        },
        "Set gizmo axis");

    // gizmo.local on|off|toggle
    R.add("gizmo.local",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 2) { logs.push_back("Usage: gizmo.local on|off|toggle"); return false; }
            bool cur = ctx.app.isGizmoLocalSpace();
            if (AppCommands::strEq(ctx.tokens[1],"toggle")) ctx.app.toggleGizmoLocalSpace();
            else if (AppCommands::strEq(ctx.tokens[1],"on"))  { if (!cur) ctx.app.toggleGizmoLocalSpace(); }
            else if (AppCommands::strEq(ctx.tokens[1],"off")) { if ( cur) ctx.app.toggleGizmoLocalSpace(); }
            else { logs.push_back("Unknown option."); return false; }
            logs.push_back(std::string("Gizmo local = ") + (ctx.app.isGizmoLocalSpace()?"ON":"OFF"));
            return true;
        },
        "Toggle local/world gizmo space");

    // gizmo.snap on|off|toggle
    R.add("gizmo.snap",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 2) { logs.push_back("Usage: gizmo.snap on|off|toggle"); return false; }
            bool cur = ctx.app.isSnapEnabled();
            if (AppCommands::strEq(ctx.tokens[1],"toggle")) ctx.app.toggleSnap();
            else if (AppCommands::strEq(ctx.tokens[1],"on"))  { if (!cur) ctx.app.toggleSnap(); }
            else if (AppCommands::strEq(ctx.tokens[1],"off")) { if ( cur) ctx.app.toggleSnap(); }
            else { logs.push_back("Unknown option."); return false; }
            logs.push_back(std::string("Gizmo snap = ") + (ctx.app.isSnapEnabled()?"ON":"OFF"));
            return true;
        },
        "Toggle gizmo snapping");

    // denoise on|off|toggle
    R.add("denoise",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            if (ctx.tokens.size() != 2) { logs.push_back("Usage: denoise on|off|toggle"); return false; }
            bool cur = ctx.app.isDenoiseEnabled();
            if (AppCommands::strEq(ctx.tokens[1],"toggle")) ctx.app.setDenoiseEnabled(!cur);
            else if (AppCommands::strEq(ctx.tokens[1],"on"))  ctx.app.setDenoiseEnabled(true);
            else if (AppCommands::strEq(ctx.tokens[1],"off")) ctx.app.setDenoiseEnabled(false);
            else { logs.push_back("Unknown option."); return false; }
            logs.push_back(std::string("Denoise = ") + (ctx.app.isDenoiseEnabled()?"ON":"OFF"));
            return true;
        },
        "Toggle the OIDN denoiser");

    // fullscreen
    R.add("fullscreen",
        [](const CommandCtx& ctx, std::vector<std::string>& logs)->bool {
            (void)ctx;
            logs.push_back("Toggling fullscreen…");
            ctx.app.toggleFullscreen();
            return true;
        },
        "Toggle fullscreen");
}

// ---------------- Wire up into Application -------------------

#include "app_state.h"

// Define the private methods declared in Application
// (place these at the bottom of this file to keep all UI plumbing in one place)
void Application::buildUiState_() {
    // If you want a richer snapshot, expand BuildUiStateFromApp
    BuildUiStateFromApp(*this);
}

void Application::bindUiCommands_() {
    uiCmd_.attach(*this);
    RegisterDefaultCommands(uiCmd_);
}