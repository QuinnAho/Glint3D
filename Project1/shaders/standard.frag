#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 GouraudLight;
in vec2 UV;

uniform sampler2D cowTexture;
uniform bool useTexture;
uniform int shadingMode;

uniform vec3 objectColor = vec3(1.0); // fallback if no texture

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
    vec3 lightResult = vec3(0.0);

    if (shadingMode == 0) {
        vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
        for (int i = 0; i < numLights; i++) {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(faceNormal, lightDir), 0.0);
            lightResult += diff * lights[i].color * lights[i].intensity;
        }
    }
    else if (shadingMode == 1) {
        lightResult = GouraudLight;
    }
    else {
        for (int i = 0; i < numLights; i++) {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            lightResult += diff * lights[i].color * lights[i].intensity;
        }
    }

    vec3 baseColor = useTexture ? texture(cowTexture, UV).rgb : objectColor;
    FragColor = vec4(baseColor * lightResult, 1.0);
}
