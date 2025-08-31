#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& filepath, bool flipY = false);
    void bind(GLuint unit = 0) const;

private:
    GLuint m_textureID;
};

#endif // TEXTURE_H
