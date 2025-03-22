#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader() : m_programID(0) {}

Shader::~Shader()
{
    if (m_programID)
        glDeleteProgram(m_programID);
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexCode = loadShaderFromFile(vertexPath);
    std::string fragmentCode = loadShaderFromFile(fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) return false;

    GLuint vert = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    GLuint frag = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    if (!vert || !frag) return false;

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vert);
    glAttachShader(m_programID, frag);
    glLinkProgram(m_programID);

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::cerr << "Shader Link Error:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
}

void Shader::use() const
{
    glUseProgram(m_programID);
}

GLuint Shader::getID() const
{
    return m_programID;
}

std::string Shader::loadShaderFromFile(const std::string& path)
{
    std::ofstream logFile("shader_log.txt", std::ios::app); // Append mode

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open shader: " << path << std::endl;
        if (logFile.is_open())
            logFile << "❌ Failed to open shader: " << path << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderCode = buffer.str();

    std::cerr << "✅ Opened shader file: " << path << std::endl;
    if (logFile.is_open()) {
        logFile << "✅ Opened shader file: " << path << "\n";
        logFile << "----- Shader Code Start -----\n";
        logFile << shaderCode << "\n";
        logFile << "----- Shader Code End -----\n\n";
    }

    return shaderCode;
}


GLuint Shader::compileShader(const std::string& source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compile Error:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// Uniform helpers
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const
{
    glUniform3fv(glGetUniformLocation(m_programID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_programID, name.c_str()), (int)value);
}
