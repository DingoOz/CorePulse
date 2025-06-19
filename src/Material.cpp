#include "Material.h"
#include <iostream>

namespace CorePulse {

// Static member definitions
std::shared_ptr<Texture> Material::white_texture_;
std::shared_ptr<Texture> Material::normal_texture_;
std::shared_ptr<Texture> Material::black_texture_;

Material::Material() {
    ensure_default_textures();
    setup_default_textures();
}

void Material::apply_to_shader(Shader& shader) const {
    if (!shader.is_valid()) {
        std::cout << "Material: Shader not valid when applying material" << std::endl;
        return;
    }
    
    shader.use();
    
    static int debug_count = 0;
    if (debug_count < 3) {
        std::cout << "Applying material '" << name << "' - base color: (" 
                 << base_color_factor.r << ", " << base_color_factor.g << ", " 
                 << base_color_factor.b << ", " << base_color_factor.a << ")" << std::endl;
        std::cout << "  metallic: " << metallic_factor << ", roughness: " << roughness_factor << std::endl;
        std::cout << "  has textures - base: " << (base_color_texture != nullptr)
                 << ", metallic: " << (metallic_roughness_texture != nullptr)
                 << ", normal: " << (normal_texture != nullptr) << std::endl;
        debug_count++;
    }
    
    // Set PBR material properties
    shader.set_vec4("u_material.baseColorFactor", base_color_factor);
    shader.set_float("u_material.metallicFactor", metallic_factor);
    shader.set_float("u_material.roughnessFactor", roughness_factor);
    shader.set_vec3("u_material.emissiveFactor", emissive_factor);
    shader.set_float("u_material.alphaCutoff", alpha_cutoff);
    
    // Set alpha mode
    int alpha_mode_int = static_cast<int>(alpha_mode);
    shader.set_int("u_material.alphaMode", alpha_mode_int);
    shader.set_bool("u_material.doubleSided", double_sided);
    
    // Set texture flags
    shader.set_bool("u_material.hasBaseColorTexture", base_color_texture != nullptr);
    shader.set_bool("u_material.hasMetallicRoughnessTexture", metallic_roughness_texture != nullptr);
    shader.set_bool("u_material.hasNormalTexture", normal_texture != nullptr);
    shader.set_bool("u_material.hasOcclusionTexture", occlusion_texture != nullptr);
    shader.set_bool("u_material.hasEmissiveTexture", emissive_texture != nullptr);
    
    // Set texture units
    shader.set_int("u_material.baseColorTexture", 0);
    shader.set_int("u_material.metallicRoughnessTexture", 1);
    shader.set_int("u_material.normalTexture", 2);
    shader.set_int("u_material.occlusionTexture", 3);
    shader.set_int("u_material.emissiveTexture", 4);
    
    shader.unuse();
}

void Material::bind_textures() const {
    // Bind textures to their respective slots
    if (base_color_texture) {
        base_color_texture->bind(0);
    } else if (white_texture_) {
        white_texture_->bind(0);
    }
    
    if (metallic_roughness_texture) {
        metallic_roughness_texture->bind(1);
    } else if (white_texture_) {
        white_texture_->bind(1);
    }
    
    if (normal_texture) {
        normal_texture->bind(2);
    } else if (normal_texture_) {
        normal_texture_->bind(2);
    }
    
    if (occlusion_texture) {
        occlusion_texture->bind(3);
    } else if (white_texture_) {
        white_texture_->bind(3);
    }
    
    if (emissive_texture) {
        emissive_texture->bind(4);
    } else if (black_texture_) {
        black_texture_->bind(4);
    }
}

void Material::unbind_textures() const {
    // Unbind all texture slots
    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0); // Reset to default
}

std::shared_ptr<Material> Material::create_default() {
    auto material = std::make_shared<Material>();
    material->name = "Default Material";
    material->base_color_factor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    material->metallic_factor = 0.0f;
    material->roughness_factor = 0.9f;
    return material;
}

std::shared_ptr<Material> Material::create_from_color(const glm::vec3& color) {
    auto material = std::make_shared<Material>();
    material->name = "Color Material";
    material->base_color_factor = glm::vec4(color, 1.0f);
    material->metallic_factor = 0.0f;
    material->roughness_factor = 0.9f;
    return material;
}

std::shared_ptr<Material> Material::create_metallic(const glm::vec3& color, float metallic, float roughness) {
    auto material = std::make_shared<Material>();
    material->name = "Metallic Material";
    material->base_color_factor = glm::vec4(color, 1.0f);
    material->metallic_factor = metallic;
    material->roughness_factor = roughness;
    return material;
}

bool Material::is_valid() const {
    // Basic validation - check that factors are in valid ranges
    if (base_color_factor.r < 0.0f || base_color_factor.r > 1.0f ||
        base_color_factor.g < 0.0f || base_color_factor.g > 1.0f ||
        base_color_factor.b < 0.0f || base_color_factor.b > 1.0f ||
        base_color_factor.a < 0.0f || base_color_factor.a > 1.0f) {
        return false;
    }
    
    if (metallic_factor < 0.0f || metallic_factor > 1.0f) {
        return false;
    }
    
    if (roughness_factor < 0.0f || roughness_factor > 1.0f) {
        return false;
    }
    
    if (alpha_cutoff < 0.0f || alpha_cutoff > 1.0f) {
        return false;
    }
    
    return true;
}

void Material::setup_default_textures() {
    // Use default textures as fallbacks
    if (!base_color_texture) {
        base_color_texture = white_texture_;
    }
    
    if (!metallic_roughness_texture) {
        metallic_roughness_texture = white_texture_;
    }
    
    if (!normal_texture) {
        normal_texture = normal_texture_;
    }
    
    if (!occlusion_texture) {
        occlusion_texture = white_texture_;
    }
    
    if (!emissive_texture) {
        emissive_texture = black_texture_;
    }
}

void Material::ensure_default_textures() {
    if (!white_texture_) {
        white_texture_ = Texture::create_white_texture();
        if (!white_texture_) {
            std::cerr << "Failed to create default white texture" << std::endl;
        }
    }
    
    if (!normal_texture_) {
        normal_texture_ = Texture::create_normal_texture();
        if (!normal_texture_) {
            std::cerr << "Failed to create default normal texture" << std::endl;
        }
    }
    
    if (!black_texture_) {
        black_texture_ = Texture::create_black_texture();
        if (!black_texture_) {
            std::cerr << "Failed to create default black texture" << std::endl;
        }
    }
}

} // namespace CorePulse