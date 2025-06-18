#include "Mesh.h"
#include <iostream>
#include <cmath>

namespace CorePulse {

Mesh::Mesh() = default;

Mesh::~Mesh() {
    cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept 
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_)
    , vertex_count_(other.vertex_count_), index_count_(other.index_count_) {
    other.vao_ = other.vbo_ = other.ebo_ = 0;
    other.vertex_count_ = other.index_count_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        ebo_ = other.ebo_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        other.vao_ = other.vbo_ = other.ebo_ = 0;
        other.vertex_count_ = other.index_count_ = 0;
    }
    return *this;
}

bool Mesh::create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    cleanup();
    
    vertex_count_ = vertices.size();
    index_count_ = indices.size();
    
    // Generate buffers
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    // Bind VAO
    glBindVertexArray(vao_);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    
    // Setup vertex attributes
    setup_vertex_attributes();
    
    // Unbind
    glBindVertexArray(0);
    
    return true;
}

bool Mesh::create(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
    // Assume vertex format: position(3) + normal(3) + texcoords(2) = 8 floats per vertex
    if (vertices.size() % 8 != 0) {
        std::cerr << "Invalid vertex data: expected 8 floats per vertex (pos3+norm3+tex2)" << std::endl;
        return false;
    }
    
    std::vector<Vertex> structured_vertices;
    structured_vertices.reserve(vertices.size() / 8);
    
    for (size_t i = 0; i < vertices.size(); i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i+1], vertices[i+2]);
        vertex.normal = glm::vec3(vertices[i+3], vertices[i+4], vertices[i+5]);
        vertex.tex_coords = glm::vec2(vertices[i+6], vertices[i+7]);
        structured_vertices.push_back(vertex);
    }
    
    return create(structured_vertices, indices);
}

void Mesh::draw() const {
    if (!is_valid()) return;
    
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count_), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::bind() const {
    if (vao_ != 0) {
        glBindVertexArray(vao_);
    }
}

void Mesh::unbind() const {
    glBindVertexArray(0);
}

void Mesh::setup_vertex_attributes() {
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
    glEnableVertexAttribArray(2);
}

void Mesh::cleanup() {
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    vertex_count_ = index_count_ = 0;
}

Mesh Mesh::create_cube(float size) {
    float half_size = size * 0.5f;
    
    std::vector<Vertex> vertices = {
        // Front face
        {{-half_size, -half_size,  half_size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ half_size, -half_size,  half_size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ half_size,  half_size,  half_size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-half_size,  half_size,  half_size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Back face
        {{-half_size, -half_size, -half_size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-half_size,  half_size, -half_size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{ half_size,  half_size, -half_size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{ half_size, -half_size, -half_size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        
        // Left face
        {{-half_size,  half_size,  half_size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-half_size,  half_size, -half_size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-half_size, -half_size, -half_size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-half_size, -half_size,  half_size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        
        // Right face
        {{ half_size,  half_size,  half_size}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ half_size, -half_size,  half_size}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ half_size, -half_size, -half_size}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ half_size,  half_size, -half_size}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        
        // Top face
        {{-half_size,  half_size, -half_size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-half_size,  half_size,  half_size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ half_size,  half_size,  half_size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ half_size,  half_size, -half_size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        
        // Bottom face
        {{-half_size, -half_size, -half_size}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ half_size, -half_size, -half_size}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ half_size, -half_size,  half_size}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-half_size, -half_size,  half_size}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}
    };
    
    std::vector<uint32_t> indices = {
        0,  1,  2,   0,  2,  3,   // Front
        4,  5,  6,   4,  6,  7,   // Back
        8,  9,  10,  8,  10, 11,  // Left
        12, 13, 14,  12, 14, 15,  // Right
        16, 17, 18,  16, 18, 19,  // Top
        20, 21, 22,  20, 22, 23   // Bottom
    };
    
    Mesh mesh;
    mesh.create(vertices, indices);
    return mesh;
}

Mesh Mesh::create_plane(float width, float height) {
    float half_width = width * 0.5f;
    float half_height = height * 0.5f;
    
    std::vector<Vertex> vertices = {
        {{-half_width, 0.0f, -half_height}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ half_width, 0.0f, -half_height}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ half_width, 0.0f,  half_height}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-half_width, 0.0f,  half_height}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2,  0, 2, 3
    };
    
    Mesh mesh;
    mesh.create(vertices, indices);
    return mesh;
}

Mesh Mesh::create_sphere(float radius, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate vertices
    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * M_PI / segments;
        float sin_theta = sin(theta);
        float cos_theta = cos(theta);
        
        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * M_PI / segments;
            float sin_phi = sin(phi);
            float cos_phi = cos(phi);
            
            glm::vec3 position(
                radius * sin_theta * cos_phi,
                radius * cos_theta,
                radius * sin_theta * sin_phi
            );
            
            glm::vec3 normal = glm::normalize(position);
            glm::vec2 tex_coords(
                static_cast<float>(lon) / segments,
                static_cast<float>(lat) / segments
            );
            
            vertices.emplace_back(position, normal, tex_coords);
        }
    }
    
    // Generate indices
    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int first = lat * (segments + 1) + lon;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    Mesh mesh;
    mesh.create(vertices, indices);
    return mesh;
}

Mesh Mesh::create_cylinder(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    float half_height = height * 0.5f;
    
    // Top and bottom centers
    vertices.emplace_back(glm::vec3(0.0f, half_height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(0.0f, -half_height, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f));
    
    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * M_PI / segments;
        float cos_angle = cos(angle);
        float sin_angle = sin(angle);
        
        glm::vec3 position_top(radius * cos_angle, half_height, radius * sin_angle);
        glm::vec3 position_bottom(radius * cos_angle, -half_height, radius * sin_angle);
        glm::vec3 normal(cos_angle, 0.0f, sin_angle);
        glm::vec2 tex_coords(static_cast<float>(i) / segments, 0.0f);
        glm::vec2 tex_coords_bottom(static_cast<float>(i) / segments, 1.0f);
        
        vertices.emplace_back(position_top, normal, tex_coords);
        vertices.emplace_back(position_bottom, normal, tex_coords_bottom);
    }
    
    // Top cap indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(2 + i * 2);
        indices.push_back(2 + ((i + 1) % segments) * 2);
    }
    
    // Bottom cap indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(1);
        indices.push_back(3 + ((i + 1) % segments) * 2);
        indices.push_back(3 + i * 2);
    }
    
    // Side indices
    for (int i = 0; i < segments; ++i) {
        int current = 2 + i * 2;
        int next = 2 + ((i + 1) % segments) * 2;
        
        indices.push_back(current);
        indices.push_back(current + 1);
        indices.push_back(next);
        
        indices.push_back(next);
        indices.push_back(current + 1);
        indices.push_back(next + 1);
    }
    
    Mesh mesh;
    mesh.create(vertices, indices);
    return mesh;
}

} // namespace CorePulse