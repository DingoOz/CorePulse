#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace CorePulse {

// Hardpoint types for different weapon categories
enum class HardpointType {
    ENERGY,      // Lasers, PPCs
    BALLISTIC,   // Autocannons, Machine guns
    MISSILE,     // LRMs, SRMs
    AMS,         // Anti-missile systems
    EQUIPMENT    // Special equipment
};

// Hardpoint size constraints
enum class HardpointSize {
    SMALL,
    MEDIUM,
    LARGE
};

// Individual hardpoint definition
struct Hardpoint {
    std::string id;
    std::string name;
    HardpointType type;
    HardpointSize size;
    
    // 3D position in model space
    glm::vec3 position;
    glm::vec3 orientation;  // Euler angles for weapon facing direction
    
    // Constraints
    float max_tonnage = 0.0f;     // Maximum weapon weight
    int critical_slots = 1;       // Number of critical slots required
    
    // State
    bool occupied = false;
    std::string mounted_weapon_id;
    
    // Visual mounting point (optional node name for attachment)
    std::string attachment_node;
};

// Component types for damage modeling
enum class MechComponentType {
    HEAD,
    CENTER_TORSO,
    LEFT_TORSO,
    RIGHT_TORSO,
    LEFT_ARM,
    RIGHT_ARM,
    LEFT_LEG,
    RIGHT_LEG,
    
    // Internal components
    ENGINE,
    GYRO,
    COCKPIT,
    LIFE_SUPPORT,
    SENSORS,
    JUMPJETS,
    HEAT_SINKS
};

// Damage zone definition
struct DamageZone {
    std::string id;
    std::string name;
    MechComponentType type;
    
    // Health and armor
    float max_armor = 0.0f;
    float current_armor = 0.0f;
    float max_internal = 0.0f;
    float current_internal = 0.0f;
    
    // Physical bounds in model space (for hit detection)
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
    
    // Critical slots and equipment
    int total_slots = 12;
    int available_slots = 12;
    std::vector<std::string> equipment_ids;
    
    // Destruction effects
    bool destroyed = false;
    std::vector<std::string> destruction_effects;  // Effects to trigger when destroyed
};

// Mech configuration data
struct MechConfiguration {
    std::string mech_id;
    std::string variant_name;
    
    // Basic stats
    float tonnage = 0.0f;
    float max_speed = 0.0f;
    float max_heat = 0.0f;
    
    // Hardpoints by location
    std::unordered_map<std::string, std::vector<Hardpoint>> hardpoints_by_location;
    
    // Damage zones
    std::vector<DamageZone> damage_zones;
    
    // Heat management
    int heat_sinks = 10;
    float heat_dissipation = 10.0f;
    
    // Movement
    int jump_jets = 0;
    float jump_range = 0.0f;
};

// GLTF Extension data structures
struct CP_Walker_Hardpoints {
    std::vector<Hardpoint> hardpoints;
    
    // Parse from GLTF extension JSON
    static CP_Walker_Hardpoints from_json(const nlohmann::json& json);
    nlohmann::json to_json() const;
};

struct CP_Damage_Zones {
    std::vector<DamageZone> zones;
    
    // Parse from GLTF extension JSON
    static CP_Damage_Zones from_json(const nlohmann::json& json);
    nlohmann::json to_json() const;
};

// Utility functions
std::string hardpoint_type_to_string(HardpointType type);
HardpointType hardpoint_type_from_string(const std::string& str);

std::string hardpoint_size_to_string(HardpointSize size);
HardpointSize hardpoint_size_from_string(const std::string& str);

std::string component_type_to_string(MechComponentType type);
MechComponentType component_type_from_string(const std::string& str);

} // namespace CorePulse