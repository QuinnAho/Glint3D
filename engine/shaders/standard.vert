#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// Transform matrices uniform block
layout(std140) uniform TransformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;  // For shadow mapping
};

// Shading mode: 0=Flat, 1=Gouraud
uniform int shadingMode;

// Outputs to the fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec3 GouraudLight;
out vec2 UV;

// Light types
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT 2

// Lights uniform block (matching standard.frag)
struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float innerCutoff; // cos(inner)
    float outerCutoff; // cos(outer)
    float _padding;   // std140 alignment
};

#define MAX_LIGHTS 10
layout(std140) uniform LightingBlock {
    int numLights;
    vec3 viewPos;
    vec4 globalAmbient;
    Light lights[MAX_LIGHTS];
};

// Material for Gouraud specular
struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float shininess;
    float roughness;
    float metallic;
};

uniform Material material;

void main()
{
    // Compute position in world space
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Normal transformation via normal matrix
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Use actual texture coordinates if available
    UV = aTexCoord;

    // Default to black unless we're in Gouraud shading mode
    GouraudLight = vec3(0.0);

    // Only compute Gouraud if shadingMode == 1
    if (shadingMode == 1) {
        vec3 normal = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        for (int i = 0; i < numLights; i++) {
            if (lights[i].intensity <= 0.0) continue;
            
            vec3 lightDir;
            if (lights[i].type == LIGHT_POINT) {
                lightDir = normalize(lights[i].position - FragPos);
            } else if (lights[i].type == LIGHT_DIRECTIONAL) {
                lightDir = normalize(-lights[i].direction);
            } else if (lights[i].type == LIGHT_SPOT) {
                // Spot: direction points out from light; compare with vector to fragment
                lightDir = normalize(lights[i].position - FragPos);
            } else {
                continue;
            }

            float spotFactor = 1.0;
            float atten = 1.0;
            if (lights[i].type == LIGHT_POINT) {
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
            } else if (lights[i].type == LIGHT_SPOT) {
                // Attenuation + cone falloff
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
                float theta = dot(normalize(-lights[i].direction), lightDir);
                float inner = lights[i].innerCutoff;
                float outer = lights[i].outerCutoff;
                float t = clamp((theta - outer) / max(inner - outer, 1e-4), 0.0, 1.0);
                spotFactor = t;
            } else {
                continue;
            }

            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 radiance = lights[i].color * lights[i].intensity * atten * spotFactor;
            vec3 diffuse = material.diffuse * diff * radiance;

            // Specular (Phong reflection model but computed at vertex)
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            vec3 specular = material.specular * spec * radiance;

            GouraudLight += diffuse + specular;
        }
    }

    // Final position for rasterization
    gl_Position = projection * view * worldPos;
}
