#ifndef LIGHT_H
#define LIGHT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct LightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
};

class Light
{
public:
    Light();
    ~Light();

    // Add a new light source
    void addLight(const glm::vec3& position, const glm::vec3& color, float intensity);

    // Send all lights to shader
    void applyLights(GLuint shaderProgram) const;

    // Get the number of lights
    size_t getLightCount() const;

private:
    std::vector<LightData> m_lights;
};

#endif // LIGHT_H
