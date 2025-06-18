#pragma once

#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>

namespace CorePulse {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    
    Vertex() = default;
    Vertex(const glm::vec3& pos, const glm::vec3& norm = glm::vec3(0.0f), const glm::vec2& tex = glm::vec2(0.0f))
        : position(pos), normal(norm), tex_coords(tex) {}
};

class Mesh {
public:
    Mesh();
    ~Mesh();
    
    // Non-copyable but movable
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    
    // Mesh creation
    bool create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    bool create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
    
    // Rendering
    void draw() const;
    void bind() const;
    void unbind() const;
    
    // Getters
    bool is_valid() const { return vao_ != 0; }
    size_t get_vertex_count() const { return vertex_count_; }
    size_t get_index_count() const { return index_count_; }
    
    // Static factory methods for common primitives
    static Mesh create_cube(float size = 1.0f);
    static Mesh create_plane(float width = 1.0f, float height = 1.0f);
    static Mesh create_sphere(float radius = 1.0f, int segments = 32);
    static Mesh create_cylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    
private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    
    size_t vertex_count_ = 0;
    size_t index_count_ = 0;
    
    void setup_vertex_attributes();
    void cleanup();
};

} // namespace CorePulse