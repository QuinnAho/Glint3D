#include "application.h"
#include <sstream>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// Serialize current scene to a compact JSON snapshot
std::string Application::sceneToJson() const
{
    std::ostringstream os;
    os << "{\n";
    os << "  \"camera\": { \"position\": [" << m_cameraPos.x << ", " << m_cameraPos.y << ", " << m_cameraPos.z
       << "], \"front\": [" << m_cameraFront.x << ", " << m_cameraFront.y << ", " << m_cameraFront.z << "] },\n";
    os << "  \"selected\": ";
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size())
        os << "\"" << m_sceneObjects[m_selectedObjectIndex].name << "\"";
    else
        os << "null";
    os << ",\n";
    os << "  \"selectedLightIndex\": " << m_selectedLightIndex << ",\n";

    os << "  \"objects\": [\n";
    for (size_t i = 0; i < m_sceneObjects.size(); ++i) {
        const auto& o = m_sceneObjects[i];
        os << "    { \"name\": \"" << o.name << "\", \"material\": {"
              " \"diffuse\": [" << o.material.diffuse.x << ", " << o.material.diffuse.y << ", " << o.material.diffuse.z << "],"
              " \"specular\": [" << o.material.specular.x << ", " << o.material.specular.y << ", " << o.material.specular.z << "],"
              " \"ambient\": [" << o.material.ambient.x << ", " << o.material.ambient.y << ", " << o.material.ambient.z << "],"
              " \"shininess\": " << o.material.shininess << ", \"roughness\": " << o.material.roughness << ", \"metallic\": " << o.material.metallic << " } }";
        if (i + 1 < m_sceneObjects.size()) os << ",";
        os << "\n";
    }
    os << "  ],\n";

    os << "  \"lights\": [\n";
    for (size_t i = 0; i < m_lights.m_lights.size(); ++i) {
        const auto& L = m_lights.m_lights[i];
        os << "    { \"position\": [" << L.position.x << ", " << L.position.y << ", " << L.position.z << "],"
              " \"color\": [" << L.color.x << ", " << L.color.y << ", " << L.color.z << "], \"intensity\": " << L.intensity << " }";
        if (i + 1 < m_lights.m_lights.size()) os << ",";
        os << "\n";
    }
    os << "  ]\n";
    os << "}\n";
    return os.str();
}

namespace {
    static std::string b64encode(const std::string& in) {
        static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out; out.reserve(((in.size()+2)/3)*4);
        size_t i=0; unsigned char a3[3];
        while (i < in.size()) {
            size_t len = 0; for (; len<3 && i<in.size(); ++len,++i) a3[len] = (unsigned char)in[i];
            if (len==3) {
                out.push_back(tbl[(a3[0] & 0xfc) >> 2]);
                out.push_back(tbl[((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4)]);
                out.push_back(tbl[((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6)]);
                out.push_back(tbl[a3[2] & 0x3f]);
            } else if (len==2) {
                out.push_back(tbl[(a3[0] & 0xfc) >> 2]);
                out.push_back(tbl[((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4)]);
                out.push_back(tbl[((a3[1] & 0x0f) << 2)]);
                out.push_back('=');
            } else if (len==1) {
                out.push_back(tbl[(a3[0] & 0xfc) >> 2]);
                out.push_back(tbl[((a3[0] & 0x03) << 4)]);
                out.push_back('=');
                out.push_back('=');
            }
        }
        return out;
    }
}

// Encode ops history into a shareable ?state= URL (web) or query piece (desktop)
std::string Application::buildShareLink() const
{
    std::ostringstream body;
    body << "{\"version\":\"1.0\",\"ops\":[";
    for (size_t i = 0; i < m_opsHistory.size(); ++i) {
        body << m_opsHistory[i];
        if (i + 1 < m_opsHistory.size()) body << ",";
    }
    body << "]}";
    std::string payload = b64encode(body.str());

#ifdef __EMSCRIPTEN__
    std::string url;
    const char* href = emscripten_run_script_string("(function(){var u=new URL(window.location.href);return (u.origin+u.pathname);})()");
    if (href && *href) url = href; else url = std::string("./");
    url += std::string("?state=") + payload;
    return url;
#else
    return std::string("?state=") + payload;
#endif
}
