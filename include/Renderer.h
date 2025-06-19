#pragma once

#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Material.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace CorePulse {

struct RenderStats {
    size_t draw_calls = 0;
    size_t vertices_rendered = 0;
    size_t triangles_rendered = 0;
    
    void reset() {
        draw_calls = 0;
        vertices_rendered = 0;
        triangles_rendered = 0;
    }
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Non-copyable but movable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = default;
    Renderer& operator=(Renderer&&) = default;
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Frame management
    void begin_frame();
    void end_frame();
    void clear(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.2f, 1.0f));
    
    // Rendering
    void render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera);
    void render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, Shader& shader);
    void render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, const Material& material);
    void render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, const Material& material, Shader& shader);
    
    // Wireframe mode
    void set_wireframe_mode(bool enabled);
    bool is_wireframe_mode() const { return wireframe_mode_; }
    
    // Depth testing
    void set_depth_test(bool enabled);
    bool is_depth_test_enabled() const { return depth_test_enabled_; }
    
    // Face culling
    void set_face_culling(bool enabled);
    bool is_face_culling_enabled() const { return face_culling_enabled_; }
    
    // Statistics
    const RenderStats& get_stats() const { return stats_; }
    void reset_stats() { stats_.reset(); }
    
    // Shaders
    Shader& get_default_shader() { return default_shader_; }
    const Shader& get_default_shader() const { return default_shader_; }
    Shader& get_pbr_shader() { return pbr_shader_; }
    const Shader& get_pbr_shader() const { return pbr_shader_; }
    
private:
    bool initialized_ = false;
    Shader default_shader_;
    Shader pbr_shader_;
    RenderStats stats_;
    
    // Render state
    bool wireframe_mode_ = false;
    bool depth_test_enabled_ = true;
    bool face_culling_enabled_ = true;
    
    bool create_default_shader();
    bool create_pbr_shader();
    void update_render_state();
    
    // Default shader sources
    static const std::string default_vertex_shader_;
    static const std::string default_fragment_shader_;
    
    // PBR shader sources
    static const std::string pbr_vertex_shader_;
    static const std::string pbr_fragment_shader_;
};

} // namespace CorePulse