#include "GpuRaytracer.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

GpuRaytracer::GpuRaytracer() : m_program(0) {}

GpuRaytracer::~GpuRaytracer()
{
    if (m_program)
        glDeleteProgram(m_program);
}

static std::string loadTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader: " << path << "\n";
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool GpuRaytracer::init()
{
    std::string compSource = loadTextFile("shaders/raytrace.comp");
    if (compSource.empty())
        return false;

    GLuint comp = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = compSource.c_str();
    glShaderSource(comp, 1, &src, nullptr);
    glCompileShader(comp);

    GLint success = 0;
    glGetShaderiv(comp, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(comp, 512, nullptr, infoLog);
        std::cerr << "Compute shader compile error:\n" << infoLog << std::endl;
        glDeleteShader(comp);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, comp);
    glLinkProgram(m_program);
    glDeleteShader(comp);

    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "Compute shader link error:\n" << infoLog << std::endl;
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    return true;
}

void GpuRaytracer::render(GLuint outputTex,
                          int width,
                          int height,
                          const glm::vec3& camPos,
                          const glm::vec3& camFront,
                          const glm::vec3& camUp,
                          float fovDeg)
{
    if (!m_program)
        return;

    glUseProgram(m_program);

    glUniform3fv(glGetUniformLocation(m_program, "uCamPos"), 1, glm::value_ptr(camPos));
    glUniform3fv(glGetUniformLocation(m_program, "uCamFront"), 1, glm::value_ptr(camFront));
    glUniform3fv(glGetUniformLocation(m_program, "uCamUp"), 1, glm::value_ptr(camUp));
    glUniform1f(glGetUniformLocation(m_program, "uFov"), fovDeg);
    glUniform1f(glGetUniformLocation(m_program, "uAspect"), static_cast<float>(width) / static_cast<float>(height));

    glBindImageTexture(0, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    const GLuint groupsX = (width + 15) / 16;
    const GLuint groupsY = (height + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

