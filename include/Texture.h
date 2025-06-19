#pragma once

#include <GL/glew.h>
#include <string>
#include <memory>

namespace CorePulse {

// Texture filtering modes
enum class TextureFilter {
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
    NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
};

// Texture wrapping modes
enum class TextureWrap {
    REPEAT = GL_REPEAT,
    MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
};

// Texture formats
enum class TextureFormat {
    RGB = GL_RGB,
    RGBA = GL_RGBA,
    RED = GL_RED,
    RG = GL_RG,
    BGR = GL_BGR,
    BGRA = GL_BGRA
};

class Texture {
public:
    Texture();
    ~Texture();
    
    // Non-copyable but movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    // Texture creation
    bool load_from_file(const std::string& filepath);
    bool create_from_data(const void* data, int width, int height, 
                         TextureFormat format = TextureFormat::RGBA,
                         GLenum data_type = GL_UNSIGNED_BYTE);
    
    // Texture parameters
    void set_min_filter(TextureFilter filter);
    void set_mag_filter(TextureFilter filter);
    void set_wrap_s(TextureWrap wrap);
    void set_wrap_t(TextureWrap wrap);
    void generate_mipmaps();
    
    // Texture usage
    void bind(GLuint slot = 0) const;
    void unbind() const;
    
    // Getters
    bool is_valid() const { return texture_id_ != 0; }
    GLuint get_id() const { return texture_id_; }
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_channels() const { return channels_; }
    const std::string& get_filepath() const { return filepath_; }
    
    // Static factory methods
    static std::shared_ptr<Texture> create_from_file(const std::string& filepath);
    static std::shared_ptr<Texture> create_white_texture();
    static std::shared_ptr<Texture> create_black_texture();
    static std::shared_ptr<Texture> create_normal_texture();
    
private:
    GLuint texture_id_ = 0;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::string filepath_;
    
    void cleanup();
    bool load_image_data(const std::string& filepath, unsigned char** data, 
                        int& width, int& height, int& channels);
};

} // namespace CorePulse