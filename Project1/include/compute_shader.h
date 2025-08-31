#pragma once
#include <string>
#include "gl_platform.h"

class ComputeShader
{
public:
    ComputeShader();
    ~ComputeShader();

    bool load(const std::string& computePath);
    void use() const;
    GLuint getID() const { return m_programID; }

    // Minimal uniform helpers
    void setInt(const std::string& name, int v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec3(const std::string& name, const float x, const float y, const float z) const;

private:
    GLuint m_programID = 0;
    std::string loadFile(const std::string& path);
    GLuint compile(GLenum type, const std::string& src);
};
