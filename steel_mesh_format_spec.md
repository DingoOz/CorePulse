# CorePulse glTF Integration Specification

## Overview

CorePulse utilises the industry-standard glTF 2.0 (Graphics Language Transmission Format) for 3D asset storage and loading. This specification details how CorePulse extends glTF with custom extensions and describes the integration with the engine's rendering pipeline.

## Why glTF 2.0?

### Advantages
- **Industry Standard:** Widely supported across 3D tools and engines
- **Modern PBR:** Native support for physically-based rendering materials
- **Extensible:** Official extension mechanism for custom data
- **Efficient:** Binary .glb format for optimised loading
- **Animation Ready:** Built-in support for skeletal and morph target animations
- **Validation:** Official validator tools ensure file integrity

### Performance Considerations
- **Binary Format:** .glb files provide optimal loading performance
- **GPU-Ready:** Vertex data can be directly uploaded to GPU buffers
- **Sparse Accessors:** Efficient storage for animation data
- **Texture Reuse:** Single textures can be shared across multiple materials

## CorePulse glTF Extensions

### CP_walker_hardpoints
Defines weapon hardpoint locations and constraints for combat walkers.

```json
{
  "extensions": {
    "CP_walker_hardpoints": {
      "hardpoints": [
        {
          "name": "right_arm_energy",
          "type": "energy",
          "node": 12,
          "position": [0.5, 2.1, 0.2],
          "orientation": [0.0, 0.0, 0.0, 1.0],
          "max_tonnage": 5.0,
          "critical_slots": 2,
          "compatible_weapons": ["laser", "ppc"]
        }
      ]
    }
  }
}
```

### CP_damage_zones
Defines damage zones for component-based damage modelling.

```json
{
  "extensions": {
    "CP_damage_zones": {
      "zones": [
        {
          "name": "torso_centre",
          "nodes": [8, 9, 10],
          "max_armour": 80,
          "max_structure": 40,
          "critical_components": ["engine", "gyro"]
        }
      ]
    }
  }
}
```

### CP_mission_data
Stores mission-specific information within scene files.

```json
{
  "extensions": {
    "CP_mission_data": {
      "objectives": [
        {
          "id": "destroy_target",
          "type": "elimination",
          "target_nodes": [15, 16, 17],
          "description": "Destroy enemy supply depot"
        }
      ],
      "spawn_points": [
        {
          "name": "player_start",
          "position": [100.0, 0.0, 50.0],
          "rotation": [0.0, 45.0, 0.0]
        }
      ]
    }
  }
}
```

### CP_physics_properties
Defines physics properties for rigid body simulation.

```json
{
  "extensions": {
    "CP_physics_properties": {
      "mass": 75000.0,
      "centre_of_mass": [0.0, 1.5, 0.0],
      "collision_shapes": [
        {
          "type": "capsule",
          "radius": 1.2,
          "height": 4.0,
          "offset": [0.0, 2.0, 0.0]
        }
      ]
    }
  }
}
```

## glTF Loading Implementation

### C++20 glTF Loader
```cpp
#include <nlohmann/json.hpp>
#include <filesystem>
#include <memory>
#include <span>

class CorePulseGLTFLoader {
public:
    struct LoadedMesh {
        std::string name;
        std::vector<uint8_t> vertex_data;
        std::vector<uint32_t> indices;
        uint32_t material_index;
        uint32_t vertex_count;
        uint32_t vertex_stride;
        VertexAttributes attributes;
    };
    
    struct LoadedMaterial {
        std::string name;
        std::array<float, 4> base_color_factor;
        float metallic_factor;
        float roughness_factor;
        float normal_scale;
        std::array<float, 3> emissive_factor;
        bool double_sided;
        
        // Texture indices
        int32_t base_color_texture = -1;
        int32_t metallic_roughness_texture = -1;
        int32_t normal_texture = -1;
        int32_t emissive_texture = -1;
        int32_t occlusion_texture = -1;
    };
    
    struct LoadedTexture {
        std::string name;
        std::vector<uint8_t> data;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        TextureFormat format;
    };
    
    struct LoadedNode {
        std::string name;
        std::array<float, 16> transform;
        std::vector<uint32_t> children;
        int32_t mesh_index = -1;
        
        // CorePulse extensions
        std::optional<WalkerHardpoints> hardpoints;
        std::optional<DamageZones> damage_zones;
        std::optional<PhysicsProperties> physics;
    };
    
    // Load glTF/glb file
    bool load_gltf(const std::filesystem::path& filename);
    
    // Access loaded data
    std::span<const LoadedMesh> get_meshes() const { return meshes_; }
    std::span<const LoadedMaterial> get_materials() const { return materials_; }
    std::span<const LoadedTexture> get_textures() const { return textures_; }
    std::span<const LoadedNode> get_nodes() const { return nodes_; }
    
private:
    nlohmann::json gltf_doc_;
    std::vector<uint8_t> binary_data_;
    
    std::vector<LoadedMesh> meshes_;
    std::vector<LoadedMaterial> materials_;
    std::vector<LoadedTexture> textures_;
    std::vector<LoadedNode> nodes_;
    
    bool parse_gltf_document();
    bool load_meshes();
    bool load_materials();
    bool load_textures();
    bool load_nodes();
    bool process_extensions();
    
    std::vector<uint8_t> decode_accessor(uint32_t accessor_index);
};
```

### Vertex Attribute Handling
```cpp
enum class VertexAttributeType {
    Position,
    Normal,
    Tangent,
    TexCoord0,
    TexCoord1,
    Color0,
    Joints0,
    Weights0
};

struct VertexAttributes {
    std::unordered_map<VertexAttributeType, uint32_t> offsets;
    uint32_t stride;
    
    bool has_attribute(VertexAttributeType type) const {
        return offsets.contains(type);
    }
    
    uint32_t get_offset(VertexAttributeType type) const {
        auto it = offsets.find(type);
        return (it != offsets.end()) ? it->second : 0;
    }
};
```

## Asset Pipeline Integration

### Blender Export Workflow
1. **Model Creation:** Standard Blender modelling workflow
2. **Material Setup:** Use Principled BSDF for PBR materials
3. **Hardpoint Definition:** Custom Blender addon to mark hardpoint locations
4. **Animation:** Standard armature-based animation
5. **Export:** Use official glTF exporter with CorePulse extensions

### Blender Addon: CorePulse Tools
```python
import bpy
from bpy.props import StringProperty, FloatProperty, EnumProperty

class CorePulseHardpoint(bpy.types.PropertyGroup):
    hardpoint_type: EnumProperty(
        name="Type",
        items=[
            ('energy', "Energy", "Energy weapon hardpoint"),
            ('ballistic', "Ballistic", "Ballistic weapon hardpoint"),
            ('missile', "Missile", "Missile weapon hardpoint")
        ]
    )
    max_tonnage: FloatProperty(name="Max Tonnage", default=5.0)
    critical_slots: bpy.props.IntProperty(name="Critical Slots", default=1)

class CorePulseDamageZone(bpy.types.PropertyGroup):
    zone_name: StringProperty(name="Zone Name")
    max_armour: FloatProperty(name="Max Armour", default=50.0)
    max_structure: FloatProperty(name="Max Structure", default=25.0)

# Export function that adds CorePulse extensions to glTF
def export_corepulse_gltf(context, filepath):
    # Standard glTF export with custom extensions
    bpy.ops.export_scene.gltf(
        filepath=filepath,
        export_format='GLB',
        export_animations=True,
        export_materials='EXPORT',
        export_extras=True  # Include custom properties
    )
```

### Content Validation Pipeline
```cpp
class GLTFValidator {
public:
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    ValidationResult validate_walker_model(const std::filesystem::path& gltf_path);
    ValidationResult validate_mission_scene(const std::filesystem::path& gltf_path);
    ValidationResult validate_weapon_model(const std::filesystem::path& gltf_path);
    
private:
    bool check_required_extensions(const nlohmann::json& gltf);
    bool validate_hardpoints(const nlohmann::json& hardpoints);
    bool validate_damage_zones(const nlohmann::json& zones);
    bool check_animation_compatibility(const nlohmann::json& animations);
};
```

## Runtime Integration

### Scene Graph Integration
```cpp
class CorePulseScene {
public:
    bool load_from_gltf(const std::filesystem::path& gltf_path);
    
    // Scene traversal
    void update_animations(float delta_time);
    void update_transforms();
    void cull_frustum(const Frustum& frustum);
    
    // Walker-specific functionality
    std::vector<HardpointInfo> get_hardpoints(uint32_t node_id) const;
    DamageZoneInfo get_damage_zone(uint32_t node_id) const;
    
private:
    struct SceneNode {
        std::string name;
        Matrix4 local_transform;
        Matrix4 world_transform;
        std::vector<uint32_t> children;
        uint32_t parent = INVALID_INDEX;
        
        // Rendering data
        int32_t mesh_index = -1;
        int32_t material_index = -1;
        
        // CorePulse extensions
        std::optional<WalkerHardpoints> hardpoints;
        std::optional<DamageZones> damage_zones;
        std::optional<PhysicsProperties> physics;
    };
    
    std::vector<SceneNode> nodes_;
    std::vector<GLTFMesh> meshes_;
    std::vector<GLTFMaterial> materials_;
    CorePulseGLTFLoader loader_;
};
```

### Material System Integration
```cpp
class CorePulseMaterial {
public:
    void load_from_gltf_material(const LoadedMaterial& gltf_material);
    
    // PBR parameters
    void set_base_color(const Vector4& color) { base_color_ = color; }
    void set_metallic(float metallic) { metallic_ = metallic; }
    void set_roughness(float roughness) { roughness_ = roughness; }
    
    // Texture binding
    void bind_textures(const TextureManager& tex_manager) const;
    
    // Shader uniform setup
    void upload_uniforms(ShaderProgram& shader) const;
    
private:
    Vector4 base_color_{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_{0.0f};
    float roughness_{1.0f};
    float normal_scale_{1.0f};
    Vector3 emissive_{0.0f, 0.0f, 0.0f};
    
    TextureHandle base_color_texture_;
    TextureHandle metallic_roughness_texture_;
    TextureHandle normal_texture_;
    TextureHandle emissive_texture_;
    TextureHandle occlusion_texture_;
};
```

## Performance Optimisations

### Loading Optimisations
- **Memory Mapping:** Use memory-mapped files for large .glb files
- **Lazy Loading:** Load textures on-demand during rendering
- **Compression:** Use KTX2/Basis Universal for texture compression
- **Level of Detail:** Store multiple LOD meshes in single glTF file

### Runtime Optimisations
- **Instancing:** Use glTF extensions for instanced rendering
- **Frustum Culling:** Pre-calculate bounding volumes from glTF data
- **Animation Sampling:** Cache animation samples for smooth playback
- **Material Batching:** Group draws by material to minimise state changes

## File Organisation

### Asset Directory Structure
```
assets/
├── models/
│   ├── walkers/
│   │   ├── light/
│   │   │   ├── scout.glb
│   │   │   └── ranger.glb
│   │   ├── medium/
│   │   │   ├── centurion.glb
│   │   │   └── hunchback.glb
│   │   └── heavy/
│   │       ├── rifleman.glb
│   │       └── warhammer.glb
│   ├── weapons/
│   │   ├── energy/
│   │   ├── ballistic/
│   │   └── missile/
│   └── environment/
│       ├── buildings/
│       ├── terrain/
│       └── props/
├── missions/
│   ├── mission_01.glb
│   ├── mission_02.glb
│   └── ...
└── textures/
    ├── walkers/
    ├── weapons/
    └── environment/
```

### Naming Conventions
- **Walkers:** `[class]_[variant].glb` (e.g., `scout_mk2.glb`)
- **Weapons:** `[type]_[name].glb` (e.g., `energy_medium_laser.glb`)
- **Missions:** `mission_[number].glb` (e.g., `mission_01.glb`)
- **Environments:** `[location]_[variant].glb` (e.g., `desert_outpost.glb`)

## Validation and Testing

### Automated Testing
```cpp
// Unit tests for glTF loading
TEST(GLTFLoader, LoadValidWalkerModel) {
    CorePulseGLTFLoader loader;
    ASSERT_TRUE(loader.load_gltf("test_assets/walker_basic.glb"));
    
    auto meshes = loader.get_meshes();
    EXPECT_GT(meshes.size(), 0);
    
    auto materials = loader.get_materials();
    EXPECT_GT(materials.size(), 0);
}

TEST(GLTFLoader, ValidateHardpoints) {
    CorePulseGLTFLoader loader;
    ASSERT_TRUE(loader.load_gltf("test_assets/walker_with_hardpoints.glb"));
    
    auto nodes = loader.get_nodes();
    bool found_hardpoints = false;
    
    for (const auto& node : nodes) {
        if (node.hardpoints.has_value()) {
            found_hardpoints = true;
            EXPECT_GT(node.hardpoints->hardpoints.size(), 0);
        }
    }
    
    EXPECT_TRUE(found_hardpoints);
}
```

### Content Validation Tools
```bash
# Validate all walker models
corepulse_validate --type walker assets/models/walkers/*.glb

# Check mission scenes
corepulse_validate --type mission assets/missions/*.glb

# Batch validation with reporting
corepulse_validate --recursive assets/ --report validation_report.html
```

## Migration and Compatibility

### glTF Version Support
- **Primary:** glTF 2.0 with binary .glb format
- **Fallback:** glTF 2.0 with separate .gltf + .bin files
- **Extensions:** KHR_materials_pbrSpecularGlossiness (legacy support)

### Backward Compatibility
- Support for models without CorePulse extensions (default behaviour)
- Graceful degradation when extensions are missing
- Version detection and appropriate loading paths

This specification provides a comprehensive framework for integrating glTF 2.0 into CorePulse whilst maintaining the flexibility to extend the format for game-specific requirements. The use of standard glTF ensures compatibility with industry tools whilst custom extensions provide the specialised functionality needed for mech combat simulation.