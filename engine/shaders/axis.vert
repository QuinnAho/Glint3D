#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

// Transform matrices uniform block
layout(std140) uniform TransformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;  // For shadow mapping
};

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
}
