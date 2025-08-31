#include "ai_bridge.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")

// Very small helper to find a JSON field value in a flat object (string field)
static bool find_json_string_field(const std::string& body, const std::string& key, std::string& out) {
    auto pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return false;
    pos = body.find('"', pos);
    if (pos == std::string::npos) return false;
    auto end = pos + 1;
    std::string s;
    while (end < body.size()) {
        char c = body[end++];
        if (c == '\\') { if (end < body.size()) { s.push_back(body[end++]); } }
        else if (c == '"') { break; }
        else { s.push_back(c); }
    }
    out = s;
    return true;
}

static std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], wlen);
    return w;
}

// Build strict instructions to force JSON-only output following our micro-DSL
static std::string build_instructions() {
    std::ostringstream os;
    os << "You convert natural language into a strict JSON command batch for a 3D app.\n";
    os << "Output ONLY JSON. No prose, no markdown.\n";
    os << "Schema: an object or array of objects with fields:\n";
    os << "{ \"op\": one of [load_model, duplicate, add_light], ... }\n";
    os << "load_model: { op, path, name?, transform?{ position:[x,y,z], scale:[x,y,z], rotation:[x,y,z] } }\n";
    os << "duplicate:  { op, source, name?, transform?{ position:[dx,dy,dz], scale? } }\n";
    os << "add_light:  { op, type: 'point'|'directional', position?, direction?, color?, intensity? }\n";
    os << "Vectors are arrays of 3 numbers.\n";
    os << "Examples:\n";
    os << "User: load cow.obj and move it right 2\n";
    os << "[{\"op\":\"load_model\",\"path\":\"cow.obj\",\"name\":\"Cow1\",\"transform\":{\"position\":[2,0,0]}}]\n";
    os << "User: duplicate Cow1 left 1 and add a light above\n";
    os << "[{\"op\":\"duplicate\",\"source\":\"Cow1\",\"name\":\"Cow2\",\"transform\":{\"position\":[-1,0,0]}},{\"op\":\"add_light\",\"type\":\"point\",\"position\":[0,5,0]}]\n";
    return os.str();
}

static std::string build_planner_instructions() {
    std::ostringstream os;
    os << "You are a scene assistant for a 3D app.\n";
    os << "Given USER instruction and SCENE JSON, output a plan as imperative commands, one per line.\n";
    os << "No explanations, no markdown, only commands.\n";
    os << "Use object names from the scene where helpful.\n";
    os << "Supported commands (free-form, natural but structured):\n";
    os << "- place <name> [in front of me <d>] | [at x y z] [scale sx sy sz]\n";
    os << "- place <count> <name> objects [scale sx sy sz | long | flat | tall | wide] [arrange into three walls and one floor]\n";
    os << "- add light [at x y z] [color r g b] [intensity v]\n";
    os << "- create material <name> [color r g b] [specular r g b] [ambient r g b] [shininess s] [roughness r] [metallic m]\n";
    os << "- assign material <mat> to <object>\n";
    os << "- fullscreen\n";
    os << "Examples:\n";
    os << "USER: put the cow 3 meters ahead and add a soft light above\n";
    os << "place cow in front of me 3\nadd light at 0 5 0 intensity 0.6\n";
    os << "USER: make a wood material and apply it to Cow1\n";
    os << "create material wood color 0.6 0.4 0.2 roughness 0.8\nassign material wood to Cow1\n";
    return os.str();
}

bool NLToJSONBridge::translate(const std::string& natural, std::string& outJson, std::string& error) {
    // Target Ollama: POST /api/generate with {model, system, prompt, stream:false}
    // Endpoint expected as http://host:port
    std::string host = "127.0.0.1";
    INTERNET_PORT port = 11434;
    // Basic parse if user customized endpoint
    if (m_cfg.endpoint.rfind("http://", 0) == 0) {
        auto rest = m_cfg.endpoint.substr(7);
        auto colon = rest.find(':');
        if (colon != std::string::npos) {
            host = rest.substr(0, colon);
            port = static_cast<INTERNET_PORT>(std::stoi(rest.substr(colon + 1)));
        } else {
            host = rest;
        }
    }

    std::ostringstream body;
    body << "{\n";
    body << "  \"model\": \"" << m_cfg.model << "\",\n";
    // /api/generate has no 'system'; prepend instructions into the prompt
    std::string prompt = build_instructions() + std::string("\n\nUser: ") + natural + "\nJSON:";
    std::string esc; esc.reserve(prompt.size()+8);
    for (char c : prompt) { if (c=='"' || c=='\\') { esc.push_back('\\'); esc.push_back(c);} else if (c=='\n'){ esc += "\\n"; } else esc.push_back(c);}    
    body << "  \"prompt\": \"" << esc << "\",\n";
    body << "  \"format\": \"json\",\n";
    body << "  \"options\": { \"temperature\": 0 },\n";
    body << "  \"stream\": false\n";
    body << "}";
    auto bodyStr = body.str();

    HINTERNET hSession = WinHttpOpen(L"OBJViewer/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { error = "WinHttpOpen failed"; return false; }
    HINTERNET hConnect = WinHttpConnect(hSession, widen(host).c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); error = "WinHttpConnect failed"; return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/generate", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpOpenRequest failed"; return false; }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1, (LPVOID)bodyStr.data(), (DWORD)bodyStr.size(), (DWORD)bodyStr.size(), 0);
    if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpSendRequest failed"; return false; }
    BOOL recvOK = WinHttpReceiveResponse(hRequest, nullptr);
    if (!recvOK) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpReceiveResponse failed"; return false; }

    std::string response;
    DWORD dwSize = 0;
    do {
        DWORD dwDownloaded = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buf(dwSize);
        if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwDownloaded)) break;
        response.append(buf.data(), dwDownloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // The Ollama /api/generate returns JSON with a "response" string field containing the model output.
    std::string modelOut;
    if (!find_json_string_field(response, "response", modelOut)) {
        std::string errText;
        if (find_json_string_field(response, "error", errText)) {
            error = std::string("Ollama error: ") + errText;
        } else {
            error = "No 'response' field in Ollama output";
        }
        return false;
    }
    // Some models may wrap in code fences; strip if present
    auto start = modelOut.find('{');
    if (start == std::string::npos) start = modelOut.find('[');
    if (start != std::string::npos) {
        // trim around inferred JSON segment
        auto s = modelOut.substr(start);
        auto lastBrace = s.find_last_of('}');
        auto lastBracket = s.find_last_of(']');
        size_t end = std::string::npos;
        if (lastBrace != std::string::npos) end = lastBrace + 1;
        if (lastBracket != std::string::npos) end = std::max(end, lastBracket + 1);
        if (end != std::string::npos) modelOut = s.substr(0, end);
    }
    outJson = modelOut;
    return true;
}

bool AIPlanner::plan(const std::string& natural,
                     const std::string& sceneJson,
                     std::string& outPlan,
                     std::string& error) {
    std::string host = "127.0.0.1";
    INTERNET_PORT port = 11434;
    if (m_cfg.endpoint.rfind("http://", 0) == 0) {
        auto rest = m_cfg.endpoint.substr(7);
        auto colon = rest.find(':');
        if (colon != std::string::npos) { host = rest.substr(0, colon); port = static_cast<INTERNET_PORT>(std::stoi(rest.substr(colon + 1))); }
        else { host = rest; }
    }

    std::string sys = build_planner_instructions();
    std::ostringstream prompt;
    prompt << sys << "\nSCENE JSON:\n" << sceneJson << "\nUSER:\n" << natural << "\nPLAN:";

    auto esc_json = [](const std::string& s){ std::string r; r.reserve(s.size()+8); for(char c: s){ if(c=='"'||c=='\\'){ r.push_back('\\'); r.push_back(c);} else if(c=='\n'){ r += "\\n"; } else r.push_back(c);} return r; };

    std::ostringstream body;
    body << "{\n";
    body << "  \"model\": \"" << m_cfg.model << "\",\n";
    body << "  \"prompt\": \"" << esc_json(prompt.str()) << "\",\n";
    body << "  \"options\": { \"temperature\": 0 },\n";
    body << "  \"stream\": false\n";
    body << "}";
    auto bodyStr = body.str();

    HINTERNET hSession = WinHttpOpen(L"OBJViewer/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { error = "WinHttpOpen failed"; return false; }
    HINTERNET hConnect = WinHttpConnect(hSession, widen(host).c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); error = "WinHttpConnect failed"; return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/generate", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpOpenRequest failed"; return false; }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1, (LPVOID)bodyStr.data(), (DWORD)bodyStr.size(), (DWORD)bodyStr.size(), 0);
    if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpSendRequest failed"; return false; }
    BOOL recvOK = WinHttpReceiveResponse(hRequest, nullptr);
    if (!recvOK) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); error = "WinHttpReceiveResponse failed"; return false; }

    std::string response;
    DWORD dwSize = 0;
    do {
        DWORD dwDownloaded = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buf(dwSize);
        if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwDownloaded)) break;
        response.append(buf.data(), dwDownloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    std::string modelOut;
    if (!find_json_string_field(response, "response", modelOut)) {
        std::string errText;
        if (find_json_string_field(response, "error", errText)) error = std::string("Ollama error: ") + errText; else error = "No 'response' field in Ollama output";
        return false;
    }
    // Strip potential code fences/markdown; keep plain text lines
    // Heuristic: if triple backticks exist, extract inside
    auto fence = modelOut.find("```\n");
    if (fence != std::string::npos) {
        auto rest = modelOut.substr(fence + 4);
        auto fence2 = rest.find("```\n");
        if (fence2 != std::string::npos) modelOut = rest.substr(0, fence2);
    }
    outPlan = modelOut;
    return true;
}
