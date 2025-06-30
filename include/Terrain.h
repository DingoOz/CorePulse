#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace CorePulse {

// Forward declarations
class Mesh;

class Terrain {
public:
    Terrain();
    ~Terrain() = default;
    
    // Initialize with procedural generation
    bool initialize(int width, int height, float scale = 1.0f, float height_scale = 5.0f);
    
    // Heightmap-based collision detection
    float get_height_at(float world_x, float world_z) const;
    glm::vec3 get_normal_at(float world_x, float world_z) const;
    
    // Terrain properties
    struct TerrainMaterial {
        float friction = 0.8f;
        float bounce = 0.3f;
        float drag = 0.1f;
    };
    
    const TerrainMaterial& get_material_at(float world_x, float world_z) const;
    
    // Mesh generation for rendering
    std::shared_ptr<Mesh> generate_mesh() const;
    
    // Getters
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    float get_scale() const { return scale_; }
    float get_height_scale() const { return height_scale_; }
    
    // Bounds checking
    bool is_in_bounds(float world_x, float world_z) const;
    glm::vec2 world_to_grid(float world_x, float world_z) const;
    glm::vec2 grid_to_world(int grid_x, int grid_z) const;

private:
    int width_ = 0;
    int height_ = 0;
    float scale_ = 1.0f;
    float height_scale_ = 5.0f;
    
    std::vector<float> heightmap_;
    TerrainMaterial default_material_;
    
    // Heightmap generation
    void generate_heightmap();
    void generate_hills_and_valleys();
    void add_noise(float amplitude, int frequency);
    
    // Utility functions
    float get_height_at_grid(int x, int z) const;
    void set_height_at_grid(int x, int z, float height);
    float bilinear_interpolate(float x, float z) const;
    
    // Simple noise function
    float simple_noise(float x, float z, int seed = 42) const;
    float fractal_noise(float x, float z, int octaves = 4) const;
};

} // namespace CorePulse