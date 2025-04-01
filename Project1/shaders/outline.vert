#version 330 core

layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float outlineThickness;

void main()
{
    // Expand vertex position along its normal in world space
    vec3 norm = normalize(mat3(transpose(inverse(model))) * aPos);
    vec4 worldPos = model * vec4(aPos + norm * outlineThickness, 1.0);
    gl_Position = projection * view * worldPos;
}
