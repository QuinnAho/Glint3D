#include "nl_executor.h"
#include "application.h"

#include <algorithm>
#include <cctype>
#include <sstream>

using glm::vec3;

namespace nl {

static std::string tolower_copy(const std::string& s){
    std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return (char)std::tolower(c); }); return r;
}

std::string Executor::trim(const std::string& s){
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

bool Executor::iequalsPrefix(const std::string& s, const std::string& prefix){
    std::string sl = tolower_copy(trim(s));
    std::string pl = tolower_copy(prefix);
    return sl.rfind(pl, 0) == 0; // starts_with
}

std::optional<vec3> Executor::parseVec3(const std::string& s){
    // Accept forms: "x y z" or "(x,y,z)" or "x, y, z"
    std::string t; t.reserve(s.size());
    for (char c : s) { if (std::isdigit((unsigned char)c) || c=='-' || c=='+' || c=='.' || c==',' || std::isspace((unsigned char)c)) t.push_back(c); }
    std::replace(t.begin(), t.end(), ',', ' ');
    std::istringstream is(t);
    float x,y,z; if (is>>x>>y>>z) return vec3{x,y,z};
    return std::nullopt;
}

std::vector<std::string> Executor::tokenize(const std::string& s){
    std::vector<std::string> out; std::istringstream is(s); std::string tok; while (is>>tok) out.push_back(tok); return out;
}

bool Executor::execute(const std::string& text, std::vector<std::string>& outLog){
    std::string t = trim(text);
    if (t.empty()) return false;
    std::string tl = tolower_copy(t);

    // Toggle fullscreen
    if (tl == "toggle fullscreen" || tl == "toggle fullscreen mode" || tl == "fullscreen") {
        outLog.push_back("Toggling fullscreen");
        m_app.toggleFullscreen();
        return true;
    }

    // Snapshot
    if (iequalsPrefix(t, "scene json") || iequalsPrefix(t, "export scene") || iequalsPrefix(t, "get scene")){
        auto json = m_app.sceneToJson();
        outLog.push_back(std::string("Scene JSON:\n") + json);
        return true;
    }

    // Add light: "add light [at x y z] [color r g b] [intensity f]"
    if (iequalsPrefix(t, "add light")){
        vec3 pos{0,0,0}; vec3 color{1,1,1}; float intensity=1.0f;
        // crude parsing
        auto atPos = tl.find(" at "); if (atPos != std::string::npos){
            auto rest = t.substr(atPos+4);
            auto v = parseVec3(rest); if (v) pos = *v;
        }
        auto colPos = tl.find(" color "); if (colPos != std::string::npos){
            auto rest = t.substr(colPos+7);
            auto v = parseVec3(rest); if (v) color = *v;
        }
        auto iPos = tl.find(" intensity "); if (iPos != std::string::npos){
            auto rest = t.substr(iPos+10); std::istringstream is(rest); is>>intensity;
        }
        m_app.addPointLightAt(pos, color, intensity);
        std::ostringstream os; os<<"Added light at ("<<pos.x<<","<<pos.y<<","<<pos.z<<")";
        outLog.push_back(os.str());
        return true;
    }

    // Remove object: "remove [this|selected|<name>] [object]"
    if (iequalsPrefix(t, "remove ")) {
        auto toks = tokenize(t);
        if (toks.size() >= 2) {
            std::string target = tolower_copy(toks[1]);
            if (target == "this" || target == "selected") {
                std::string sel = m_app.getSelectedObjectName();
                if (sel.empty()) { outLog.push_back("No object is selected"); return false; }
                bool ok = m_app.removeObjectByName(sel);
                outLog.push_back(ok ? ("Removed '"+sel+"'") : ("Failed to remove '"+sel+"'"));
                return ok;
            } else {
                // allow optional trailing word 'object'
                if (toks.size() >= 3 && tolower_copy(toks[2]) == "object") {
                    // target already set
                }
                bool ok = m_app.removeObjectByName(toks[1]);
                outLog.push_back(ok ? ("Removed '"+toks[1]+"'") : ("Failed to remove '"+toks[1]+"'"));
                return ok;
            }
        }
    }

    // Move object: "move [this|selected|<name>] <dir> [dist]"
    // dirs: left, right, up, down, forward, back/backward (camera-relative)
    if (iequalsPrefix(t, "move ")) {
        auto toks = tokenize(t);
        if (toks.size() >= 3) {
            std::string nameTok = tolower_copy(toks[1]);
            std::string dirTok  = tolower_copy(toks[2]);
            float dist = 1.0f;
            if (toks.size() >= 4) { try { dist = std::stof(toks[3]); } catch(...){} }

            std::string name;
            if (nameTok == "this" || nameTok == "selected") {
                name = m_app.getSelectedObjectName();
                if (name.empty()) { outLog.push_back("No object is selected"); return false; }
            } else {
                name = toks[1];
            }

            glm::vec3 front = glm::normalize(m_app.getCameraFront());
            glm::vec3 up    = glm::normalize(m_app.getCameraUp());
            glm::vec3 right = glm::normalize(glm::cross(front, up));
            glm::vec3 delta(0.0f);
            if (dirTok == "left")      delta = -right * dist;
            else if (dirTok == "right") delta =  right * dist;
            else if (dirTok == "up")    delta =  up    * dist;
            else if (dirTok == "down")  delta = -up    * dist;
            else if (dirTok == "forward") delta = front * dist;
            else if (dirTok == "back" || dirTok == "backward") delta = -front * dist;
            else { outLog.push_back("Unknown direction; use left/right/up/down/forward/back"); return false; }

            bool ok = m_app.moveObjectByName(name, delta);
            std::ostringstream os; os << "Moved '"<<name<<"' by ("<<delta.x<<","<<delta.y<<","<<delta.z<<")";
            outLog.push_back(ok ? os.str() : ("Failed to move '"+name+"'"));
            return ok;
        }
    }

    // Multi-place with arrangement: "place 4 cube objects ... arrange them ... three walls and one floor"
    // Also supports simple adjectives like "long", "flat", "tall", "wide" affecting default scale
    if (iequalsPrefix(t, "place ")){
        auto toks = tokenize(t);
        if (toks.size() >= 3){
            // Try to parse count in second token
            int count = 1;
            try { count = std::stoi(toks[1]); } catch(...) { /* not a number */ }
            if (count > 1) {
                // Base name/path for the object to place
                std::string baseName = toks[2];
                std::string path = baseName; // rely on Application::loadObjAt path resolution

                // Heuristic scale from adjectives
                glm::vec3 baseScale(1.0f);
                if (tl.find(" long") != std::string::npos) baseScale.x = std::max(baseScale.x, 4.0f);
                if (tl.find(" flat") != std::string::npos) baseScale.y = std::min(baseScale.y, 0.2f);
                if (tl.find(" tall") != std::string::npos) baseScale.y = std::max(baseScale.y, 3.0f);
                if (tl.find(" wide") != std::string::npos) baseScale.x = std::max(baseScale.x, 4.0f);

                // Explicit numeric scale override if present: "scale sx sy sz"
                auto sclPos = tl.find(" scale ");
                if (sclPos != std::string::npos) {
                    // Try to parse three numbers following 'scale'
                    auto rest = t.substr(sclPos + 7);
                    auto v = parseVec3(rest);
                    if (v) baseScale = *v;
                }

                // Default placement reference in front of camera
                glm::vec3 camPos = m_app.getCameraPosition();
                glm::vec3 front  = glm::normalize(m_app.getCameraFront());
                glm::vec3 up     = glm::normalize(m_app.getCameraUp());
                glm::vec3 right  = glm::normalize(glm::cross(front, up));

                // Recognize special arrangement: three walls and one floor
                bool wantWallsFloor = (tl.find("three walls") != std::string::npos || tl.find("3 walls") != std::string::npos)
                                   && (tl.find("one floor")   != std::string::npos || tl.find("1 floor")   != std::string::npos);

                if (wantWallsFloor) {
                    // Force count to 4 for this pattern
                    const float roomSize = 6.0f;     // X/Z extent
                    const float wallHeight = 3.0f;   // Y for walls
                    const float thickness = 0.2f;    // small thickness
                    const float distForward = 3.0f;  // center the room a bit ahead

                    glm::vec3 center = camPos + front * distForward;

                    // Floor
                    glm::vec3 floorScale(roomSize, thickness, roomSize);
                    if (baseScale.x > 1.0f) floorScale.x = baseScale.x; // allow adjective override for length
                    if (baseScale.y < 1.0f) floorScale.y = std::min(baseScale.y, thickness);
                    if (baseScale.z > 1.0f) floorScale.z = baseScale.z;
                    glm::vec3 floorPos = center - up * (floorScale.y * 0.5f);

                    // Back wall (across X)
                    glm::vec3 backScale(roomSize, wallHeight, thickness);
                    glm::vec3 backPos = center - front * (roomSize * 0.5f - backScale.z * 0.5f) + up * (backScale.y * 0.5f);

                    // Left and right walls (along Z)
                    glm::vec3 sideScale(thickness, wallHeight, roomSize);
                    glm::vec3 leftPos  = center - right * (roomSize * 0.5f - sideScale.x * 0.5f) + up * (sideScale.y * 0.5f);
                    glm::vec3 rightPos = center + right * (roomSize * 0.5f - sideScale.x * 0.5f) + up * (sideScale.y * 0.5f);

                    // Place objects
                    bool ok = true;
                    ok &= m_app.loadObjAt(baseName + "_floor", path, floorPos, floorScale);
                    ok &= m_app.loadObjAt(baseName + "_wall_back", path, backPos, backScale);
                    ok &= m_app.loadObjAt(baseName + "_wall_left", path, leftPos, sideScale);
                    ok &= m_app.loadObjAt(baseName + "_wall_right", path, rightPos, sideScale);

                    outLog.push_back(ok ? "Placed 3 walls and 1 floor (U-shape)" : "Failed to place room arrangement");
                    return ok;
                }

                // Generic multi-placement: line in front, spaced along camera-right
                const float distForward = 2.5f;
                const float spacing = 2.0f;
                glm::vec3 start = camPos + front * distForward - right * (spacing * 0.5f * (float)(count - 1));
                bool allOk = true;
                for (int i = 0; i < count; ++i) {
                    std::ostringstream nm; nm << baseName << (i+1);
                    glm::vec3 pos = start + right * (spacing * (float)i);
                    allOk &= m_app.loadObjAt(nm.str(), path, pos, baseScale);
                }
                outLog.push_back(allOk ? ("Placed " + std::to_string(count) + " objects of '" + baseName + "'") : "Failed to place some objects");
                return allOk;
            }
        }
    }

    // Place/load obj in front of camera: "place cow" or "place cow at x y z" or "load my.obj in front of me [d]"
    if (iequalsPrefix(t, "place ") || iequalsPrefix(t, "load ")){
        auto toks = tokenize(t);
        if (toks.size() >= 2){
            std::string name = toks[1];
            // allow explicit .obj
            std::string path = name;
            // distance in front of camera
            float dist = 2.0f;
            vec3 scale(1.0f);
            bool atRequested = false; vec3 atPos;
            // scan for options
            for (size_t i=2;i<toks.size();++i){
                std::string w = tolower_copy(toks[i]);
                if (w == "at" && i+3 < toks.size()){
                    atRequested = true;
                    try{
                        atPos = vec3{ std::stof(toks[i+1]), std::stof(toks[i+2]), std::stof(toks[i+3]) };
                        i+=3;
                    } catch(...){}
                } else if (w=="scale" && i+3 < toks.size()){
                    try{ scale = vec3{ std::stof(toks[i+1]), std::stof(toks[i+2]), std::stof(toks[i+3]) }; i+=3; }catch(...){}
                } else if (w=="in" && i+3 < toks.size() && tolower_copy(toks[i+1])=="front" && tolower_copy(toks[i+2])=="of" && tolower_copy(toks[i+3])=="me"){
                    if (i+4 < toks.size()){
                        try { dist = std::stof(toks[i+4]); i+=4; } catch(...){}
                    } else { i+=3; }
                }
            }

            bool ok=false;
            if (atRequested){
                ok = m_app.loadObjAt(name, path, atPos, scale);
            } else {
                ok = m_app.loadObjInFrontOfCamera(name, path, dist, scale);
            }
            outLog.push_back(ok ? ("Placed object '"+name+"'") : ("Failed to place object '"+name+"'"));
            return ok;
        }
    }

    // Create material: "create material wood color r g b specular r g b shininess s roughness r metallic m"
    if (iequalsPrefix(t, "create material ")){
        std::string rest = t.substr(16);
        auto toks = tokenize(rest);
        if (toks.size()>=1){
            std::string matName = toks[0];
            Material m; // defaults
            // look for named properties
            auto tlr = tolower_copy(rest);
            auto cp = tlr.find("color "); if (cp!=std::string::npos){ auto v = parseVec3(rest.substr(cp+6)); if (v) m.diffuse=*v; }
            auto sp = tlr.find("specular "); if (sp!=std::string::npos){ auto v = parseVec3(rest.substr(sp+9)); if (v) m.specular=*v; }
            auto ap = tlr.find("ambient "); if (ap!=std::string::npos){ auto v = parseVec3(rest.substr(ap+8)); if (v) m.ambient=*v; }
            auto shp = tlr.find("shininess "); if (shp!=std::string::npos){ std::istringstream is(rest.substr(shp+10)); is>>m.shininess; }
            auto rp = tlr.find("roughness "); if (rp!=std::string::npos){ std::istringstream is(rest.substr(rp+10)); is>>m.roughness; }
            auto mp = tlr.find("metallic "); if (mp!=std::string::npos){ std::istringstream is(rest.substr(mp+9)); is>>m.metallic; }
            bool ok = m_app.createMaterialNamed(matName, m);
            outLog.push_back(ok ? ("Created material '"+matName+"'") : ("Failed to create material '"+matName+"'"));
            return ok;
        }
    }

    // Assign material: "assign material wood to cow"
    if (iequalsPrefix(t, "assign material ")){
        std::string rest = t.substr(16);
        auto toPos = tolower_copy(rest).find(" to ");
        if (toPos != std::string::npos){
            std::string matName = trim(rest.substr(0, toPos));
            std::string objName = trim(rest.substr(toPos+4));
            bool ok = m_app.assignMaterialToObject(objName, matName);
            outLog.push_back(ok ? ("Assigned '"+matName+"' to '"+objName+"'") : ("Failed to assign '"+matName+"'"));
            return ok;
        }
    }

    // TODO: generate primitives e.g., "create cube 1 1 1 at x y z" or "add plane size x"
    outLog.push_back("Unrecognized instruction. Try: 'place cow', 'add light', 'create material', 'assign material', 'fullscreen', or 'scene json'");
    return false;
}

} // namespace nl
