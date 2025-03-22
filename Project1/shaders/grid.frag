#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

// Control grid look
const float gridSize = 1.0;
const float lineWidth = 0.02;
const vec3 gridColor = vec3(0.5);
const vec3 axisColor = vec3(1.0, 0.0, 0.0); // Red X and Blue Z later

void main()
{
    // Raycast into world from UV
    vec2 screenUV = vUV * 2.0 - 1.0;
    vec4 clip = vec4(screenUV, -1.0, 1.0);
    vec4 eye = inverse(projection) * clip;
    eye = vec4(eye.xy, -1.0, 0.0);
    vec3 rayDir = normalize((inverse(view) * eye).xyz);

    // Find intersection with y=0 plane (ZX plane)
    float t = -cameraPos.y / rayDir.y;
    vec3 pos = cameraPos + t * rayDir;

    // Draw grid lines by checking how close to nearest integer
    float xLine = abs(fract(pos.x / gridSize) - 0.5);
    float zLine = abs(fract(pos.z / gridSize) - 0.5);
    float line = min(xLine, zLine);

    float intensity = smoothstep(lineWidth, 0.0, line);
    vec3 finalColor = mix(vec3(0.0), gridColor, intensity);

    FragColor = vec4(finalColor, 1.0);
}
