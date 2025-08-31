#include "compute_shader.h"
#include "gl43_compat.h"
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader() {}
ComputeShader::~ComputeShader() {
    if (m_programID) glDeleteProgram(m_programID);
}

std::string ComputeShader::loadFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return {};
    std::stringstream ss; ss << ifs.rdbuf();
    return ss.str();
}

GLuint ComputeShader::compile(GLenum type, const std::string& src) {
    GLuint sh = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; GLsizei n=0; glGetShaderInfoLog(sh, sizeof(log), &n, log);
        std::cerr << "[ComputeShader] Compile error: " << std::string(log, n) << "\n";
        glDeleteShader(sh); return 0;
    }
    return sh;
}

bool ComputeShader::load(const std::string& computePath) {
    std::string csrc = loadFile(computePath);
    if (csrc.empty()) {
        std::cerr << "[ComputeShader] Failed to read " << computePath << "\n";
        return false;
    }
    GLuint csh = compile(GL_COMPUTE_SHADER, csrc);
    if (!csh) return false;
    m_programID = glCreateProgram();
    glAttachShader(m_programID, csh);
    glLinkProgram(m_programID);
    glDeleteShader(csh);
    GLint ok = GL_FALSE; glGetProgramiv(m_programID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; GLsizei n=0; glGetProgramInfoLog(m_programID, sizeof(log), &n, log);
        std::cerr << "[ComputeShader] Link error: " << std::string(log, n) << "\n";
        glDeleteProgram(m_programID); m_programID=0; return false;
    }
    return true;
}

void ComputeShader::use() const { glUseProgram(m_programID); }

void ComputeShader::setInt(const std::string& name, int v) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    if (loc >= 0) glUniform1i(loc, v);
}
void ComputeShader::setFloat(const std::string& name, float v) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    if (loc >= 0) glUniform1f(loc, v);
}
void ComputeShader::setVec3(const std::string& name, const float x, const float y, const float z) const {
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    if (loc >= 0) glUniform3f(loc, x, y, z);
}
