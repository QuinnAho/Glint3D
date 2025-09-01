#include "application.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

// ---- JSON Ops v1 apply (minimal tolerant parser) ----
namespace {
    static inline std::string trim_copy(std::string s){
        auto ns = [](int c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), ns));
        s.erase(std::find_if(s.rbegin(), s.rend(), ns).base(), s.end());
        return s;
    }
    static bool findString(const std::string& src, const std::string& key, std::string& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false;
        pos = src.find(':', pos); if (pos==std::string::npos) return false;
        pos = src.find('"', pos); if (pos==std::string::npos) return false;
        auto end = pos + 1; std::string s;
        while (end < src.size()) { char c = src[end++]; if (c=='\\') { if (end<src.size()) s.push_back(src[end++]); } else if (c=='"') break; else s.push_back(c);} out = s; return true;
    }
    static bool findNumber(const std::string& src, const std::string& key, float& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false;
        auto i = pos + 1; while (i<src.size() && (std::isspace((unsigned char)src[i])||src[i]=='"')) ++i;
        std::string num; while (i<src.size() && (std::isdigit((unsigned char)src[i])||src[i]=='-'||src[i]=='+'||src[i]=='.'||src[i]=='e'||src[i]=='E')) num.push_back(src[i++]);
        if (num.empty()) return false; try { out = std::stof(num); return true; } catch (...) { return false; }
    }
    static bool findBool(const std::string& src, const std::string& key, bool& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false;
        auto i = pos + 1; while (i<src.size() && std::isspace((unsigned char)src[i])) ++i;
        if (i>=src.size()) return false;
        if (src.compare(i, 4, "true") == 0) { out = true; return true; }
        if (src.compare(i, 5, "false") == 0) { out = false; return true; }
        if (src[i]=='1') { out = true; return true; }
        if (src[i]=='0') { out = false; return true; }
        return false;
    }
    static bool findVec3(const std::string& src, const std::string& key, glm::vec3& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false; pos = src.find('[', pos); if (pos==std::string::npos) return false;
        auto end = src.find(']', pos); if (end==std::string::npos) return false; std::string inside = src.substr(pos+1, end-pos-1);
        std::stringstream ss(inside); float x=0,y=0,z=0; char ch; if (!(ss>>x)) return false; ss>>ch; if (!(ss>>y)) return false; ss>>ch; if (!(ss>>z)) return false; out = glm::vec3(x,y,z); return true;
    }
}

bool Application::applyJsonOpsV1(const std::string& json, std::string& error)
{
    std::string s = trim_copy(json);
    if (s.empty()) { error = "Empty ops"; return false; }
    // Remember original ops text for sharing
    m_opsHistory.push_back(json);
    std::vector<std::string> objects;
    if (s[0] == '[') {
        size_t pos = 0; while (true) {
            auto l = s.find('{', pos); if (l==std::string::npos) break; int d=1; size_t r=l+1; while (r<s.size() && d>0){ if (s[r]=='{') d++; else if (s[r]=='}') d--; r++; }
            if (d!=0) { error = "Unbalanced braces"; return false; }
            objects.push_back(s.substr(l, r-l)); pos = r;
        }
    } else if (s[0] == '{') {
        objects.push_back(s);
    } else { error = "Top-level must be object or array"; return false; }

    for (const auto& obj : objects) {
        std::string op; if (!findString(obj, "op", op)) { error = "Missing op"; return false; }
        if (op == "load") {
            std::string path; if (!findString(obj, "path", path)) { error = "load: missing path"; return false; }
            std::string name; findString(obj, "name", name);
            glm::vec3 pos(0), scl(1), rot(0);
            // parse transform
            auto tpos = obj.find("\"transform\""); if (tpos != std::string::npos){ auto l=obj.find('{',tpos); auto r=obj.find('}',l); if (l!=std::string::npos&&r!=std::string::npos){ std::string t = obj.substr(l, r-l+1); findVec3(t, "position", pos); findVec3(t, "scale", scl); if (!findVec3(t, "rotation_deg", rot)) findVec3(t, "rotation", rot); }}
            if (name.empty()) name = path;
            bool ok = loadObjAt(name, path, pos, scl);
            if (!ok) { error = std::string("Failed to load: ")+path; return false; }
            // apply rotation about model center
            for (auto& o : m_sceneObjects) if (o.name == name) {
                glm::vec3 minB = o.objLoader.getMinBounds(), maxB = o.objLoader.getMaxBounds(); glm::vec3 center = (minB+maxB)*0.5f;
                glm::mat4 M = glm::mat4(1.0f);
                M = glm::translate(M, pos);
                M = glm::rotate(M, glm::radians(rot.x), glm::vec3(1,0,0));
                M = glm::rotate(M, glm::radians(rot.y), glm::vec3(0,1,0));
                M = glm::rotate(M, glm::radians(rot.z), glm::vec3(0,0,1));
                M = M * glm::scale(glm::mat4(1.0f), scl) * glm::translate(glm::mat4(1.0f), -center);
                o.modelMatrix = M; break;
            }
        }
        else if (op == "duplicate") {
            std::string source; if (!findString(obj, "source", source)) { error = "duplicate: missing source"; return false; }
            std::string name; if (!findString(obj, "name", name)) name = source + std::string("_copy");
            glm::vec3 dpos(0), dscale(1), drot(0);
            auto tpos2 = obj.find("\"transform_delta\""); if (tpos2 == std::string::npos) tpos2 = obj.find("\"transform\"");
            if (tpos2 != std::string::npos){ auto l=obj.find('{',tpos2); auto r=obj.find('}',l); if (l!=std::string::npos&&r!=std::string::npos){ std::string t = obj.substr(l, r-l+1); findVec3(t, "position", dpos); findVec3(t, "scale", dscale); if (!findVec3(t, "rotation_deg", drot)) findVec3(t, "rotation", drot);} }
            if (!duplicateObject(source, name, &dpos, &dscale, &drot)) { error = "duplicate failed"; return false; }
        }
        else if (op == "transform") {
            std::string target; if (!findString(obj, "target", target)) { error = "transform: missing target"; return false; }
            std::string mode; findString(obj, "mode", mode); bool isDelta = (mode == "delta");
            glm::vec3 pos, scale, rot; bool hasPos=false, hasScale=false, hasRot=false;
            auto tkey = isDelta ? "transform_delta" : "transform"; auto tp = obj.find(std::string("\"")+tkey+"\""); if (tp!=std::string::npos){ auto l=obj.find('{',tp); auto r=obj.find('}',l); if (l!=std::string::npos&&r!=std::string::npos){ std::string t = obj.substr(l, r-l+1); hasPos = findVec3(t, "position", pos); hasScale = findVec3(t, "scale", scale); hasRot = (findVec3(t, "rotation_deg", rot) || findVec3(t, "rotation", rot)); }}
            for (auto& o : m_sceneObjects) if (o.name == target) {
                if (isDelta) {
                    if (hasPos) o.modelMatrix = glm::translate(o.modelMatrix, pos);
                    if (hasScale) o.modelMatrix = o.modelMatrix * glm::scale(glm::mat4(1.0f), scale);
                    if (hasRot) { glm::mat4 R(1.0f); R=glm::rotate(R, glm::radians(rot.x), {1,0,0}); R=glm::rotate(R, glm::radians(rot.y), {0,1,0}); R=glm::rotate(R, glm::radians(rot.z), {0,0,1}); o.modelMatrix = o.modelMatrix * R; }
                } else {
                    glm::vec3 minB = o.objLoader.getMinBounds(), maxB = o.objLoader.getMaxBounds(); glm::vec3 center = (minB+maxB)*0.5f;
                    glm::mat4 M(1.0f);
                    glm::vec3 P = hasPos ? pos : glm::vec3(0);
                    glm::vec3 S = hasScale ? scale : glm::vec3(1);
                    glm::vec3 Rdeg = hasRot ? rot : glm::vec3(0);
                    M = glm::translate(M, P);
                    M = glm::rotate(M, glm::radians(Rdeg.x), {1,0,0});
                    M = glm::rotate(M, glm::radians(Rdeg.y), {0,1,0});
                    M = glm::rotate(M, glm::radians(Rdeg.z), {0,0,1});
                    M = M * glm::scale(glm::mat4(1.0f), S) * glm::translate(glm::mat4(1.0f), -center);
                    o.modelMatrix = M;
                }
                break;
            }
        }
        else if (op == "set_material") {
            std::string target; if (!findString(obj, "target", target)) { error = "set_material: missing target"; return false; }
            std::string matName; bool byName = findString(obj, "material_name", matName);
            if (byName) { assignMaterialToObject(target, matName); }
            else {
                glm::vec3 color, spec, ambient; float shin=0, rough=0, metal=0; bool hc=false, hs=false, ha=false, hsh=false, hr=false, hm=false;
                auto mp = obj.find("\"material\""); if (mp!=std::string::npos){ auto l=obj.find('{',mp); auto r=obj.find('}',l); if (l!=std::string::npos&&r!=std::string::npos){ std::string m = obj.substr(l, r-l+1); hc=findVec3(m, "color", color); hs=findVec3(m, "specular", spec); ha=findVec3(m, "ambient", ambient); hsh=findNumber(m, "shininess", shin); hr=findNumber(m, "roughness", rough); hm=findNumber(m, "metallic", metal);} }
                for (auto& o : m_sceneObjects) if (o.name == target) {
                    if (hc) o.material.diffuse = color; if (hs) o.material.specular = spec; if (ha) o.material.ambient = ambient; if (hsh) o.material.shininess = shin; if (hr) o.material.roughness = rough; if (hm) o.material.metallic = metal; break;
                }
            }
        }
        else if (op == "add_light") {
            std::string type; findString(obj, "type", type); if (type.empty()) type = "point";
            glm::vec3 pos(0), dir(0), col(1); float intensity = 1.0f; bool hasPos = findVec3(obj, "position", pos); findVec3(obj, "direction", dir); findVec3(obj, "color", col); findNumber(obj, "intensity", intensity);
            if (!hasPos) pos = getCameraPosition() + glm::normalize(getCameraFront()) * 2.0f;
            if (type == "point") addPointLightAt(pos, col, intensity);
            else /*directional*/ addPointLightAt(-glm::normalize(dir)*10.0f, col, intensity);
        }
        else if (op == "set_camera") {
            glm::vec3 pos(0), target(0), front(0), up(0,1,0); bool hp=findVec3(obj, "position", pos); bool ht=findVec3(obj, "target", target); bool hf=findVec3(obj, "front", front); findVec3(obj, "up", up);
            float fov=0, nz=0, fz=0; findNumber(obj, "fov_deg", fov); findNumber(obj, "near", nz); findNumber(obj, "far", fz);
            if (hp && ht) setCameraTarget(pos, target, up); else if (hp && hf) setCameraFrontUp(pos, front, up);
            setCameraLens(fov, nz, fz);
        }
        else if (op == "select") {
            std::string type; findString(obj, "type", type);
            if (type == "light") {
                float idxF = -1.0f; if (!findNumber(obj, "index", idxF)) idxF = -1.0f;
                int idx = (int)idxF;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    setSelectedLightIndex(idx);
                }
            } else { // object by name
                std::string name; if (findString(obj, "target", name)) {
                    int found = -1;
                    for (int i = 0; i < (int)m_sceneObjects.size(); ++i) if (m_sceneObjects[i].name == name) { found = i; break; }
                    if (found >= 0) { setSelectedObjectIndex(found); setSelectedLightIndex(-1); }
                }
            }
        }
        else if (op == "remove") {
            std::string type; findString(obj, "type", type);
            bool ok = false;
            if (type == "light") {
                float idxF = -1.0f; if (findNumber(obj, "index", idxF)) ok = removeLightAtIndex((int)idxF);
                else if (m_selectedLightIndex >= 0) ok = removeLightAtIndex(m_selectedLightIndex);
            } else { // object
                std::string name; if (findString(obj, "target", name)) ok = removeObjectByName(name);
                else if (m_selectedObjectIndex >= 0) ok = removeObjectByName(m_sceneObjects[(size_t)m_selectedObjectIndex].name);
            }
            if (!ok) { error = "remove failed"; return false; }
        }
        else if (op == "set_light") {
            // target light by index (or selected)
            int idx = m_selectedLightIndex;
            float idxF= -1.0f; if (findNumber(obj, "index", idxF)) idx = (int)idxF;
            if (idx < 0 || idx >= (int)m_lights.m_lights.size()) { error = "set_light: invalid index"; return false; }
            auto& L = m_lights.m_lights[(size_t)idx];
            bool b=false; if (findBool(obj, "enabled", b)) L.enabled = b;
            glm::vec3 c; if (findVec3(obj, "color", c)) L.color = c;
            float I=0; if (findNumber(obj, "intensity", I)) L.intensity = I;
            glm::vec3 p; if (findVec3(obj, "position", p)) L.position = p;
        }
        else if (op == "duplicate_light") {
            int idx = m_selectedLightIndex;
            float idxF=-1.0f; if (findNumber(obj, "index", idxF)) idx = (int)idxF;
            if (idx < 0 || idx >= (int)m_lights.m_lights.size()) { error = "duplicate_light: invalid index"; return false; }
            auto L = m_lights.m_lights[(size_t)idx];
            addPointLightAt(L.position + glm::vec3(0.2f,0,0), L.color, L.intensity);
        }
        else if (op == "set_render_mode") {
            int mode = -1; float mnum = -1.0f; if (findNumber(obj, "mode", mnum)) mode = int(mnum + 0.5f);
            if (mode < 0) {
                std::string mstr; if (findString(obj, "mode", mstr)) {
                    std::string s = mstr; for (auto& c : s) c = char(std::tolower((unsigned char)c));
                    if (s=="point" || s=="points" || s=="pointcloud") mode = 0;
                    else if (s=="wire" || s=="wireframe") mode = 1;
                    else if (s=="solid" || s=="shaded") mode = 2;
                    else if (s=="raytrace" || s=="rt" || s=="ray") mode = 3;
                }
            }
            if (mode >= 0 && mode <= 3) m_renderMode = mode; else { error = "set_render_mode: invalid mode"; return false; }
        }
        else if (op == "set_gizmo") {
            std::string mstr; if (findString(obj, "mode", mstr)) {
                std::string s = mstr; for (auto& c : s) c = char(std::tolower((unsigned char)c));
                if (s=="translate" || s=="move") m_gizmoMode = GizmoMode::Translate;
                else if (s=="rotate") m_gizmoMode = GizmoMode::Rotate;
                else if (s=="scale") m_gizmoMode = GizmoMode::Scale;
            }
            std::string astr; if (findString(obj, "axis", astr)) {
                std::string s = astr; for (auto& c : s) c = char(std::tolower((unsigned char)c));
                if (s=="x") m_gizmoAxis = GizmoAxis::X;
                else if (s=="y") m_gizmoAxis = GizmoAxis::Y;
                else if (s=="z") m_gizmoAxis = GizmoAxis::Z;
                else if (s=="none") m_gizmoAxis = GizmoAxis::None;
            }
            bool b=false; if (findBool(obj, "local", b)) m_gizmoLocalSpace = b; if (findBool(obj, "snap", b)) m_snapEnabled = b;
        }
        else if (op == "fullscreen") {
            bool t=false, on=false; bool hasToggle = findBool(obj, "toggle", t); bool hasOn = findBool(obj, "on", on);
            if (hasToggle && t) { toggleFullscreen(); }
            else if (hasOn) { if (on != m_fullscreen) toggleFullscreen(); }
            else { toggleFullscreen(); }
        }
        else if (op == "render") {
            // no-op here; handled by CLI flags
        }
        else { error = std::string("Unknown op: ")+op; return false; }
    }
    return true;
}
