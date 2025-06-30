#include "Texture.h"
#include <iostream>
#include <fstream>
#include <filesystem>

// Use stb_image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

namespace CorePulse {

Texture::Texture() = default;

Texture::~Texture() {
    cleanup();
}

Texture::Texture(Texture&& other) noexcept
    : texture_id_(other.texture_id_)
    , width_(other.width_)
    , height_(other.height_)
    , channels_(other.channels_)
    , filepath_(std::move(other.filepath_)) {
    other.texture_id_ = 0;
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        texture_id_ = other.texture_id_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        filepath_ = std::move(other.filepath_);
        
        other.texture_id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

bool Texture::load_from_file(const std::string& filepath) {
    // Check if file exists
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "Texture file not found: " << filepath << std::endl;
        return false;
    }
    
    // Load image data
    unsigned char* data = nullptr;
    int width, height, channels;
    
    if (!load_image_data(filepath, &data, width, height, channels)) {
        return false;
    }
    
    // Determine format based on channels
    TextureFormat format = TextureFormat::RGBA;
    switch (channels) {
        case 1: format = TextureFormat::RED; break;
        case 2: format = TextureFormat::RG; break;
        case 3: format = TextureFormat::RGB; break;
        case 4: format = TextureFormat::RGBA; break;
        default:
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
            stbi_image_free(data);
            return false;
    }
    
    // Create texture from data
    bool success = create_from_data(data, width, height, format);
    
    // Store filepath for reference
    if (success) {
        filepath_ = filepath;
        std::cout << "Loaded texture: " << filepath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    }
    
    // Free image data
    stbi_image_free(data);
    
    return success;
}

bool Texture::create_from_data(const void* data, int width, int height, 
                              TextureFormat format, GLenum data_type) {
    cleanup();
    
    glGenTextures(1, &texture_id_);
    if (texture_id_ == 0) {
        std::cerr << "Failed to generate texture" << std::endl;
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, 
                 static_cast<GLenum>(format), data_type, data);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during texture creation: " << error << std::endl;
        cleanup();
        return false;
    }
    
    // Set default parameters
    set_min_filter(TextureFilter::LINEAR);
    set_mag_filter(TextureFilter::LINEAR);
    set_wrap_s(TextureWrap::REPEAT);
    set_wrap_t(TextureWrap::REPEAT);
    
    // Generate mipmaps if needed
    generate_mipmaps();
    
    width_ = width;
    height_ = height;
    
    // Determine channels from format
    switch (format) {
        case TextureFormat::RED: channels_ = 1; break;
        case TextureFormat::RG: channels_ = 2; break;
        case TextureFormat::RGB:
        case TextureFormat::BGR: channels_ = 3; break;
        case TextureFormat::RGBA:
        case TextureFormat::BGRA: channels_ = 4; break;
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture::set_min_filter(TextureFilter filter) {
    if (!is_valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set_mag_filter(TextureFilter filter) {
    if (!is_valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set_wrap_s(TextureWrap wrap) {
    if (!is_valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set_wrap_t(TextureWrap wrap) {
    if (!is_valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::generate_mipmaps() {
    if (!is_valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(GLuint slot) const {
    if (!is_valid()) return;
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::shared_ptr<Texture> Texture::create_from_file(const std::string& filepath) {
    auto texture = std::make_shared<Texture>();
    if (texture->load_from_file(filepath)) {
        return texture;
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::create_white_texture() {
    auto texture = std::make_shared<Texture>();
    uint32_t white_pixel = 0xFFFFFFFF; // RGBA white
    if (texture->create_from_data(&white_pixel, 1, 1, TextureFormat::RGBA)) {
        return texture;
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::create_black_texture() {
    auto texture = std::make_shared<Texture>();
    uint32_t black_pixel = 0x000000FF; // RGBA black with full alpha
    if (texture->create_from_data(&black_pixel, 1, 1, TextureFormat::RGBA)) {
        return texture;
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::create_normal_texture() {
    auto texture = std::make_shared<Texture>();
    uint32_t normal_pixel = 0xFF8080FF; // RGB(128, 128, 255) = normal pointing up
    if (texture->create_from_data(&normal_pixel, 1, 1, TextureFormat::RGBA)) {
        return texture;
    }
    return nullptr;
}

void Texture::cleanup() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    width_ = 0;
    height_ = 0;
    channels_ = 0;
    filepath_.clear();
}

bool Texture::load_image_data(const std::string& filepath, unsigned char** data, 
                             int& width, int& height, int& channels) {
    // Flip images vertically for OpenGL coordinate system
    stbi_set_flip_vertically_on_load(true);
    
    *data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (*data == nullptr) {
        std::cerr << "Failed to load texture: " << filepath << " - " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    return true;
}

} // namespace CorePulse