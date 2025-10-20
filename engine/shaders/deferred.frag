#version 330 core

out vec4 FragColor;

in vec2 vUV;

// G-Buffer textures
layout(binding = 0) uniform sampler2D gBaseColor;  // RGB: base color, A: metallic
layout(binding = 1) uniform sampler2D gNormal;     // RGB: world normal, A: roughness
layout(binding = 2) uniform sampler2D gPosition;   // RGB: world position, A: depth
layout(binding = 3) uniform sampler2D gMaterial;   // R: transmission, G: ior, B: thickness, A: unused

// Light types
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT 2

// Lights uniform block
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

// Rendering uniform block
layout(std140) uniform RenderingBlock {
    float exposure;
    float gamma;
    int toneMappingMode;
    int shadingMode;
    float iblIntensity;
    vec3 objectColor;
};

// IBL textures
layout(binding = 4) uniform samplerCube irradianceMap;
layout(binding = 5) uniform samplerCube prefilterMap;
layout(binding = 6) uniform sampler2D brdfLUT;

// PBR Functions (copied from pbr.frag)
vec3 getNormalFromMap(vec2 uv, vec3 worldPos, vec3 normal)
{
    return normal; // Normal already processed in G-buffer
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Tone mapping
vec3 toneMapACES(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 toneMapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main()
{
    // Sample G-Buffer
    vec4 baseColorSample = texture(gBaseColor, vUV);
    vec4 normalSample = texture(gNormal, vUV);
    vec4 positionSample = texture(gPosition, vUV);
    vec4 materialSample = texture(gMaterial, vUV);

    // Early discard for background pixels
    if (positionSample.a == 0.0) {
        discard;
    }

    // Decode G-Buffer data
    vec3 albedo = baseColorSample.rgb;
    float metallic = baseColorSample.a;
    vec3 normal = normalize(normalSample.rgb * 2.0 - 1.0);
    float roughness = normalSample.a;
    vec3 worldPos = positionSample.rgb;
    float transmission = materialSample.r;
    float ior = materialSample.g;
    float thickness = materialSample.b;

    // Calculate F0 for PBR
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // If material has transmission, calculate F0 from IOR
    if (transmission > 0.01) {
        float f0_dielectric = pow((1.0 - ior) / (1.0 + ior), 2.0);
        F0 = vec3(f0_dielectric);
        F0 = mix(F0, albedo, metallic);
    }

    vec3 V = normalize(viewPos - worldPos);
    vec3 R = reflect(-V, normal);

    // Direct lighting calculation
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        vec3 L;
        float attenuation = 1.0;

        if (lights[i].type == LIGHT_DIRECTIONAL) {
            L = normalize(-lights[i].direction);
        } else if (lights[i].type == LIGHT_POINT) {
            L = normalize(lights[i].position - worldPos);
            float distance = length(lights[i].position - worldPos);
            attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        } else if (lights[i].type == LIGHT_SPOT) {
            L = normalize(lights[i].position - worldPos);
            float distance = length(lights[i].position - worldPos);
            attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

            float theta = dot(L, normalize(-lights[i].direction));
            float epsilon = lights[i].innerCutoff - lights[i].outerCutoff;
            float intensity = clamp((theta - lights[i].outerCutoff) / epsilon, 0.0, 1.0);
            attenuation *= intensity;
        }

        vec3 H = normalize(V + L);
        vec3 radiance = lights[i].color * lights[i].intensity * attenuation;

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(normal, L), 0.0);
        Lo += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    }

    // IBL ambient lighting
    vec3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(normal, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * iblIntensity;
    vec3 color = ambient + Lo;

    // Apply exposure and tone mapping
    color *= exposure;

    if (toneMappingMode == 1) {
        color = toneMapReinhard(color);
    } else if (toneMappingMode == 2) {
        color = toneMapACES(color);
    }

    // Gamma correction
    color = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}
