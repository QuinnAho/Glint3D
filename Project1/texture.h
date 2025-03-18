#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    // Loads texture from file and returns texture ID
    bool loadFromFile(const std::string& filepath);

    // Activates and binds the texture to a specified unit
    void bind(GLuint unit = 0) const;

    GLuint getTextureID() const;

private:
    GLuint m_textureID;
};

#endif // TEXTURE_H
