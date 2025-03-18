#include "Texture.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vertex Shader Source
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int shadingMode; // 0 = Flat, 1 = Gouraud, 2 = Phong

out vec3 FragPos;
out vec3 Normal;
out vec3 LightColor;
out vec3 LightPos;
out vec3 ViewPos;
out vec2 ModelUV;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal; // Correct normal transformation

    ModelUV = aPos.xy * 0.5 + 0.5;

    LightColor = vec3(0.0);
    LightPos = vec3(0.0);
    ViewPos = vec3(0.0);

    // Compute Gouraud shading if selected
    if (shadingMode == 1) {
        vec3 normal = normalize(Normal);
        vec3 viewDir = normalize(-FragPos);
        for (int i = 0; i < numLights; i++)
        {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            LightColor += diff * lights[i].color * lights[i].intensity;
        }
    }

    gl_Position = projection * view * worldPos;
}
)";



// Fragment Shader Source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec3 LightColor;
in vec2 ModelUV;

uniform sampler2D cowTexture;
uniform bool useTexture;
uniform int shadingMode; // 0 = Flat, 1 = Gouraud, 2 = Phong

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(-FragPos);
    vec3 totalLight = vec3(0.0);

    if (shadingMode == 0) {
        // Flat Shading
        vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos))); // Compute face normal
        for (int i = 0; i < numLights; i++)
        {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(faceNormal, lightDir), 0.0);
            totalLight += diff * lights[i].color * lights[i].intensity;
        }
    }
    else if (shadingMode == 1) {
        // Gouraud Shading
        totalLight = LightColor;
    }
    else {
        // Phong Shading (Default)
        for (int i = 0; i < numLights; i++)
        {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            totalLight += diff * lights[i].color * lights[i].intensity;
        }
    }

    vec3 objectColor = useTexture ? texture(cowTexture, ModelUV).rgb : vec3(1.0);

    FragColor = vec4(objectColor * totalLight, 1.0);
}
)";

Texture::Texture() : m_textureID(0), m_shaderProgram(0)
{
}

Texture::~Texture()
{
    if (m_textureID)
        glDeleteTextures(1, &m_textureID);
    if (m_shaderProgram)
        glDeleteProgram(m_shaderProgram);
}

bool Texture::loadFromFile(const std::string& filepath)
{
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return false;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Texture::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

bool Texture::initShaders()
{
    m_shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    return m_shaderProgram != 0;
}

GLuint Texture::compileShader(const char* source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
        return 0;
    }

    return shader;
}

GLuint Texture::createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    if (!vertexShader || !fragmentShader)
        return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program Linking Error:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
