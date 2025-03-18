#ifndef LIGHT_H
#define LIGHT_H

#include <glad/glad.h>
#include <glm/glm.hpp>

class Light
{
public:
    Light();
    ~Light();

    // Set Light Position
    void setPosition(const glm::vec3& position);

    // Set Light Color
    void setColor(const glm::vec3& color);

    // Set Light Intensity
    void setIntensity(float intensity);

    // Send Light data to shader
    void applyLight(GLuint shaderProgram) const;

    // Get Light Position
    glm::vec3 getPosition() const;

    // Get Light Color
    glm::vec3 getColor() const;

    // Get Light Intensity
    float getIntensity() const;

private:
    glm::vec3 m_position;
    glm::vec3 m_color;
    float m_intensity;
};

#endif // LIGHT_H
