#include "Light.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>  // Add this at the top

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
}

// Add a new light source
void Light::addLight(const glm::vec3& position, const glm::vec3& color, float intensity)
{
    m_lights.push_back({ position, color, intensity });
}

// Send multiple lights to the shader
void Light::applyLights(GLuint shaderProgram) const
{
    glUseProgram(shaderProgram);

    // Get location of light count uniform once
    GLint lightCountLoc = glGetUniformLocation(shaderProgram, "numLights");
    if (lightCountLoc != -1)
    {
        glUniform1i(lightCountLoc, static_cast<int>(m_lights.size()));
    }

    // Apply all lights
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        std::string baseName = "lights[" + std::to_string(i) + "]";

        GLint lightPosLoc = glGetUniformLocation(shaderProgram, (baseName + ".position").c_str());
        GLint lightColorLoc = glGetUniformLocation(shaderProgram, (baseName + ".color").c_str());
        GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, (baseName + ".intensity").c_str());

        // Only print uniform errors once, not every frame
        static bool printedUniformError = false;
        if (!printedUniformError && (lightPosLoc == -1 || lightColorLoc == -1 || lightIntensityLoc == -1))
        {
            std::cerr << "Error: Failed to get uniform location for lights[" << i << "]\n";
            printedUniformError = true;
        }

        // Only set uniforms if they exist
        if (lightPosLoc != -1) glUniform3fv(lightPosLoc, 1, glm::value_ptr(m_lights[i].position));
        if (lightColorLoc != -1) glUniform3fv(lightColorLoc, 1, glm::value_ptr(m_lights[i].color));
        if (lightIntensityLoc != -1) glUniform1f(lightIntensityLoc, m_lights[i].intensity);
    }
}

size_t Light::getLightCount() const
{
    return m_lights.size();
}
