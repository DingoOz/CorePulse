#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace CorePulse {

Shader::Shader() = default;

Shader::~Shader() {
    cleanup();
}

Shader::Shader(Shader&& other) noexcept : program_id_(other.program_id_) {
    other.program_id_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        cleanup();
        program_id_ = other.program_id_;
        other.program_id_ = 0;
    }
    return *this;
}

bool Shader::load_from_strings(const std::string& vertex_source, const std::string& fragment_source) {
    cleanup();
    
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (vertex_shader == 0) return false;
    
    GLuint fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    bool success = link_program(vertex_shader, fragment_shader);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return success;
}

bool Shader::load_from_files(const std::string& vertex_path, const std::string& fragment_path) {
    std::string vertex_source = read_file(vertex_path);
    std::string fragment_source = read_file(fragment_path);
    
    if (vertex_source.empty()) {
        std::cerr << "Failed to read vertex shader file: " << vertex_path << std::endl;
        return false;
    }
    
    if (fragment_source.empty()) {
        std::cerr << "Failed to read fragment shader file: " << fragment_path << std::endl;
        return false;
    }
    
    return load_from_strings(vertex_source, fragment_source);
}

void Shader::use() const {
    if (program_id_ != 0) {
        glUseProgram(program_id_);
    }
}

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::set_bool(const std::string& name, bool value) {
    glUniform1i(get_uniform_location(name), static_cast<int>(value));
}

void Shader::set_int(const std::string& name, int value) {
    glUniform1i(get_uniform_location(name), value);
}

void Shader::set_float(const std::string& name, float value) {
    glUniform1f(get_uniform_location(name), value);
}

void Shader::set_vec2(const std::string& name, const glm::vec2& value) {
    glUniform2fv(get_uniform_location(name), 1, glm::value_ptr(value));
}

void Shader::set_vec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(get_uniform_location(name), 1, glm::value_ptr(value));
}

void Shader::set_vec4(const std::string& name, const glm::vec4& value) {
    glUniform4fv(get_uniform_location(name), 1, glm::value_ptr(value));
}

void Shader::set_mat3(const std::string& name, const glm::mat3& value) {
    glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::set_mat4(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(value));
}

GLuint Shader::compile_shader(const std::string& source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);
    
    std::string type_name = (shader_type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    check_compile_errors(shader, type_name);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::link_program(GLuint vertex_shader, GLuint fragment_shader) {
    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vertex_shader);
    glAttachShader(program_id_, fragment_shader);
    glLinkProgram(program_id_);
    
    check_compile_errors(program_id_, "PROGRAM");
    
    GLint success;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
        return false;
    }
    
    return true;
}

GLint Shader::get_uniform_location(const std::string& name) const {
    auto it = uniform_locations_.find(name);
    if (it != uniform_locations_.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    uniform_locations_[name] = location;
    
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader" << std::endl;
    }
    
    return location;
}

void Shader::cleanup() {
    if (program_id_ != 0) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
    uniform_locations_.clear();
}

std::string Shader::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void Shader::check_compile_errors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar info_log[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "Shader compilation error (" << type << "):\n" << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "Shader program linking error:\n" << info_log << std::endl;
        }
    }
}

} // namespace CorePulse