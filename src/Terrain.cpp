#include "Terrain.h"
#include "Mesh.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>

namespace CorePulse {

Terrain::Terrain() {
    default_material_.friction = 0.6f;
    default_material_.bounce = 0.4f;  // Reduced bounce to prevent oscillation
    default_material_.drag = 0.1f;    // Increased drag to settle bouncing
}

bool Terrain::initialize(int width, int height, float scale, float height_scale) {
    width_ = width;
    height_ = height;
    scale_ = scale;
    height_scale_ = height_scale;
    
    // Allocate heightmap
    heightmap_.resize(width_ * height_);
    
    // Generate procedural terrain
    generate_heightmap();
    
    std::cout << "Terrain: Initialized " << width_ << "x" << height_ 
              << " terrain with scale=" << scale_ << ", height_scale=" << height_scale_ << std::endl;
    
    return true;
}

float Terrain::get_height_at(float world_x, float world_z) const {
    if (!is_in_bounds(world_x, world_z)) {
        return 0.0f; // Return ground level for out-of-bounds
    }
    
    // Convert world coordinates to grid coordinates
    glm::vec2 grid_pos = world_to_grid(world_x, world_z);
    
    // Use bilinear interpolation for smooth height
    return bilinear_interpolate(grid_pos.x, grid_pos.y) * height_scale_;
}

glm::vec3 Terrain::get_normal_at(float world_x, float world_z) const {
    const float epsilon = 0.1f; // Small offset for calculating gradient
    
    // Sample heights at nearby points
    float h_center = get_height_at(world_x, world_z);
    float h_right = get_height_at(world_x + epsilon, world_z);
    float h_up = get_height_at(world_x, world_z + epsilon);
    
    // Calculate tangent vectors
    glm::vec3 tangent_x(epsilon, h_right - h_center, 0.0f);
    glm::vec3 tangent_z(0.0f, h_up - h_center, epsilon);
    
    // Cross product gives normal
    glm::vec3 normal = glm::normalize(glm::cross(tangent_x, tangent_z));
    
    return normal;
}

const Terrain::TerrainMaterial& Terrain::get_material_at(float world_x, float world_z) const {
    // For now, return default material everywhere
    // In a full implementation, this could vary based on terrain type/texture
    return default_material_;
}

std::shared_ptr<Mesh> Terrain::generate_mesh() const {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate vertices
    vertices.reserve(width_ * height_);
    for (int z = 0; z < height_; ++z) {
        for (int x = 0; x < width_; ++x) {
            glm::vec2 world_pos = grid_to_world(x, z);
            float height = get_height_at_grid(x, z) * height_scale_;
            
            Vertex vertex;
            vertex.position = glm::vec3(world_pos.x, height, world_pos.y);
            vertex.normal = get_normal_at(world_pos.x, world_pos.y);
            vertex.tex_coords = glm::vec2(
                static_cast<float>(x) / (width_ - 1),
                static_cast<float>(z) / (height_ - 1)
            );
            
            vertices.push_back(vertex);
        }
    }
    
    // Generate indices for triangulated terrain
    indices.reserve((width_ - 1) * (height_ - 1) * 6);
    for (int z = 0; z < height_ - 1; ++z) {
        for (int x = 0; x < width_ - 1; ++x) {
            uint32_t base = z * width_ + x;
            
            // First triangle (top-left)
            indices.push_back(base);
            indices.push_back(base + width_);
            indices.push_back(base + 1);
            
            // Second triangle (bottom-right)
            indices.push_back(base + 1);
            indices.push_back(base + width_);
            indices.push_back(base + width_ + 1);
        }
    }
    
    // Create mesh
    auto mesh = std::make_shared<Mesh>();
    if (!mesh->create(vertices, indices)) {
        std::cerr << "Terrain: Failed to create mesh" << std::endl;
        return nullptr;
    }
    
    return mesh;
}

bool Terrain::is_in_bounds(float world_x, float world_z) const {
    glm::vec2 grid_pos = world_to_grid(world_x, world_z);
    return grid_pos.x >= 0.0f && grid_pos.x < width_ - 1 &&
           grid_pos.y >= 0.0f && grid_pos.y < height_ - 1;
}

glm::vec2 Terrain::world_to_grid(float world_x, float world_z) const {
    return glm::vec2(
        (world_x / scale_) + (width_ * 0.5f),
        (world_z / scale_) + (height_ * 0.5f)
    );
}

glm::vec2 Terrain::grid_to_world(int grid_x, int grid_z) const {
    return glm::vec2(
        (grid_x - width_ * 0.5f) * scale_,
        (grid_z - height_ * 0.5f) * scale_
    );
}

void Terrain::generate_heightmap() {
    // Fill with base height
    std::fill(heightmap_.begin(), heightmap_.end(), 0.0f);
    
    // Generate hills and valleys
    generate_hills_and_valleys();
    
    // Add noise for detail
    add_noise(0.1f, 8);  // High frequency detail
    add_noise(0.3f, 4);  // Medium frequency features
    add_noise(0.5f, 2);  // Low frequency large features
}

void Terrain::generate_hills_and_valleys() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(0.2f, 0.8f);
    std::uniform_real_distribution<float> height_dist(0.3f, 1.0f);
    std::uniform_real_distribution<float> radius_dist(0.1f, 0.3f);
    
    // Generate several hills
    int num_features = 6;
    for (int i = 0; i < num_features; ++i) {
        float center_x = pos_dist(gen) * width_;
        float center_z = pos_dist(gen) * height_;
        float max_height = height_dist(gen);
        float radius = radius_dist(gen) * std::min(width_, height_);
        
        bool is_hill = (i % 2 == 0); // Alternate between hills and valleys
        if (!is_hill) max_height *= -0.5f; // Make valleys shallower
        
        // Apply Gaussian-like falloff
        for (int z = 0; z < height_; ++z) {
            for (int x = 0; x < width_; ++x) {
                float dx = x - center_x;
                float dz = z - center_z;
                float distance = std::sqrt(dx * dx + dz * dz);
                
                if (distance < radius) {
                    float factor = std::exp(-(distance * distance) / (radius * radius * 0.3f));
                    float current_height = get_height_at_grid(x, z);
                    set_height_at_grid(x, z, current_height + max_height * factor);
                }
            }
        }
    }
}

void Terrain::add_noise(float amplitude, int frequency) {
    for (int z = 0; z < height_; ++z) {
        for (int x = 0; x < width_; ++x) {
            float noise_value = fractal_noise(
                x * frequency / static_cast<float>(width_),
                z * frequency / static_cast<float>(height_)
            );
            
            float current_height = get_height_at_grid(x, z);
            set_height_at_grid(x, z, current_height + noise_value * amplitude);
        }
    }
}

float Terrain::get_height_at_grid(int x, int z) const {
    if (x < 0 || x >= width_ || z < 0 || z >= height_) {
        return 0.0f;
    }
    return heightmap_[z * width_ + x];
}

void Terrain::set_height_at_grid(int x, int z, float height) {
    if (x >= 0 && x < width_ && z >= 0 && z < height_) {
        heightmap_[z * width_ + x] = height;
    }
}

float Terrain::bilinear_interpolate(float x, float z) const {
    // Get integer coordinates
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));
    int x1 = x0 + 1;
    int z1 = z0 + 1;
    
    // Get fractional parts
    float fx = x - x0;
    float fz = z - z0;
    
    // Sample heightmap at corners
    float h00 = get_height_at_grid(x0, z0);
    float h10 = get_height_at_grid(x1, z0);
    float h01 = get_height_at_grid(x0, z1);
    float h11 = get_height_at_grid(x1, z1);
    
    // Bilinear interpolation
    float h_top = h00 * (1.0f - fx) + h10 * fx;
    float h_bottom = h01 * (1.0f - fx) + h11 * fx;
    
    return h_top * (1.0f - fz) + h_bottom * fz;
}

float Terrain::simple_noise(float x, float z, int seed) const {
    // Simple pseudo-random noise
    int ix = static_cast<int>(std::floor(x * 1000)) + seed;
    int iz = static_cast<int>(std::floor(z * 1000)) + seed;
    
    // Hash the coordinates
    int hash = (ix * 374761393 + iz * 668265263) ^ seed;
    hash = (hash << 13) ^ hash;
    hash = (hash * (hash * hash * 60493 + 19990303) + 1376312589) & 0x7fffffff;
    
    return (hash / 1073741824.0f) - 1.0f; // Normalize to [-1, 1]
}

float Terrain::fractal_noise(float x, float z, int octaves) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        result += simple_noise(x * frequency, z * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return result / max_value; // Normalize
}

} // namespace CorePulse