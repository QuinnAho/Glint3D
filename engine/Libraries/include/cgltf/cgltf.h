#pragma once

// cgltf - Lightweight, robust glTF 2.0 loader
// This is a header-only glTF 2.0 loader implementation
// Full implementation should be downloaded from https://github.com/jkuhlmann/cgltf

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace cgltf {

// Forward declarations
struct Node;
struct Mesh;
struct Material;
struct Scene;
struct Buffer;
struct BufferView;
struct Accessor;
struct Image;
struct Texture;
struct Sampler;

// Basic types
using size_type = std::size_t;
using float_type = float;

// Component types for accessors
enum class component_type : uint16_t {
    byte = 5120,
    unsigned_byte = 5121,
    short_ = 5122,
    unsigned_short = 5123,
    unsigned_int = 5125,
    float_ = 5126
};

// Accessor types
enum class accessor_type {
    scalar,
    vec2,
    vec3,
    vec4,
    mat2,
    mat3,
    mat4
};

// glTF primitive modes
enum class primitive_mode : uint8_t {
    points = 0,
    lines = 1,
    line_loop = 2,
    line_strip = 3,
    triangles = 4,
    triangle_strip = 5,
    triangle_fan = 6
};

// Material alpha modes
enum class alpha_mode {
    opaque,
    mask,
    blend
};

// Texture filters
enum class filter : uint16_t {
    nearest = 9728,
    linear = 9729,
    nearest_mipmap_nearest = 9984,
    linear_mipmap_nearest = 9985,
    nearest_mipmap_linear = 9986,
    linear_mipmap_linear = 9987
};

// Texture wrap modes
enum class wrap : uint16_t {
    clamp_to_edge = 33071,
    mirrored_repeat = 33648,
    repeat = 10497
};

// Buffer structure
struct Buffer {
    std::string uri;
    size_type byte_length = 0;
    std::vector<uint8_t> data;
};

// Buffer view structure
struct BufferView {
    size_type buffer = 0;
    size_type byte_offset = 0;
    size_type byte_length = 0;
    size_type byte_stride = 0;
    bool has_byte_stride = false;
};

// Accessor structure
struct Accessor {
    size_type buffer_view = 0;
    size_type byte_offset = 0;
    component_type component_type_ = component_type::float_;
    bool normalized = false;
    size_type count = 0;
    accessor_type type = accessor_type::scalar;
    std::vector<float_type> min;
    std::vector<float_type> max;
    bool has_min = false;
    bool has_max = false;
};

// Texture info
struct texture_info {
    size_type index = 0;
    size_type tex_coord = 0;
};

// PBR metallic roughness
struct pbr_metallic_roughness {
    std::vector<float_type> base_color_factor = {1.0f, 1.0f, 1.0f, 1.0f};
    texture_info base_color_texture;
    bool has_base_color_texture = false;
    float_type metallic_factor = 1.0f;
    float_type roughness_factor = 1.0f;
    texture_info metallic_roughness_texture;
    bool has_metallic_roughness_texture = false;
};

// Material structure
struct Material {
    std::string name;
    pbr_metallic_roughness pbr_metallic_roughness_;
    texture_info normal_texture;
    bool has_normal_texture = false;
    texture_info occlusion_texture;
    bool has_occlusion_texture = false;
    texture_info emissive_texture;
    bool has_emissive_texture = false;
    std::vector<float_type> emissive_factor = {0.0f, 0.0f, 0.0f};
    alpha_mode alpha_mode_ = alpha_mode::opaque;
    float_type alpha_cutoff = 0.5f;
    bool double_sided = false;
};

// Primitive structure
struct Primitive {
    std::map<std::string, size_type> attributes;
    size_type indices = 0;
    bool has_indices = false;
    size_type material = 0;
    bool has_material = false;
    primitive_mode mode = primitive_mode::triangles;
};

// Mesh structure
struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
};

// Node structure
struct Node {
    std::string name;
    std::vector<size_type> children;
    size_type mesh = 0;
    bool has_mesh = false;
    std::vector<float_type> matrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    bool has_matrix = false;
    std::vector<float_type> translation = {0.0f, 0.0f, 0.0f};
    bool has_translation = false;
    std::vector<float_type> rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bool has_rotation = false;
    std::vector<float_type> scale = {1.0f, 1.0f, 1.0f};
    bool has_scale = false;
};

// Image structure
struct Image {
    std::string name;
    std::string uri;
    std::string mime_type;
    size_type buffer_view = 0;
    bool has_buffer_view = false;
};

// Sampler structure
struct Sampler {
    filter mag_filter = filter::linear;
    bool has_mag_filter = false;
    filter min_filter = filter::linear;
    bool has_min_filter = false;
    wrap wrap_s = wrap::repeat;
    wrap wrap_t = wrap::repeat;
};

// Texture structure
struct Texture {
    size_type sampler = 0;
    bool has_sampler = false;
    size_type source = 0;
    bool has_source = false;
};

// Scene structure
struct Scene {
    std::string name;
    std::vector<size_type> nodes;
};

// Main glTF document structure
struct Document {
    std::vector<Buffer> buffers;
    std::vector<BufferView> buffer_views;
    std::vector<Accessor> accessors;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<Texture> textures;
    std::vector<Scene> scenes;
    size_type scene = 0;
    bool has_scene = false;
};

// Result structure for loading operations
template<typename T>
struct result {
    T data;
    std::string error;
    bool success = false;
    
    operator bool() const { return success; }
};

// Loading functions
result<Document> load_gltf(const std::string& filename);
result<Document> parse_gltf(const std::string& json_content);
result<Document> load_glb(const std::string& filename);

// Utility functions
size_type component_size(component_type type);
size_type accessor_element_count(accessor_type type);
size_type accessor_stride(const Accessor& accessor);

} // namespace cgltf

// Note: This is a minimal placeholder implementation
// For production use, download the full cgltf library from:
// https://github.com/jkuhlmann/cgltf
// Or use similar libraries like tinygltf