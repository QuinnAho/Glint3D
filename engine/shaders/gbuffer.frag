#version 330 core

// G-Buffer outputs
layout (location = 0) out vec4 gBaseColor;    // RGB: base color, A: metallic
layout (location = 1) out vec4 gNormal;       // RGB: world normal, A: roughness
layout (location = 2) out vec4 gPosition;     // RGB: world position, A: depth
layout (location = 3) out vec4 gMaterial;     // R: transmission, G: ior, B: thickness, A: unused

in vec3 vWorldPos;
in vec2 vUV;
in mat3 vTBN;

// Material properties uniform block
layout(std140) uniform MaterialBlock {
    vec4 baseColorFactor;     // rgba
    float metallicFactor;
    float roughnessFactor;
    float ior;               // Index of refraction for F0 computation
    float transmission;      // [0,1]
    float thickness;         // meters
    float attenuationDistance; // meters (Beer-Lambert)
    vec3 attenuationColor;     // tint for attenuation (approx)
    float clearcoat;           // [0,1]
    float clearcoatRoughness;  // [0,1]

    // Texture flags (bools as ints for std140)
    bool hasBaseColorMap;
    bool hasNormalMap;
    bool hasMRMap;
    bool hasTangents;
    bool useTexture;
};

// Texture samplers (match TextureSlots constants)
layout(binding = 0) uniform sampler2D baseColorTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D mrTex; // Metallic-Roughness texture

void main()
{
    // Sample base color
    vec4 baseColor = baseColorFactor;
    if (hasBaseColorMap) {
        vec4 texColor = texture(baseColorTex, vUV);
        baseColor *= texColor;
    }

    // Sample metallic and roughness
    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    if (hasMRMap) {
        vec3 mr = texture(mrTex, vUV).rgb;
        metallic *= mr.b;    // Blue channel = metallic
        roughness *= mr.g;   // Green channel = roughness
    }

    // Sample and calculate normal
    vec3 normal = normalize(vTBN[2]); // Default to surface normal
    if (hasNormalMap && hasTangents) {
        vec3 normalSample = texture(normalTex, vUV).rgb;
        normal = normalize(normalSample * 2.0 - 1.0);
        normal = normalize(vTBN * normal);
    }

    // Output to G-Buffer
    gBaseColor = vec4(baseColor.rgb, metallic);
    gNormal = vec4(normal * 0.5 + 0.5, roughness); // Encode normal to [0,1]
    gPosition = vec4(vWorldPos, gl_FragCoord.z);
    gMaterial = vec4(transmission, ior, thickness, 1.0);
}
