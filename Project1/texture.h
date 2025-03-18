#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    // Load texture from file
    bool loadFromFile(const std::string& filepath);

    // Activate and bind the texture
    void bind(GLuint unit = 0) const;

    // Initialize shaders
    bool initShaders();

    // Get the Shader Program ID
    GLuint getShaderProgram() const { return m_shaderProgram; }

private:
    GLuint m_textureID;
    GLuint m_shaderProgram;

    // Shader loading helpers
    GLuint compileShader(const char* source, GLenum type);
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
};

#endif // TEXTURE_H
