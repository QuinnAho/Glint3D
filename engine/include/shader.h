#pragma once
#include <string>
#include "gl_platform.h"
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

// Forward declaration
namespace glint3d { class RHI; }

class Shader
{
public:
    Shader();
    ~Shader();

    bool load(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromStrings(const std::string& vertexSource, const std::string& fragmentSource);
    void use() const;

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;
    void setBool(const std::string& name, bool value) const;

    GLuint getID() const;

    // RHI bridge for transitional period
    static void setRHI(glint3d::RHI* rhi);

private:
    GLuint m_programID;
    static glint3d::RHI* s_rhi; // Static RHI reference for uniform bridging

    std::string loadShaderFromFile(const std::string& path);
    GLuint compileShader(const std::string& source, GLenum type);
};
