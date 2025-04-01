#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 GouraudLight;
in vec2 UV;

// Texturing & shading mode controls
uniform sampler2D cowTexture;
uniform bool useTexture;
uniform int shadingMode;
uniform vec3 objectColor; // fallback if no texture

// NEW: Global ambient light
uniform vec4 globalAmbient; 

// Light struct
struct Light {
    vec3 position;
    vec3 color;
    float intensity; // If intensity=0, treat as 'disabled'.
};

#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

void main()
{
    // Base color from texture or solid color
    vec3 baseColor = useTexture ? texture(cowTexture, UV).rgb : objectColor;

    // Start with ambient term
    vec3 totalLight = globalAmbient.rgb; // ignoring globalAmbient.a here, typically it’s not used for shading

    // Normalized normal & view direction (if needed)
    vec3 N = normalize(Normal);

    // Decide on lighting based on shading mode
    if (shadingMode == 0) {
        // ---- FLAT (face) Shading ----
        // Recompute a face normal using derivatives of FragPos
        vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));

        vec3 flatLighting = vec3(0.0);
        for (int i = 0; i < numLights; i++) {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(faceNormal, lightDir), 0.0);
            flatLighting += diff * lights[i].color * lights[i].intensity;
        }
        totalLight += flatLighting;
    }
    else if (shadingMode == 1) {
        // ---- GOURAUD Shading ----
        // GouraudLight was computed in the vertex shader (diffuse, etc.).
        // Just add it to global ambient here
        totalLight += GouraudLight; 
    }
    else {
        // ---- PHONG Shading ----
        // Per-pixel normal-based lighting:
        vec3 phongLighting = vec3(0.0);
        for (int i = 0; i < numLights; i++) {
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(N, lightDir), 0.0);
            phongLighting += diff * lights[i].color * lights[i].intensity;
        }
        totalLight += phongLighting;
    }

    // Final color
    vec3 finalColor = baseColor * totalLight;
    FragColor = vec4(finalColor, 1.0);
}
