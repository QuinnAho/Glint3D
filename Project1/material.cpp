#include "Material.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

void Material::apply(GLuint shaderProgram, const std::string& uniformName) const {
    glUniform3fv(glGetUniformLocation(shaderProgram, (uniformName + ".diffuse").c_str()), 1, glm::value_ptr(diffuse));
    glUniform3fv(glGetUniformLocation(shaderProgram, (uniformName + ".specular").c_str()), 1, glm::value_ptr(specular));
    glUniform3fv(glGetUniformLocation(shaderProgram, (uniformName + ".ambient").c_str()), 1, glm::value_ptr(ambient));
    glUniform1f(glGetUniformLocation(shaderProgram, (uniformName + ".shininess").c_str()), shininess);

    // Optional: for future PBR
    glUniform1f(glGetUniformLocation(shaderProgram, (uniformName + ".roughness").c_str()), roughness);
    glUniform1f(glGetUniformLocation(shaderProgram, (uniformName + ".metallic").c_str()), metallic);
}
