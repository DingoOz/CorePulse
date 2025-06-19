#include "MechExtensions.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>

namespace CorePulse {

// Utility function implementations
std::string hardpoint_type_to_string(HardpointType type) {
    switch (type) {
        case HardpointType::ENERGY: return "ENERGY";
        case HardpointType::BALLISTIC: return "BALLISTIC";
        case HardpointType::MISSILE: return "MISSILE";
        case HardpointType::AMS: return "AMS";
        case HardpointType::EQUIPMENT: return "EQUIPMENT";
        default: return "UNKNOWN";
    }
}

HardpointType hardpoint_type_from_string(const std::string& str) {
    if (str == "ENERGY") return HardpointType::ENERGY;
    if (str == "BALLISTIC") return HardpointType::BALLISTIC;
    if (str == "MISSILE") return HardpointType::MISSILE;
    if (str == "AMS") return HardpointType::AMS;
    if (str == "EQUIPMENT") return HardpointType::EQUIPMENT;
    throw std::invalid_argument("Unknown hardpoint type: " + str);
}

std::string hardpoint_size_to_string(HardpointSize size) {
    switch (size) {
        case HardpointSize::SMALL: return "SMALL";
        case HardpointSize::MEDIUM: return "MEDIUM";
        case HardpointSize::LARGE: return "LARGE";
        default: return "UNKNOWN";
    }
}

HardpointSize hardpoint_size_from_string(const std::string& str) {
    if (str == "SMALL") return HardpointSize::SMALL;
    if (str == "MEDIUM") return HardpointSize::MEDIUM;
    if (str == "LARGE") return HardpointSize::LARGE;
    throw std::invalid_argument("Unknown hardpoint size: " + str);
}

std::string component_type_to_string(MechComponentType type) {
    switch (type) {
        case MechComponentType::HEAD: return "HEAD";
        case MechComponentType::CENTER_TORSO: return "CENTER_TORSO";
        case MechComponentType::LEFT_TORSO: return "LEFT_TORSO";
        case MechComponentType::RIGHT_TORSO: return "RIGHT_TORSO";
        case MechComponentType::LEFT_ARM: return "LEFT_ARM";
        case MechComponentType::RIGHT_ARM: return "RIGHT_ARM";
        case MechComponentType::LEFT_LEG: return "LEFT_LEG";
        case MechComponentType::RIGHT_LEG: return "RIGHT_LEG";
        case MechComponentType::ENGINE: return "ENGINE";
        case MechComponentType::GYRO: return "GYRO";
        case MechComponentType::COCKPIT: return "COCKPIT";
        case MechComponentType::LIFE_SUPPORT: return "LIFE_SUPPORT";
        case MechComponentType::SENSORS: return "SENSORS";
        case MechComponentType::JUMPJETS: return "JUMPJETS";
        case MechComponentType::HEAT_SINKS: return "HEAT_SINKS";
        default: return "UNKNOWN";
    }
}

MechComponentType component_type_from_string(const std::string& str) {
    if (str == "HEAD") return MechComponentType::HEAD;
    if (str == "CENTER_TORSO") return MechComponentType::CENTER_TORSO;
    if (str == "LEFT_TORSO") return MechComponentType::LEFT_TORSO;
    if (str == "RIGHT_TORSO") return MechComponentType::RIGHT_TORSO;
    if (str == "LEFT_ARM") return MechComponentType::LEFT_ARM;
    if (str == "RIGHT_ARM") return MechComponentType::RIGHT_ARM;
    if (str == "LEFT_LEG") return MechComponentType::LEFT_LEG;
    if (str == "RIGHT_LEG") return MechComponentType::RIGHT_LEG;
    if (str == "ENGINE") return MechComponentType::ENGINE;
    if (str == "GYRO") return MechComponentType::GYRO;
    if (str == "COCKPIT") return MechComponentType::COCKPIT;
    if (str == "LIFE_SUPPORT") return MechComponentType::LIFE_SUPPORT;
    if (str == "SENSORS") return MechComponentType::SENSORS;
    if (str == "JUMPJETS") return MechComponentType::JUMPJETS;
    if (str == "HEAT_SINKS") return MechComponentType::HEAT_SINKS;
    throw std::invalid_argument("Unknown component type: " + str);
}

// CP_Walker_Hardpoints implementation
CP_Walker_Hardpoints CP_Walker_Hardpoints::from_json(const nlohmann::json& json) {
    CP_Walker_Hardpoints result;
    
    if (!json.contains("hardpoints") || !json["hardpoints"].is_array()) {
        std::cerr << "Warning: CP_walker_hardpoints extension missing or invalid hardpoints array" << std::endl;
        return result;
    }
    
    for (const auto& hp_json : json["hardpoints"]) {
        Hardpoint hp;
        
        // Required fields
        if (hp_json.contains("id")) {
            hp.id = hp_json["id"];
        }
        if (hp_json.contains("name")) {
            hp.name = hp_json["name"];
        }
        if (hp_json.contains("type")) {
            hp.type = hardpoint_type_from_string(hp_json["type"]);
        }
        if (hp_json.contains("size")) {
            hp.size = hardpoint_size_from_string(hp_json["size"]);
        }
        
        // Position and orientation
        if (hp_json.contains("position") && hp_json["position"].is_array() && hp_json["position"].size() >= 3) {
            hp.position = glm::vec3(
                hp_json["position"][0],
                hp_json["position"][1], 
                hp_json["position"][2]
            );
        }
        
        if (hp_json.contains("orientation") && hp_json["orientation"].is_array() && hp_json["orientation"].size() >= 3) {
            hp.orientation = glm::vec3(
                hp_json["orientation"][0],
                hp_json["orientation"][1],
                hp_json["orientation"][2]
            );
        }
        
        // Constraints
        if (hp_json.contains("max_tonnage")) {
            hp.max_tonnage = hp_json["max_tonnage"];
        }
        if (hp_json.contains("critical_slots")) {
            hp.critical_slots = hp_json["critical_slots"];
        }
        
        // Optional attachment node
        if (hp_json.contains("attachment_node")) {
            hp.attachment_node = hp_json["attachment_node"];
        }
        
        result.hardpoints.push_back(hp);
        
        std::cout << "Loaded hardpoint: " << hp.name << " (" << hardpoint_type_to_string(hp.type) 
                  << ", " << hardpoint_size_to_string(hp.size) << ")" << std::endl;
    }
    
    return result;
}

nlohmann::json CP_Walker_Hardpoints::to_json() const {
    nlohmann::json result;
    result["hardpoints"] = nlohmann::json::array();
    
    for (const auto& hp : hardpoints) {
        nlohmann::json hp_json;
        hp_json["id"] = hp.id;
        hp_json["name"] = hp.name;
        hp_json["type"] = hardpoint_type_to_string(hp.type);
        hp_json["size"] = hardpoint_size_to_string(hp.size);
        hp_json["position"] = {hp.position.x, hp.position.y, hp.position.z};
        hp_json["orientation"] = {hp.orientation.x, hp.orientation.y, hp.orientation.z};
        hp_json["max_tonnage"] = hp.max_tonnage;
        hp_json["critical_slots"] = hp.critical_slots;
        
        if (!hp.attachment_node.empty()) {
            hp_json["attachment_node"] = hp.attachment_node;
        }
        
        result["hardpoints"].push_back(hp_json);
    }
    
    return result;
}

// CP_Damage_Zones implementation
CP_Damage_Zones CP_Damage_Zones::from_json(const nlohmann::json& json) {
    CP_Damage_Zones result;
    
    if (!json.contains("zones") || !json["zones"].is_array()) {
        std::cerr << "Warning: CP_damage_zones extension missing or invalid zones array" << std::endl;
        return result;
    }
    
    for (const auto& zone_json : json["zones"]) {
        DamageZone zone;
        
        // Basic info
        if (zone_json.contains("id")) {
            zone.id = zone_json["id"];
        }
        if (zone_json.contains("name")) {
            zone.name = zone_json["name"];
        }
        if (zone_json.contains("type")) {
            zone.type = component_type_from_string(zone_json["type"]);
        }
        
        // Health values
        if (zone_json.contains("max_armor")) {
            zone.max_armor = zone_json["max_armor"];
            zone.current_armor = zone.max_armor; // Start at full armor
        }
        if (zone_json.contains("max_internal")) {
            zone.max_internal = zone_json["max_internal"];
            zone.current_internal = zone.max_internal; // Start at full internal
        }
        
        // Bounds for hit detection
        if (zone_json.contains("bounds_min") && zone_json["bounds_min"].is_array() && zone_json["bounds_min"].size() >= 3) {
            zone.bounds_min = glm::vec3(
                zone_json["bounds_min"][0],
                zone_json["bounds_min"][1],
                zone_json["bounds_min"][2]
            );
        }
        
        if (zone_json.contains("bounds_max") && zone_json["bounds_max"].is_array() && zone_json["bounds_max"].size() >= 3) {
            zone.bounds_max = glm::vec3(
                zone_json["bounds_max"][0],
                zone_json["bounds_max"][1],
                zone_json["bounds_max"][2]
            );
        }
        
        // Critical slots
        if (zone_json.contains("total_slots")) {
            zone.total_slots = zone_json["total_slots"];
            zone.available_slots = zone.total_slots;
        }
        
        // Destruction effects
        if (zone_json.contains("destruction_effects") && zone_json["destruction_effects"].is_array()) {
            for (const auto& effect : zone_json["destruction_effects"]) {
                zone.destruction_effects.push_back(effect);
            }
        }
        
        result.zones.push_back(zone);
        
        std::cout << "Loaded damage zone: " << zone.name << " (" << component_type_to_string(zone.type) 
                  << ", " << zone.max_armor << " armor, " << zone.max_internal << " internal)" << std::endl;
    }
    
    return result;
}

nlohmann::json CP_Damage_Zones::to_json() const {
    nlohmann::json result;
    result["zones"] = nlohmann::json::array();
    
    for (const auto& zone : zones) {
        nlohmann::json zone_json;
        zone_json["id"] = zone.id;
        zone_json["name"] = zone.name;
        zone_json["type"] = component_type_to_string(zone.type);
        zone_json["max_armor"] = zone.max_armor;
        zone_json["max_internal"] = zone.max_internal;
        zone_json["bounds_min"] = {zone.bounds_min.x, zone.bounds_min.y, zone.bounds_min.z};
        zone_json["bounds_max"] = {zone.bounds_max.x, zone.bounds_max.y, zone.bounds_max.z};
        zone_json["total_slots"] = zone.total_slots;
        zone_json["destruction_effects"] = zone.destruction_effects;
        
        result["zones"].push_back(zone_json);
    }
    
    return result;
}

} // namespace CorePulse