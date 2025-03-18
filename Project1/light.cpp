#include "Light.h"
#include <glm/gtc/type_ptr.hpp>

Light::Light()
    : m_position(0.0f, 3.0f, 5.0f),  // Default position
    m_color(1.0f, 1.0f, 1.0f),    // White light
    m_intensity(1.0f)             // Default intensity
{
}

Light::~Light()
{
}

void Light::setPosition(const glm::vec3& position)
{
    m_position = position;
}

void Light::setColor(const glm::vec3& color)
{
    m_color = color;
}

void Light::setIntensity(float intensity)
{
    m_intensity = intensity;
}

glm::vec3 Light::getPosition() const
{
    return m_position;
}

glm::vec3 Light::getColor() const
{
    return m_color;
}

float Light::getIntensity() const
{
    return m_intensity;
}

// Pass light properties to the shader
void Light::applyLight(GLuint shaderProgram) const
{
    glUseProgram(shaderProgram);

    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "light.position");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "light.color");
    GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, "light.intensity");

    glUniform3fv(lightPosLoc, 1, glm::value_ptr(m_position));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(m_color));
    glUniform1f(lightIntensityLoc, m_intensity);
}
