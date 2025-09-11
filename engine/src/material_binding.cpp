#include "material_binding.h"
#include "shader.h"

void bindMaterialUniforms(Shader& shader, const MaterialCore& mat)
{
    // Basic PBR
    shader.setVec4("baseColorFactor", mat.baseColor);
    shader.setFloat("metallicFactor", mat.metallic);
    shader.setFloat("roughnessFactor", mat.roughness);
    shader.setFloat("ior", mat.ior);

    // Extensions
    shader.setFloat("transmission", mat.transmission);
    shader.setFloat("thickness", mat.thickness);
    shader.setFloat("attenuationDistance", mat.attenuationDistance);
    // Use baseColor as a reasonable default attenuation tint if no dedicated color exists
    shader.setVec3("attenuationColor", glm::vec3(mat.baseColor));
    shader.setFloat("clearcoat", mat.clearcoat);
    shader.setFloat("clearcoatRoughness", mat.clearcoatRoughness);
}

