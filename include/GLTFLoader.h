#pragma once

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace CorePulse {

// Forward declarations
class Mesh;
class Texture;
class Material;

// Import Vertex struct from Mesh.h
struct Vertex;

// glTF 2.0 data structures following the specification
namespace GLTF {

    // Asset metadata
    struct Asset {
        std::string version = "2.0";
        std::string generator;
        std::string copyright;
        std::string min_version;
    };

    // Buffer data
    struct Buffer {
        std::string uri;
        uint32_t byte_length = 0;
        std::vector<uint8_t> data; // For embedded or loaded buffer data
    };

    // Buffer view
    struct BufferView {
        uint32_t buffer = 0;
        uint32_t byte_offset = 0;
        uint32_t byte_length = 0;
        uint32_t byte_stride = 0; // Optional
        uint32_t target = 0; // GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    };

    // Accessor for buffer data
    struct Accessor {
        enum class Type {
            SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4
        };
        
        enum class ComponentType {
            BYTE = 5120,
            UNSIGNED_BYTE = 5121,
            SHORT = 5122,
            UNSIGNED_SHORT = 5123,
            UNSIGNED_INT = 5125,
            FLOAT = 5126
        };

        uint32_t buffer_view = 0;
        uint32_t byte_offset = 0;
        ComponentType component_type = ComponentType::FLOAT;
        bool normalized = false;
        uint32_t count = 0;
        Type type = Type::SCALAR;
        std::vector<float> min;
        std::vector<float> max;
    };

    // Image data
    struct Image {
        std::string uri;
        std::string mime_type;
        uint32_t buffer_view = 0; // For embedded images
    };

    // Texture sampler
    struct Sampler {
        uint32_t mag_filter = 9729; // GL_LINEAR
        uint32_t min_filter = 9987; // GL_LINEAR_MIPMAP_LINEAR
        uint32_t wrap_s = 10497; // GL_REPEAT
        uint32_t wrap_t = 10497; // GL_REPEAT
    };

    // Texture
    struct Texture {
        uint32_t sampler = 0;
        uint32_t source = 0; // Image index
    };

    // Material PBR properties
    struct PBRMetallicRoughness {
        glm::vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
        uint32_t base_color_texture = 0;
        float metallic_factor = 1.0f;
        float roughness_factor = 1.0f;
        uint32_t metallic_roughness_texture = 0;
    };

    // Material
    struct Material {
        std::string name;
        PBRMetallicRoughness pbr_metallic_roughness;
        uint32_t normal_texture = 0;
        uint32_t occlusion_texture = 0;
        uint32_t emissive_texture = 0;
        glm::vec3 emissive_factor{0.0f, 0.0f, 0.0f};
        std::string alpha_mode = "OPAQUE"; // OPAQUE, MASK, BLEND
        float alpha_cutoff = 0.5f;
        bool double_sided = false;
    };

    // Mesh primitive
    struct Primitive {
        std::unordered_map<std::string, uint32_t> attributes; // POSITION, NORMAL, TEXCOORD_0, etc.
        uint32_t indices = 0;
        uint32_t material = 0;
        uint32_t mode = 4; // GL_TRIANGLES
    };

    // Mesh
    struct Mesh {
        std::string name;
        std::vector<Primitive> primitives;
    };

    // Node transformation
    struct Node {
        std::string name;
        std::vector<uint32_t> children;
        uint32_t mesh = 0; // Optional mesh index
        glm::mat4 matrix{1.0f}; // Transformation matrix
        glm::vec3 translation{0.0f};
        glm::vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f}; // Quaternion
        glm::vec3 scale{1.0f};
    };

    // Scene
    struct Scene {
        std::string name;
        std::vector<uint32_t> nodes;
    };

    // Main glTF document
    struct Document {
        Asset asset;
        uint32_t scene = 0; // Default scene index
        std::vector<Scene> scenes;
        std::vector<Node> nodes;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<Texture> textures;
        std::vector<Image> images;
        std::vector<Sampler> samplers;
        std::vector<Accessor> accessors;
        std::vector<BufferView> buffer_views;
        std::vector<Buffer> buffers;
        
        // Extensions and extras
        nlohmann::json extensions;
        nlohmann::json extras;
    };

} // namespace GLTF

// glTF Loader class
class GLTFLoader {
public:
    GLTFLoader();
    ~GLTFLoader() = default;

    // Load glTF file
    bool load_gltf(const std::string& filepath);
    bool load_glb(const std::string& filepath);

    // Access loaded data
    const GLTF::Document& get_document() const { return document_; }
    
    // Extract meshes for rendering
    std::vector<std::shared_ptr<Mesh>> extract_meshes();
    std::vector<std::shared_ptr<Material>> extract_materials();
    std::vector<std::shared_ptr<Texture>> extract_textures();

    // Utility functions
    bool is_loaded() const { return loaded_; }
    const std::string& get_error() const { return error_message_; }

private:
    GLTF::Document document_;
    std::string base_path_;
    bool loaded_ = false;
    mutable std::string error_message_;

    // Parsing functions
    bool parse_json(const nlohmann::json& json);
    bool parse_asset(const nlohmann::json& json);
    bool parse_scenes(const nlohmann::json& json);
    bool parse_nodes(const nlohmann::json& json);
    bool parse_meshes(const nlohmann::json& json);
    bool parse_materials(const nlohmann::json& json);
    bool parse_textures(const nlohmann::json& json);
    bool parse_images(const nlohmann::json& json);
    bool parse_samplers(const nlohmann::json& json);
    bool parse_accessors(const nlohmann::json& json);
    bool parse_buffer_views(const nlohmann::json& json);
    bool parse_buffers(const nlohmann::json& json);

    // Buffer loading
    bool load_buffer_data();
    bool load_external_buffer(GLTF::Buffer& buffer);
    
    // Data extraction helpers
    template<typename T>
    std::vector<T> extract_accessor_data(uint32_t accessor_index) const;
    
    bool extract_primitive_data(const GLTF::Primitive& primitive, 
                               std::vector<Vertex>& vertices, 
                               std::vector<uint32_t>& indices) const;
    
    std::vector<glm::vec3> extract_positions(uint32_t accessor_index) const;
    std::vector<glm::vec3> extract_normals(uint32_t accessor_index) const;
    std::vector<glm::vec2> extract_tex_coords(uint32_t accessor_index) const;
    std::vector<uint32_t> extract_indices(uint32_t accessor_index) const;
    
    // Validation
    bool validate_document() const;
    
    // Error handling
    void set_error(const std::string& message) const;
};

} // namespace CorePulse