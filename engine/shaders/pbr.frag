#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vUV;
in mat3 vTBN;

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

// Material properties uniform block
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

// Rendering state uniform block
layout(std140) uniform RenderingBlock {
    float exposure;
    float gamma;
    int toneMappingMode;
    int shadingMode;

    float iblIntensity;

    vec3 objectColor;  // for wireframe/debug modes
};

// Texture samplers (not in UBOs)
uniform sampler2D baseColorTex;
uniform sampler2D normalTex;
uniform sampler2D mrTex; // glTF convention: G=roughness, B=metallic

uniform sampler2D shadowMap;

// IBL textures
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// Shadow
float calculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;
    return (currentDepth - bias) > closestDepth ? 0.5 : 1.0;
}

// Helpers
const float PI = 3.14159265359;

// Compute F0 from index of refraction: F0 = ((ior-1)/(ior+1))^2
float computeF0FromIOR(float ior) {
    float f0_scalar = (ior - 1.0) / (ior + 1.0);
    return f0_scalar * f0_scalar;
}

// Schlick Fresnel approximation: F = F0 + (1-F0)(1-V·H)^5
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a = rough*rough; float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 1e-4);
}
float GeometrySchlickGGX(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, rough);
    float ggx2 = GeometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

// Clearcoat lobe (second specular layer)
vec3 computeClearcoat(vec3 N, vec3 V, vec3 L, float cc, float ccRough) {
    if (cc < 0.01) return vec3(0.0);
    vec3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, ccRough);
    float G   = GeometrySmith(N, V, L, ccRough);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04)); // fixed F0 for clearcoat
    return (NDF * G) * F * cc;
}

// Simple Beer-Lambert attenuation using per-material thickness
vec3 applyVolumeAttenuation(vec3 color, float distance, float attnDistance, vec3 attnColor) {
    if (distance <= 0.0 || attnDistance <= 0.0) return color;
    float attn = exp(-distance / attnDistance);
    return mix(attnColor, color, attn);
}

// Tone mapping functions
vec3 reinhardToneMapping(vec3 color) {
    return color / (1.0 + color);
}

vec3 filmicToneMapping(vec3 color) {
    vec3 x = max(vec3(0.0), color - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

vec3 acesToneMapping(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 applyToneMapping(vec3 color) {
    // Apply exposure first - use exp2 for better performance
    color *= exp2(exposure);
    
    // Optimized tone mapping with fewer branches
    vec3 reinhard = color / (1.0 + color);
    vec3 x = max(vec3(0.0), color - 0.004);
    vec3 filmic = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    vec3 aces = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    
    // Use step functions instead of branches for better GPU performance
    float useReinhard = step(0.5, float(toneMappingMode == 1));
    float useFilmic = step(0.5, float(toneMappingMode == 2));
    float useAces = step(0.5, float(toneMappingMode == 3));
    
    color = mix(color, reinhard, useReinhard);
    color = mix(color, filmic, useFilmic);
    color = mix(color, aces, useAces);
    
    return color;
}

void main() {
    // Sample inputs
    vec3 albedo = baseColorFactor.rgb;
    if (hasBaseColorMap) albedo = pow(texture(baseColorTex, vUV).rgb, vec3(2.2)); // assume sRGB
    float metallic = metallicFactor;
    float roughness = clamp(roughnessFactor, 0.04, 1.0);
    if (hasMRMap) {
        vec3 mrs = texture(mrTex, vUV).rgb; // R=occlusion (optional), G=roughness, B=metallic (glTF)
        roughness = clamp(mrs.g, 0.04, 1.0);
        metallic = mrs.b;
    }

    // Normal mapping
    vec3 N = normalize(vTBN[2]);
    if (hasNormalMap) {
        vec3 n = texture(normalTex, vUV).xyz * 2.0 - 1.0;
        N = normalize(vTBN * n);
    }

    vec3 V = normalize(viewPos - vWorldPos);
    
    // F0 derivation: For metallic workflow, mix between dielectric F0 and baseColor
    // For non-metals (metallic=0), use either default 0.04 or compute from IOR
    // For metals (metallic=1), use baseColor as F0
    vec3 dielectricF0 = vec3(0.04); // Default for generic dielectrics
    if (ior != 1.5 && metallic < 0.01) { // Custom IOR for non-metallic materials
        dielectricF0 = vec3(computeF0FromIOR(ior));
    }
    vec3 F0 = mix(dielectricF0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    // Disable shadowing until we implement a proper shadow map path
    float shadow = 1.0;
    for (int i=0;i<numLights;i++) {
        if (lights[i].intensity <= 0.0) continue;
        
        vec3 L;
        vec3 radiance;
        
        if (lights[i].type == LIGHT_POINT) {
            // Point light calculation
            L = normalize(lights[i].position - vWorldPos);
            float dist = length(lights[i].position - vWorldPos);
            float atten = 1.0 / (dist*dist);
            radiance = lights[i].color * lights[i].intensity * atten;
        } else if (lights[i].type == LIGHT_DIRECTIONAL) {
            // Directional light calculation
            L = normalize(-lights[i].direction);  // Light direction points towards the surface
            radiance = lights[i].color * lights[i].intensity;  // No attenuation for directional lights
        } else if (lights[i].type == LIGHT_SPOT) {
            // Spot light calculation
            L = normalize(lights[i].position - vWorldPos);
            float dist = length(lights[i].position - vWorldPos);
            float atten = 1.0 / (dist*dist);
            float theta = dot(normalize(-lights[i].direction), L);
            float inner = lights[i].innerCutoff;
            float outer = lights[i].outerCutoff;
            float t = clamp((theta - outer) / max(inner - outer, 1e-4), 0.0, 1.0);
            radiance = lights[i].color * lights[i].intensity * atten * t;
        } else {
            continue; // Skip unknown light types
        }
        
        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 nominator = NDF * G * F;
        float denom = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 1e-4;
        vec3 specular = nominator / denom;

        // Clearcoat secondary lobe
        vec3 clearcoatSpec = computeClearcoat(N, V, L, clearcoat, clearcoatRoughness);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N,L), 0.0);
        Lo += (kD * albedo / PI + specular + clearcoatSpec) * radiance * NdotL * shadow;
    }

    // IBL ambient lighting
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    // Sample both the pre-filter map and BRDF LUT and combine them together as per the BRDF equation
    vec3 R = reflect(-V, N);   
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * iblIntensity;
    vec3 color = ambient + Lo;

    // Apply simple volumetric attenuation using material thickness
    color = applyVolumeAttenuation(color, thickness, max(attenuationDistance, 1e-4), attenuationColor);
    
    // Apply tone mapping and exposure
    color = applyToneMapping(color);
    
    // Apply gamma correction
    color = pow(color, vec3(1.0/gamma));
    FragColor = vec4(color, baseColorFactor.a);
}
