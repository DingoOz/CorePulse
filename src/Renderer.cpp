#include "Renderer.h"
#include <iostream>
#include <GL/glew.h>

namespace CorePulse {

const std::string Renderer::default_vertex_shader_ = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;

void main() {
    frag_pos = vec3(u_model * vec4(a_position, 1.0));
    normal = u_normal_matrix * a_normal;
    tex_coords = a_tex_coords;
    
    gl_Position = u_projection * u_view * vec4(frag_pos, 1.0);
}
)";

const std::string Renderer::default_fragment_shader_ = R"(
#version 330 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform vec3 u_color = vec3(0.8, 0.8, 0.8);
uniform vec3 u_light_pos = vec3(5.0, 5.0, 5.0);
uniform vec3 u_light_color = vec3(1.0, 1.0, 1.0);
uniform vec3 u_view_pos = vec3(0.0, 0.0, 3.0);

void main() {
    // Ambient lighting
    float ambient_strength = 0.2;
    vec3 ambient = ambient_strength * u_light_color;
    
    // Diffuse lighting
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(u_light_pos - frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * u_light_color;
    
    // Specular lighting
    float specular_strength = 0.5;
    vec3 view_dir = normalize(u_view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * u_light_color;
    
    vec3 result = (ambient + diffuse + specular) * u_color;
    frag_color = vec4(result, 1.0);
}
)";

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (initialized_) {
        std::cout << "Renderer already initialized\n";
        return true;
    }
    
    // Initialize GLEW
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(glew_error) << std::endl;
        return false;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Check OpenGL version
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 or higher is required" << std::endl;
        return false;
    }
    
    // Create default shader
    if (!create_default_shader()) {
        std::cerr << "Failed to create default shader" << std::endl;
        return false;
    }
    
    // Set initial render state
    update_render_state();
    
    initialized_ = true;
    std::cout << "Renderer initialized successfully\n";
    return true;
}

void Renderer::shutdown() {
    if (!initialized_) return;
    
    default_shader_ = Shader{};
    initialized_ = false;
    std::cout << "Renderer shutdown complete\n";
}

void Renderer::begin_frame() {
    if (!initialized_) return;
    
    stats_.reset();
    update_render_state();
}

void Renderer::end_frame() {
    // Nothing specific needed for end frame currently
}

void Renderer::clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera) {
    render_mesh(mesh, model_matrix, camera, default_shader_);
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, Shader& shader) {
    if (!initialized_ || !mesh.is_valid() || !shader.is_valid()) return;
    
    shader.use();
    
    // Set matrices
    shader.set_mat4("u_model", model_matrix);
    shader.set_mat4("u_view", camera.get_view_matrix());
    shader.set_mat4("u_projection", camera.get_projection_matrix());
    
    // Calculate and set normal matrix
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
    shader.set_mat3("u_normal_matrix", normal_matrix);
    
    // Set camera position for lighting
    shader.set_vec3("u_view_pos", camera.get_position());
    
    // Render mesh
    mesh.draw();
    
    // Update statistics
    stats_.draw_calls++;
    stats_.vertices_rendered += mesh.get_vertex_count();
    stats_.triangles_rendered += mesh.get_index_count() / 3;
    
    shader.unuse();
}

void Renderer::set_wireframe_mode(bool enabled) {
    wireframe_mode_ = enabled;
    update_render_state();
}

void Renderer::set_depth_test(bool enabled) {
    depth_test_enabled_ = enabled;
    update_render_state();
}

void Renderer::set_face_culling(bool enabled) {
    face_culling_enabled_ = enabled;
    update_render_state();
}

bool Renderer::create_default_shader() {
    return default_shader_.load_from_strings(default_vertex_shader_, default_fragment_shader_);
}

void Renderer::update_render_state() {
    // Wireframe mode
    if (wireframe_mode_) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Depth testing
    if (depth_test_enabled_) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Face culling
    if (face_culling_enabled_) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

} // namespace CorePulse