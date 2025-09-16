#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;

// Transform matrices uniform block
layout(std140) uniform TransformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;  // For shadow mapping
};

// Material properties uniform block (for hasTangents flag)
layout(std140) uniform MaterialBlock {
    vec4 baseColorFactor; // rgba
    float metallicFactor;
    float roughnessFactor;
    float ior;            // Index of refraction for F0 computation
    float transmission;   // [0,1]
    float thickness;      // meters
    float attenuationDistance; // meters (Beer-Lambert)
    vec3 attenuationColor;     // tint for attenuation (approx)
    float clearcoat;           // [0,1]
    float clearcoatRoughness;  // [0,1]

    // Texture flags (bools as ints for std140)
    bool hasBaseColorMap;  // int converted to bool
    bool hasNormalMap;     // int converted to bool
    bool hasMRMap;         // int converted to bool
    bool hasTangents;      // int converted to bool
    bool useTexture;       // int converted to bool
};

out vec3 vWorldPos;
out vec2 vUV;
out mat3 vTBN;

void main() {
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 T;
    if (hasTangents) {
        T = normalize(mat3(model) * aTangent);
        // Orthonormalize T against N
        T = normalize(T - N * dot(N, T));
    } else {
        // Fallback: build arbitrary T perpendicular to N
        vec3 up = abs(N.y) < 0.999 ? vec3(0,1,0) : vec3(1,0,0);
        T = normalize(cross(up, N));
    }
    vec3 B = normalize(cross(N, T));

    vTBN = mat3(T, B, N);
    vUV = aUV;
    vWorldPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(vWorldPos, 1.0);
}
