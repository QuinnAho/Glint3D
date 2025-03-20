#include "Light.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>

std::string intToString(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

Light::Light()
{
}

Light::~Light()
{
    if (m_indicatorVAO) glDeleteVertexArrays(1, &m_indicatorVAO);
    if (m_indicatorVBO) glDeleteBuffers(1, &m_indicatorVBO);
    if (m_indicatorShader) glDeleteProgram(m_indicatorShader);
}

void Light::addLight(const glm::vec3& position, const glm::vec3& color, float intensity)
{
    m_lights.push_back({ position, color, intensity });
}

void Light::applyLights(GLuint shaderProgram) const
{
    glUseProgram(shaderProgram);

    GLint lightCountLoc = glGetUniformLocation(shaderProgram, "numLights");
    if (lightCountLoc != -1)
    {
        glUniform1i(lightCountLoc, static_cast<int>(m_lights.size()));
    }

    for (size_t i = 0; i < m_lights.size(); i++)
    {
        std::string baseName = "lights[" + std::to_string(i) + "]";
        GLint lightPosLoc = glGetUniformLocation(shaderProgram, (baseName + ".position").c_str());
        GLint lightColorLoc = glGetUniformLocation(shaderProgram, (baseName + ".color").c_str());
        GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, (baseName + ".intensity").c_str());

        static bool printedUniformError = false;
        if (!printedUniformError && (lightPosLoc == -1 || lightColorLoc == -1 || lightIntensityLoc == -1))
        {
            std::cerr << "Error: Failed to get uniform location for lights[" << i << "]\n";
            printedUniformError = true;
        }

        if (lightPosLoc != -1) glUniform3fv(lightPosLoc, 1, glm::value_ptr(m_lights[i].position));
        if (lightColorLoc != -1) glUniform3fv(lightColorLoc, 1, glm::value_ptr(m_lights[i].color));
        if (lightIntensityLoc != -1) glUniform1f(lightIntensityLoc, m_lights[i].intensity);
    }
}

size_t Light::getLightCount() const
{
    return m_lights.size();
}

// NEW: Initialize indicator geometry (a small cube)
void Light::initIndicator()
{
    float vertices[] = {
        // positions for 12 triangles (36 vertices)
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,

        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,

         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,

        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f
    };

    glGenVertexArrays(1, &m_indicatorVAO);
    glGenBuffers(1, &m_indicatorVBO);
    glBindVertexArray(m_indicatorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_indicatorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// NEW: Initialize the indicator shader within the Light class
bool Light::initIndicatorShader()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 indicatorColor;
        void main()
        {
            FragColor = vec4(indicatorColor, 1.0);
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Vertex Shader Compilation Error:\n" << infoLog << std::endl;
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Fragment Shader Compilation Error:\n" << infoLog << std::endl;
        return false;
    }

    // Link shader program
    m_indicatorShader = glCreateProgram();
    glAttachShader(m_indicatorShader, vertexShader);
    glAttachShader(m_indicatorShader, fragmentShader);
    glLinkProgram(m_indicatorShader);
    glGetProgramiv(m_indicatorShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_indicatorShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Shader Linking Error:\n" << infoLog << std::endl;
        return false;
    }

    // Clean up shaders (no longer needed once linked)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

// NEW: Render visual indicators for each light using the indicator shader stored in the Light class
void Light::renderIndicators(const glm::mat4& view, const glm::mat4& projection) const
{
    if (m_indicatorShader == 0)
        return; // Shader not initialized

    glUseProgram(m_indicatorShader);

    GLint viewLoc = glGetUniformLocation(m_indicatorShader, "view");
    GLint projLoc = glGetUniformLocation(m_indicatorShader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(m_indicatorVAO);
    for (size_t i = 0; i < m_lights.size(); i++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_lights[i].position);
        GLint modelLoc = glGetUniformLocation(m_indicatorShader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(m_indicatorShader, "indicatorColor");
        glUniform3fv(colorLoc, 1, glm::value_ptr(m_lights[i].color));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
}
