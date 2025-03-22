#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int shadingMode;

out vec3 FragPos;
out vec3 Normal;
out vec3 GouraudLight;
out vec2 UV;

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
    Normal = mat3(transpose(inverse(model))) * aNormal;
    UV = aPos.xy * 0.5 + 0.5; // Fake UVs if not using real ones

    GouraudLight = vec3(0.0);

    if (shadingMode == 1) {
        vec3 normal = normalize(Normal);
        vec3 viewDir = normalize(-FragPos);
        for (int i = 0; i < numLights; i++) {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            GouraudLight += diff * lights[i].color * lights[i].intensity;
        }
    }

    gl_Position = projection * view * worldPos;
}
