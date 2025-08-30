#include "commands.h"

#include <algorithm>
#include <sstream>

// NOTE: Minimal, forgiving parser for the micro-DSL. Not a full JSON parser.
// Accepts objects or arrays of objects. Keys of interest are extracted by
// simple string scanning. Strings must be in double quotes. Numbers can be
// integer or float. Vectors: [x,y,z].

namespace {
    static inline std::string trim(std::string s) {
        auto not_space = [](int ch) { return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
        return s;
    }

    static bool findString(const std::string& src, const std::string& key, std::string& out) {
        auto pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = src.find(':', pos);
        if (pos == std::string::npos) return false;
        pos = src.find('"', pos);
        if (pos == std::string::npos) return false;
        auto end = src.find('"', pos + 1);
        if (end == std::string::npos) return false;
        out = src.substr(pos + 1, end - pos - 1);
        return true;
    }

    static bool findNumber(const std::string& src, const std::string& key, float& out) {
        auto pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = src.find(':', pos);
        if (pos == std::string::npos) return false;
        // read until comma or end or }
        auto end = pos + 1;
        while (end < src.size() && (std::isspace(static_cast<unsigned char>(src[end])) || src[end] == '"')) end++;
        std::string num;
        while (end < src.size() && (std::isdigit(static_cast<unsigned char>(src[end])) || src[end] == '-' || src[end] == '+' || src[end] == '.' || src[end] == 'e' || src[end] == 'E')) {
            num.push_back(src[end]);
            end++;
        }
        if (num.empty()) return false;
        try { out = std::stof(num); return true; } catch (...) { return false; }
    }

    static bool findVec3(const std::string& src, const std::string& key, glm::vec3& out) {
        auto pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = src.find(':', pos);
        if (pos == std::string::npos) return false;
        pos = src.find('[', pos);
        if (pos == std::string::npos) return false;
        auto end = src.find(']', pos);
        if (end == std::string::npos) return false;
        std::string inside = src.substr(pos + 1, end - pos - 1);
        std::stringstream ss(inside);
        float v[3] = {0,0,0};
        char ch;
        if (!(ss >> v[0])) return false;
        ss >> ch; // ,
        if (!(ss >> v[1])) return false;
        ss >> ch;
        if (!(ss >> v[2])) return false;
        out = glm::vec3(v[0], v[1], v[2]);
        return true;
    }

    static std::string escape(const std::string& s) {
        std::string o; o.reserve(s.size()+8);
        for (char c : s) {
            if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
            else if (c == '\n') { o += "\\n"; }
            else { o.push_back(c); }
        }
        return o;
    }
}

static CommandOp opFromString(const std::string& s) {
    if (s == "load_model") return CommandOp::LoadModel;
    if (s == "duplicate")  return CommandOp::Duplicate;
    return CommandOp::AddLight; // default
}

static const char* opToString(CommandOp op) {
    switch (op) {
    case CommandOp::LoadModel: return "load_model";
    case CommandOp::Duplicate: return "duplicate";
    case CommandOp::AddLight:  return "add_light";
    }
    return "unknown";
}

static void serializeTransform(const CmdTransform& t, std::ostream& os) {
    bool first = true;
    if (t.position) { os << (first?"":" ,") << "\n      \"position\": [" << t.position->x << ", " << t.position->y << ", " << t.position->z << "]"; first=false; }
    if (t.scale)    { os << (first?"":" ,") << "\n      \"scale\": ["    << t.scale->x    << ", " << t.scale->y    << ", " << t.scale->z    << "]"; first=false; }
    if (t.rotationEuler) { os << (first?"":" ,") << "\n      \"rotation\": [" << t.rotationEuler->x << ", " << t.rotationEuler->y << ", " << t.rotationEuler->z << "]"; }
}

std::string ToJson(const CommandBatch& batch) {
    std::ostringstream os;
    os << "[\n";
    for (size_t i = 0; i < batch.commands.size(); ++i) {
        const auto& c = batch.commands[i];
        os << "  {\n    \"op\": \"" << opToString(c.op) << "\"";
        switch (c.op) {
        case CommandOp::LoadModel:
            os << ",\n    \"path\": \"" << escape(c.loadModel.path) << "\"";
            if (c.loadModel.name) os << ",\n    \"name\": \"" << escape(*c.loadModel.name) << "\"";
            if (c.loadModel.transform.position || c.loadModel.transform.scale || c.loadModel.transform.rotationEuler) {
                os << ",\n    \"transform\": {";
                serializeTransform(c.loadModel.transform, os);
                os << "\n    }";
            }
            break;
        case CommandOp::Duplicate:
            os << ",\n    \"source\": \"" << escape(c.duplicate.source) << "\"";
            if (c.duplicate.name) os << ",\n    \"name\": \"" << escape(*c.duplicate.name) << "\"";
            if (c.duplicate.transform.position || c.duplicate.transform.scale || c.duplicate.transform.rotationEuler) {
                os << ",\n    \"transform\": {";
                serializeTransform(c.duplicate.transform, os);
                os << "\n    }";
            }
            break;
        case CommandOp::AddLight:
            os << ",\n    \"type\": \"" << escape(c.addLight.type) << "\"";
            if (c.addLight.position)  os << ",\n    \"position\": ["  << c.addLight.position->x  << ", " << c.addLight.position->y  << ", " << c.addLight.position->z  << "]";
            if (c.addLight.direction) os << ",\n    \"direction\": [" << c.addLight.direction->x << ", " << c.addLight.direction->y << ", " << c.addLight.direction->z << "]";
            if (c.addLight.color)     os << ",\n    \"color\": ["     << c.addLight.color->x     << ", " << c.addLight.color->y     << ", " << c.addLight.color->z     << "]";
            if (c.addLight.intensity) os << ",\n    \"intensity\": " << *c.addLight.intensity;
            break;
        }
        os << "\n  }" << (i + 1 < batch.commands.size() ? ",\n" : "\n");
    }
    os << "]";
    return os.str();
}

static bool parseTransform(const std::string& src, CmdTransform& t) {
    glm::vec3 v;
    if (findVec3(src, "position", v)) t.position = v;
    if (findVec3(src, "scale", v))    t.scale    = v;
    if (findVec3(src, "rotation", v)) t.rotationEuler = v;
    return true; // optional fields only
}

static bool parseSingleCommand(const std::string& obj, Command& out, std::string& error) {
    std::string opStr;
    if (!findString(obj, "op", opStr)) { error = "Missing 'op'"; return false; }
    out.op = opFromString(opStr);
    switch (out.op) {
    case CommandOp::LoadModel: {
        if (!findString(obj, "path", out.loadModel.path)) { error = "load_model: missing 'path'"; return false; }
        std::string name; if (findString(obj, "name", name)) out.loadModel.name = name;
        // optional transform sub-object: parse within the slice starting at "transform"
        auto tpos = obj.find("\"transform\"");
        if (tpos != std::string::npos) {
            auto l = obj.find('{', tpos);
            auto r = obj.find('}', l);
            if (l != std::string::npos && r != std::string::npos) {
                parseTransform(obj.substr(l, r - l + 1), out.loadModel.transform);
            }
        }
        break; }
    case CommandOp::Duplicate: {
        if (!findString(obj, "source", out.duplicate.source)) { error = "duplicate: missing 'source'"; return false; }
        std::string name; if (findString(obj, "name", name)) out.duplicate.name = name;
        auto tpos = obj.find("\"transform\"");
        if (tpos != std::string::npos) {
            auto l = obj.find('{', tpos);
            auto r = obj.find('}', l);
            if (l != std::string::npos && r != std::string::npos) {
                parseTransform(obj.substr(l, r - l + 1), out.duplicate.transform);
            }
        }
        break; }
    case CommandOp::AddLight: {
        if (!findString(obj, "type", out.addLight.type)) out.addLight.type = "point";
        glm::vec3 v;
        if (findVec3(obj, "position", v))  out.addLight.position  = v;
        if (findVec3(obj, "direction", v)) out.addLight.direction = v;
        if (findVec3(obj, "color", v))     out.addLight.color     = v;
        float f;
        if (findNumber(obj, "intensity", f)) out.addLight.intensity = f;
        break; }
    }
    return true;
}

bool ParseCommandBatch(const std::string& json, CommandBatch& out, std::string& error) {
    out.commands.clear();
    std::string s = trim(json);
    if (s.empty()) { error = "Empty input"; return false; }
    if (s[0] == '[') {
        // naive split top-level objects by '{' ... '}'
        size_t pos = 0;
        while (true) {
            auto l = s.find('{', pos);
            if (l == std::string::npos) break;
            int depth = 1; size_t r = l + 1;
            while (r < s.size() && depth > 0) {
                if (s[r] == '{') depth++;
                else if (s[r] == '}') depth--;
                r++;
            }
            if (depth != 0) { error = "Unbalanced braces in array"; return false; }
            std::string obj = s.substr(l, r - l);
            Command cmd;
            if (!parseSingleCommand(obj, cmd, error)) return false;
            out.commands.push_back(std::move(cmd));
            pos = r;
        }
        if (out.commands.empty()) { error = "No commands found"; return false; }
        return true;
    }
    else if (s[0] == '{') {
        Command cmd;
        if (!parseSingleCommand(s, cmd, error)) return false;
        out.commands.push_back(std::move(cmd));
        return true;
    }
    error = "Input must be an object or array";
    return false;
}

