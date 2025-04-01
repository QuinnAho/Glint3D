#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 GouraudLight;  // We only use this if shadingMode == 1
in vec2 UV;

// For texturing & shading controls
uniform sampler2D cowTexture;
uniform bool useTexture;
uniform int shadingMode;
uniform vec3 objectColor; // fallback if no texture

// ------------------------------------------------------------------------
// Global ambient
uniform vec4 globalAmbient; // .a is often unused, so we just use .rgb

// Light struct
struct Light {
    vec3 position;
    vec3 color;
    float intensity; // If 0, treat as disabled
};

#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

// ------------------------------------------------------------------------
// Material struct
struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float shininess;
    float roughness;
    float metallic;
};
uniform Material material;

// Camera position (for specular reflection in Gouraud)
uniform vec3 viewPos;

// ------------------------------------------------------------------------
// Main
void main()
{
    // Base color from either texture or fallback color
    vec3 baseColor = useTexture ? texture(cowTexture, UV).rgb : objectColor;

    // Start with an ambient term (global + material's own ambient color)
    vec3 totalLight = globalAmbient.rgb * material.ambient;

    if (shadingMode == 0) {
        // ----- FLAT Shading -----
        // Recompute a face normal using derivatives
        vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));

        // Simple diffuse for each light
        for (int i = 0; i < numLights; i++) {
            vec3 L = normalize(lights[i].position - FragPos);
            float diff = max(dot(faceNormal, L), 0.0);
            totalLight += material.diffuse * diff * lights[i].color * lights[i].intensity;
        }
    }
    else if (shadingMode == 1) {
        // ----- GOURAUD Shading -----
        // The vertex shader already computed diffuse + specular
        totalLight += GouraudLight;
    }

    // Multiply lighting by base color (texture or objectColor)
    vec3 finalColor = baseColor * totalLight;
    FragColor = vec4(finalColor, 1.0);
}
