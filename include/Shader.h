#pragma once

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CorePulse {

class Shader {
public:
    Shader();
    ~Shader();
    
    // Non-copyable but movable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    
    // Shader compilation
    bool load_from_strings(const std::string& vertex_source, const std::string& fragment_source);
    bool load_from_files(const std::string& vertex_path, const std::string& fragment_path);
    
    // Shader usage
    void use() const;
    void unuse() const;
    bool is_valid() const { return program_id_ != 0; }
    
    // Uniform setters
    void set_bool(const std::string& name, bool value);
    void set_int(const std::string& name, int value);
    void set_float(const std::string& name, float value);
    void set_vec2(const std::string& name, const glm::vec2& value);
    void set_vec3(const std::string& name, const glm::vec3& value);
    void set_vec4(const std::string& name, const glm::vec4& value);
    void set_mat3(const std::string& name, const glm::mat3& value);
    void set_mat4(const std::string& name, const glm::mat4& value);
    
    // Utility
    GLuint get_program_id() const { return program_id_; }
    
private:
    GLuint program_id_ = 0;
    mutable std::unordered_map<std::string, GLint> uniform_locations_;
    
    GLuint compile_shader(const std::string& source, GLenum shader_type);
    bool link_program(GLuint vertex_shader, GLuint fragment_shader);
    GLint get_uniform_location(const std::string& name) const;
    void cleanup();
    
    static std::string read_file(const std::string& filepath);
    static void check_compile_errors(GLuint shader, const std::string& type);
};

} // namespace CorePulse