#pragma once

#include "Texture.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace CorePulse {

// Material alpha modes following glTF specification
enum class AlphaMode {
    OPAQUE,
    MASK,
    BLEND
};

// PBR material following glTF 2.0 specification
class Material {
public:
    Material();
    ~Material() = default;
    
    // PBR Metallic-Roughness properties
    glm::vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    
    // Emissive properties
    glm::vec3 emissive_factor{0.0f, 0.0f, 0.0f};
    
    // Alpha properties
    AlphaMode alpha_mode = AlphaMode::OPAQUE;
    float alpha_cutoff = 0.5f;
    bool double_sided = false;
    
    // Textures
    std::shared_ptr<Texture> base_color_texture;
    std::shared_ptr<Texture> metallic_roughness_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> occlusion_texture;
    std::shared_ptr<Texture> emissive_texture;
    
    // Material properties
    std::string name;
    
    // Apply material to shader
    void apply_to_shader(Shader& shader) const;
    
    // Bind all textures
    void bind_textures() const;
    void unbind_textures() const;
    
    // Factory methods
    static std::shared_ptr<Material> create_default();
    static std::shared_ptr<Material> create_from_color(const glm::vec3& color);
    static std::shared_ptr<Material> create_metallic(const glm::vec3& color, float metallic, float roughness);
    
    // Validation
    bool is_valid() const;
    
private:
    void setup_default_textures();
    
    // Default fallback textures
    static std::shared_ptr<Texture> white_texture_;
    static std::shared_ptr<Texture> normal_texture_;
    static std::shared_ptr<Texture> black_texture_;
    
    static void ensure_default_textures();
};

} // namespace CorePulse