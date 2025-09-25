#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

out vec3 vWorldPos;
out vec2 vUV;
out mat3 vTBN;

// Transform uniform block
layout(std140) uniform TransformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
};

void main()
{
    vWorldPos = vec3(model * vec4(aPos, 1.0));
    vUV = aUV;

    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));
    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    vTBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(vWorldPos, 1.0);
}