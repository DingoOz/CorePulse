#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace CorePulse {

// Forward declarations
class Mesh;

struct TerrainConfig {
    // Terrain dimensions
    int width = 32;            // Number of vertices along X axis
    int depth = 32;            // Number of vertices along Z axis
    float scale = 1.0f;        // Size of each terrain quad in world units
    
    // Height generation
    float height_scale = 3.0f; // Maximum height variation
    float noise_frequency = 0.1f; // Frequency of noise patterns
    int octaves = 4;           // Number of noise octaves for detail
    float persistence = 0.5f;  // Amplitude reduction per octave
    float lacunarity = 2.0f;   // Frequency increase per octave
    
    // Terrain features
    bool generate_normals = true;
    bool generate_texcoords = true;
    
    // Material properties
    glm::vec3 base_color = glm::vec3(0.3f, 0.7f, 0.2f); // Grass green
    float roughness = 0.8f;
    float metallic = 0.0f;
};

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
    
public:
    // New config-based methods
    void regenerate(const TerrainConfig& config);
    const TerrainConfig& get_config() const { return config_; }
    
private:
    TerrainConfig config_;
};

// Procedural landscape generator for creating varied terrains
class LandscapeGenerator {
public:
    // Generate different types of landscapes
    static TerrainConfig create_flat_plains();
    static TerrainConfig create_rolling_hills();
    static TerrainConfig create_mountainous();
    static TerrainConfig create_desert_dunes();
    static TerrainConfig create_battlefield();
    
    // Create a random landscape
    static TerrainConfig create_random();
};

} // namespace CorePulse