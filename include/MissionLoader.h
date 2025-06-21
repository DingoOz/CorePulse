#pragma once

#include "GLTFLoader.h"
#include "MissionSystem.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <optional>

namespace CorePulse {

// Forward declarations
struct Mission;
struct MissionObjective;
struct MissionSpawnPoint;

// Mission data extension for glTF files
struct CP_Mission_Data {
    std::vector<MissionObjective> objectives;
    std::vector<MissionSpawnPoint> spawn_points;
    std::string mission_name;
    std::string mission_description;
    float time_limit = 0.0f;
    bool require_all_objectives = true;
    std::string success_message;
    std::string failure_message;
    
    // Lua scripts
    std::string lua_init_script;
    std::string lua_update_script;
    std::string lua_cleanup_script;
    
    // Mission metadata
    std::string author;
    std::string version;
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> custom_properties;
};

// Mission validation result
struct MissionValidationResult {
    bool is_valid = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    void add_error(const std::string& error) {
        is_valid = false;
        errors.push_back(error);
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

// Mission loader class - handles loading missions from glTF files
class MissionLoader {
public:
    MissionLoader();
    ~MissionLoader() = default;
    
    // Mission loading
    std::unique_ptr<Mission> load_mission(const std::string& filepath);
    bool load_mission_into(const std::string& filepath, Mission& mission);
    
    // Mission validation
    MissionValidationResult validate_mission_file(const std::string& filepath);
    MissionValidationResult validate_mission_data(const CP_Mission_Data& mission_data);
    
    // Utility functions
    bool is_mission_file(const std::string& filepath);
    std::vector<std::string> find_mission_files(const std::string& directory);
    
    // Error handling
    const std::string& get_last_error() const { return last_error_; }
    bool has_error() const { return !last_error_.empty(); }
    void clear_error() { last_error_.clear(); }
    
    // Mission metadata
    struct MissionMetadata {
        std::string name;
        std::string description;
        std::string author;
        std::string version;
        std::vector<std::string> tags;
        std::string filepath;
        size_t file_size;
        std::string last_modified;
    };
    
    std::optional<MissionMetadata> get_mission_metadata(const std::string& filepath);
    std::vector<MissionMetadata> scan_mission_directory(const std::string& directory);
    
private:
    std::string last_error_;
    
    // glTF loading and parsing
    bool load_gltf_file(const std::string& filepath, GLTF::Document& document);
    std::optional<CP_Mission_Data> extract_mission_data(const GLTF::Document& document);
    
    // CP_mission_data extension parsing
    bool parse_mission_extension(const nlohmann::json& extension_json, CP_Mission_Data& mission_data);
    bool parse_objectives(const nlohmann::json& objectives_json, std::vector<MissionObjective>& objectives);
    bool parse_spawn_points(const nlohmann::json& spawn_points_json, std::vector<MissionSpawnPoint>& spawn_points);
    
    // Individual objective parsing
    MissionObjective parse_objective(const nlohmann::json& obj_json);
    ObjectiveType parse_objective_type(const std::string& type_str);
    
    // Individual spawn point parsing
    MissionSpawnPoint parse_spawn_point(const nlohmann::json& spawn_json);
    
    // Data conversion
    std::unique_ptr<Mission> convert_to_mission(const CP_Mission_Data& mission_data, const std::string& filepath);
    
    // Node reference resolution
    std::vector<Entity> resolve_target_nodes(const std::vector<uint32_t>& node_indices, const GLTF::Document& document);
    glm::vec3 extract_node_position(uint32_t node_index, const GLTF::Document& document);
    glm::vec3 extract_node_rotation(uint32_t node_index, const GLTF::Document& document);
    
    // Validation helpers
    void validate_objectives(const std::vector<MissionObjective>& objectives, MissionValidationResult& result);
    void validate_spawn_points(const std::vector<MissionSpawnPoint>& spawn_points, MissionValidationResult& result);
    void validate_objective(const MissionObjective& objective, MissionValidationResult& result);
    void validate_spawn_point(const MissionSpawnPoint& spawn_point, MissionValidationResult& result);
    
    // Lua script validation
    bool validate_lua_script(const std::string& script, std::string& error_message);
    
    // File utilities
    bool file_exists(const std::string& filepath);
    size_t get_file_size(const std::string& filepath);
    std::string get_file_modification_time(const std::string& filepath);
    std::string extract_filename(const std::string& filepath);
    std::string extract_directory(const std::string& filepath);
    
    // Error handling
    void set_error(const std::string& error) { last_error_ = error; }
    void clear_error_internal() { last_error_.clear(); }
    
    // JSON utilities
    bool json_has_field(const nlohmann::json& json, const std::string& field);
    template<typename T>
    T json_get_or_default(const nlohmann::json& json, const std::string& field, const T& default_value);
    
    glm::vec3 json_to_vec3(const nlohmann::json& json, const glm::vec3& default_value = glm::vec3(0.0f));
    std::vector<uint32_t> json_to_uint32_vector(const nlohmann::json& json);
    std::vector<std::string> json_to_string_vector(const nlohmann::json& json);
    std::unordered_map<std::string, std::string> json_to_string_map(const nlohmann::json& json);
};

} // namespace CorePulse