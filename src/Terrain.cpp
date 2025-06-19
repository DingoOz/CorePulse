#include "Terrain.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

namespace CorePulse {

Terrain::Terrain(const TerrainConfig& config) : config_(config) {
    generate();
}

void Terrain::generate() {
    std::cout << "Terrain: Generating " << config_.width << "x" << config_.depth 
              << " terrain with scale " << config_.scale << std::endl;
    
    generate_height_map();
    generate_mesh();
    generate_material();
    
    std::cout << "Terrain: Generation complete" << std::endl;
}

void Terrain::regenerate(const TerrainConfig& new_config) {
    config_ = new_config;
    generate();
}

void Terrain::generate_height_map() {
    // Initialize height map
    height_map_.clear();
    height_map_.resize(config_.width, std::vector<float>(config_.depth, 0.0f));
    
    // Generate heights using multi-octave noise
    for (int x = 0; x < config_.width; ++x) {
        for (int z = 0; z < config_.depth; ++z) {
            float height = 0.0f;
            float amplitude = 1.0f;
            float frequency = config_.noise_frequency;
            
            // Sum multiple octaves of noise
            for (int octave = 0; octave < config_.octaves; ++octave) {
                float noise_x = x * frequency;
                float noise_z = z * frequency;
                
                float noise_value = generate_noise(noise_x, noise_z);
                height += noise_value * amplitude;
                
                amplitude *= config_.persistence;
                frequency *= config_.lacunarity;
            }
            
            // Scale and store height
            height_map_[x][z] = height * config_.height_scale;
        }
    }
}

void Terrain::generate_mesh() {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    // Reserve space for efficiency
    vertices.reserve(config_.width * config_.depth * 8); // pos(3) + normal(3) + texcoord(2)
    indices.reserve((config_.width - 1) * (config_.depth - 1) * 6);
    
    // Generate vertices
    for (int z = 0; z < config_.depth; ++z) {
        for (int x = 0; x < config_.width; ++x) {
            // World position
            float world_x = (x - config_.width * 0.5f) * config_.scale;
            float world_z = (z - config_.depth * 0.5f) * config_.scale;
            float world_y = height_map_[x][z];
            
            // Position
            vertices.push_back(world_x);
            vertices.push_back(world_y);
            vertices.push_back(world_z);
            
            // Normal
            if (config_.generate_normals) {
                glm::vec3 normal = calculate_normal(x, z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            } else {
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
            
            // Texture coordinates
            if (config_.generate_texcoords) {
                vertices.push_back(static_cast<float>(x) / (config_.width - 1));
                vertices.push_back(static_cast<float>(z) / (config_.depth - 1));
            } else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }
    }
    
    // Generate indices for triangles
    for (int z = 0; z < config_.depth - 1; ++z) {
        for (int x = 0; x < config_.width - 1; ++x) {
            uint32_t top_left = z * config_.width + x;
            uint32_t top_right = top_left + 1;
            uint32_t bottom_left = (z + 1) * config_.width + x;
            uint32_t bottom_right = bottom_left + 1;
            
            // First triangle (top-left, bottom-left, top-right)
            indices.push_back(top_left);
            indices.push_back(bottom_left);
            indices.push_back(top_right);
            
            // Second triangle (top-right, bottom-left, bottom-right)
            indices.push_back(top_right);
            indices.push_back(bottom_left);
            indices.push_back(bottom_right);
        }
    }
    
    // Create mesh
    mesh_ = std::make_shared<Mesh>();
    if (!mesh_->create(vertices, indices)) {
        std::cerr << "Terrain: Failed to create mesh" << std::endl;
    }
}

void Terrain::generate_material() {
    material_ = Material::create_default();
    material_->name = "Terrain Material";
    material_->base_color_factor = glm::vec4(config_.base_color, 1.0f);
    material_->roughness_factor = config_.roughness;
    material_->metallic_factor = config_.metallic;
}

float Terrain::get_height_at(float world_x, float world_z) const {
    // Convert world coordinates to terrain coordinates
    float terrain_x = (world_x / config_.scale) + (config_.width * 0.5f);
    float terrain_z = (world_z / config_.scale) + (config_.depth * 0.5f);
    
    // Check bounds
    if (terrain_x < 0 || terrain_x >= config_.width - 1 ||
        terrain_z < 0 || terrain_z >= config_.depth - 1) {
        return 0.0f; // Return 0 height for out-of-bounds positions
    }
    
    // Get integer coordinates
    int x0 = static_cast<int>(terrain_x);
    int z0 = static_cast<int>(terrain_z);
    int x1 = std::min(x0 + 1, config_.width - 1);
    int z1 = std::min(z0 + 1, config_.depth - 1);
    
    // Get fractional parts for interpolation
    float fx = terrain_x - x0;
    float fz = terrain_z - z0;
    
    // Bilinear interpolation
    float h00 = height_map_[x0][z0];
    float h10 = height_map_[x1][z0];
    float h01 = height_map_[x0][z1];
    float h11 = height_map_[x1][z1];
    
    float h_top = interpolate(h00, h10, fx);
    float h_bottom = interpolate(h01, h11, fx);
    
    return interpolate(h_top, h_bottom, fz);
}

glm::vec3 Terrain::get_normal_at(float world_x, float world_z) const {
    // Simple normal calculation using height samples
    const float offset = config_.scale * 0.1f;
    
    float height_left = get_height_at(world_x - offset, world_z);
    float height_right = get_height_at(world_x + offset, world_z);
    float height_down = get_height_at(world_x, world_z - offset);
    float height_up = get_height_at(world_x, world_z + offset);
    
    glm::vec3 normal;
    normal.x = height_left - height_right;
    normal.y = 2.0f * offset;
    normal.z = height_down - height_up;
    
    return glm::normalize(normal);
}

bool Terrain::is_valid_position(float world_x, float world_z) const {
    float terrain_x = (world_x / config_.scale) + (config_.width * 0.5f);
    float terrain_z = (world_z / config_.scale) + (config_.depth * 0.5f);
    
    return terrain_x >= 0 && terrain_x < config_.width &&
           terrain_z >= 0 && terrain_z < config_.depth;
}

glm::vec2 Terrain::get_world_size() const {
    return glm::vec2(config_.width * config_.scale, config_.depth * config_.scale);
}

glm::vec3 Terrain::get_world_center() const {
    return glm::vec3(0.0f, config_.height_scale * 0.5f, 0.0f);
}

// Noise generation functions
float Terrain::generate_noise(float x, float z) const {
    return perlin_noise(x, z);
}

float Terrain::perlin_noise(float x, float z) const {
    // Simplified 2D Perlin noise implementation
    int xi = static_cast<int>(std::floor(x)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;
    
    float xf = x - std::floor(x);
    float zf = z - std::floor(z);
    
    float u = fade(xf);
    float v = fade(zf);
    
    // Hash coordinates of the 4 cube corners
    static const int permutation[] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
    
    auto hash = [&](int i) -> int {
        return permutation[i % 256];
    };
    
    int aa = hash(xi) + zi;
    int ab = hash(xi) + zi + 1;
    int ba = hash(xi + 1) + zi;
    int bb = hash(xi + 1) + zi + 1;
    
    float grad_aa = gradient(hash(aa), xf, zf);
    float grad_ba = gradient(hash(ba), xf - 1, zf);
    float grad_ab = gradient(hash(ab), xf, zf - 1);
    float grad_bb = gradient(hash(bb), xf - 1, zf - 1);
    
    float lerp1 = interpolate(grad_aa, grad_ba, u);
    float lerp2 = interpolate(grad_ab, grad_bb, u);
    
    return interpolate(lerp1, lerp2, v);
}

float Terrain::interpolate(float a, float b, float t) const {
    return a + t * (b - a);
}

float Terrain::fade(float t) const {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float Terrain::gradient(int hash, float x, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : z;
    float v = h < 4 ? z : (h == 12 || h == 14 ? x : 0);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

glm::vec3 Terrain::calculate_normal(int x, int z) const {
    // Calculate normal using neighboring heights
    float height_left = (x > 0) ? height_map_[x-1][z] : height_map_[x][z];
    float height_right = (x < config_.width-1) ? height_map_[x+1][z] : height_map_[x][z];
    float height_down = (z > 0) ? height_map_[x][z-1] : height_map_[x][z];
    float height_up = (z < config_.depth-1) ? height_map_[x][z+1] : height_map_[x][z];
    
    glm::vec3 normal;
    normal.x = (height_left - height_right) / (2.0f * config_.scale);
    normal.y = 1.0f;
    normal.z = (height_down - height_up) / (2.0f * config_.scale);
    
    return glm::normalize(normal);
}

// Landscape Generator implementations
TerrainConfig LandscapeGenerator::create_flat_plains() {
    TerrainConfig config;
    config.width = 64;
    config.depth = 64;
    config.scale = 4.0f;
    config.height_scale = 1.0f;
    config.noise_frequency = 0.02f;
    config.octaves = 2;
    config.base_color = glm::vec3(0.4f, 0.8f, 0.3f); // Bright grass green
    return config;
}

TerrainConfig LandscapeGenerator::create_rolling_hills() {
    TerrainConfig config;
    config.width = 96;
    config.depth = 96;
    config.scale = 3.0f;
    config.height_scale = 12.0f;
    config.noise_frequency = 0.05f;
    config.octaves = 3;
    config.persistence = 0.6f;
    config.base_color = glm::vec3(0.3f, 0.7f, 0.2f); // Rolling hills green
    return config;
}

TerrainConfig LandscapeGenerator::create_mountainous() {
    TerrainConfig config;
    config.width = 128;
    config.depth = 128;
    config.scale = 2.0f;
    config.height_scale = 25.0f;
    config.noise_frequency = 0.08f;
    config.octaves = 5;
    config.persistence = 0.7f;
    config.lacunarity = 2.2f;
    config.base_color = glm::vec3(0.5f, 0.5f, 0.4f); // Rocky mountain color
    return config;
}

TerrainConfig LandscapeGenerator::create_desert_dunes() {
    TerrainConfig config;
    config.width = 80;
    config.depth = 80;
    config.scale = 4.0f;
    config.height_scale = 8.0f;
    config.noise_frequency = 0.03f;
    config.octaves = 3;
    config.persistence = 0.4f;
    config.base_color = glm::vec3(0.9f, 0.8f, 0.6f); // Desert sand color
    return config;
}

TerrainConfig LandscapeGenerator::create_battlefield() {
    TerrainConfig config;
    config.width = 32;
    config.depth = 32;
    config.scale = 2.0f;
    config.height_scale = 4.0f;
    config.noise_frequency = 0.1f;
    config.octaves = 3;
    config.persistence = 0.5f;
    config.base_color = glm::vec3(0.25f, 0.5f, 0.2f); // Darker battlefield green
    config.roughness = 0.9f;
    return config;
}

TerrainConfig LandscapeGenerator::create_random() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> size_dist(64, 128);
    std::uniform_real_distribution<> scale_dist(1.5f, 4.0f);
    std::uniform_real_distribution<> height_dist(5.0f, 20.0f);
    std::uniform_real_distribution<> freq_dist(0.02f, 0.1f);
    std::uniform_int_distribution<> octave_dist(2, 5);
    std::uniform_real_distribution<> color_dist(0.2f, 0.8f);
    
    TerrainConfig config;
    config.width = size_dist(gen);
    config.depth = size_dist(gen);
    config.scale = scale_dist(gen);
    config.height_scale = height_dist(gen);
    config.noise_frequency = freq_dist(gen);
    config.octaves = octave_dist(gen);
    config.base_color = glm::vec3(color_dist(gen), color_dist(gen), color_dist(gen));
    
    return config;
}

} // namespace CorePulse