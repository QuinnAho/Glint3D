#pragma once

#include "material-core.h"
class Shader;

// Bind MaterialCore fields to shader uniforms expected by pbr.frag
// Uniform names:
//  baseColorFactor (vec4), metallicFactor (float), roughnessFactor (float), ior (float)
//  transmission (float), thickness (float), attenuationDistance (float), attenuationColor (vec3)
//  clearcoat (float), clearcoatRoughness (float)
void bindMaterialUniforms(Shader& shader, const MaterialCore& mat);

