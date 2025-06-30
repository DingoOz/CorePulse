#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Mesh.h"
#include "Material.h"

namespace CorePulse {

struct TerrainConfig {
    // Terrain dimensions
    int width = 128;           // Number of vertices along X axis
    int depth = 128;           // Number of vertices along Z axis
    float scale = 2.0f;        // Size of each terrain quad in world units
    
    // Height generation
    float height_scale = 8.0f; // Maximum height variation
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
    explicit Terrain(const TerrainConfig& config = TerrainConfig{});
    ~Terrain() = default;
    
    // Terrain generation
    void generate();
    void regenerate(const TerrainConfig& new_config);
    
    // Accessors
    std::shared_ptr<Mesh> get_mesh() const { return mesh_; }
    std::shared_ptr<Material> get_material() const { return material_; }
    const TerrainConfig& get_config() const { return config_; }
    
    // Terrain queries
    float get_height_at(float world_x, float world_z) const;
    glm::vec3 get_normal_at(float world_x, float world_z) const;
    bool is_valid_position(float world_x, float world_z) const;
    
    // Terrain bounds
    glm::vec2 get_world_size() const;
    glm::vec3 get_world_center() const;
    
private:
    TerrainConfig config_;
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Material> material_;
    
    // Height data for queries
    std::vector<std::vector<float>> height_map_;
    
    // Generation helpers
    void generate_height_map();
    void generate_mesh();
    void generate_material();
    
    // Noise functions
    float generate_noise(float x, float z) const;
    float perlin_noise(float x, float z) const;
    float interpolate(float a, float b, float t) const;
    float fade(float t) const;
    float gradient(int hash, float x, float z) const;
    
    // Mesh generation helpers
    glm::vec3 calculate_normal(int x, int z) const;
    void add_quad(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                  int x, int z, unsigned int& index_offset) const;
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