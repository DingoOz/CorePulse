#include "MissionLoader.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <regex>
#include <algorithm>
#include <unordered_set>

#ifdef COREPULSE_HAS_LUA
#include <lua.hpp>
#endif

namespace CorePulse {

MissionLoader::MissionLoader() {
    clear_error_internal();
}

std::unique_ptr<Mission> MissionLoader::load_mission(const std::string& filepath) {
    clear_error_internal();
    
    if (!file_exists(filepath)) {
        set_error("Mission file does not exist: " + filepath);
        return nullptr;
    }
    
    // Load glTF document
    GLTF::Document document;
    if (!load_gltf_file(filepath, document)) {
        return nullptr;
    }
    
    // Extract mission data from glTF extensions
    auto mission_data = extract_mission_data(document);
    if (!mission_data.has_value()) {
        set_error("Failed to extract mission data from glTF file");
        return nullptr;
    }
    
    // Validate mission data
    auto validation = validate_mission_data(mission_data.value());
    if (!validation.is_valid) {
        std::string error_msg = "Mission validation failed:";
        for (const auto& error : validation.errors) {
            error_msg += "\n  - " + error;
        }
        set_error(error_msg);
        return nullptr;
    }
    
    // Convert to Mission object
    return convert_to_mission(mission_data.value(), filepath);
}

bool MissionLoader::load_mission_into(const std::string& filepath, Mission& mission) {
    auto loaded_mission = load_mission(filepath);
    if (!loaded_mission) {
        return false;
    }
    
    mission = *loaded_mission;
    return true;
}

MissionValidationResult MissionLoader::validate_mission_file(const std::string& filepath) {
    MissionValidationResult result;
    
    if (!file_exists(filepath)) {
        result.add_error("Mission file does not exist: " + filepath);
        return result;
    }
    
    // Load and validate glTF structure
    GLTF::Document document;
    if (!load_gltf_file(filepath, document)) {
        result.add_error("Failed to load glTF file: " + last_error_);
        return result;
    }
    
    // Extract and validate mission data
    auto mission_data = extract_mission_data(document);
    if (!mission_data.has_value()) {
        result.add_error("No CP_mission_data extension found in glTF file");
        return result;
    }
    
    return validate_mission_data(mission_data.value());
}

MissionValidationResult MissionLoader::validate_mission_data(const CP_Mission_Data& mission_data) {
    MissionValidationResult result;
    result.is_valid = true;
    
    // Validate basic mission info
    if (mission_data.mission_name.empty()) {
        result.add_error("Mission name is required");
    }
    
    if (mission_data.mission_description.empty()) {
        result.add_warning("Mission description is empty");
    }
    
    // Validate objectives
    if (mission_data.objectives.empty()) {
        result.add_error("Mission must have at least one objective");
    } else {
        validate_objectives(mission_data.objectives, result);
    }
    
    // Validate spawn points
    if (mission_data.spawn_points.empty()) {
        result.add_warning("Mission has no spawn points");
    } else {
        validate_spawn_points(mission_data.spawn_points, result);
    }
    
    // Validate Lua scripts
    std::string lua_error;
    if (!mission_data.lua_init_script.empty() && !validate_lua_script(mission_data.lua_init_script, lua_error)) {
        result.add_error("Invalid Lua init script: " + lua_error);
    }
    
    if (!mission_data.lua_update_script.empty() && !validate_lua_script(mission_data.lua_update_script, lua_error)) {
        result.add_error("Invalid Lua update script: " + lua_error);
    }
    
    if (!mission_data.lua_cleanup_script.empty() && !validate_lua_script(mission_data.lua_cleanup_script, lua_error)) {
        result.add_error("Invalid Lua cleanup script: " + lua_error);
    }
    
    return result;
}

bool MissionLoader::is_mission_file(const std::string& filepath) {
    if (!file_exists(filepath)) {
        return false;
    }
    
    // Check file extension
    std::string extension = std::filesystem::path(filepath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension != ".gltf" && extension != ".glb") {
        return false;
    }
    
    // Quick check for CP_mission_data extension
    try {
        GLTF::Document document;
        if (!load_gltf_file(filepath, document)) {
            return false;
        }
        
        return document.extensions.contains("CP_mission_data");
    } catch (...) {
        return false;
    }
}

std::vector<std::string> MissionLoader::find_mission_files(const std::string& directory) {
    std::vector<std::string> mission_files;
    
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        return mission_files;
    }
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filepath = entry.path().string();
            if (is_mission_file(filepath)) {
                mission_files.push_back(filepath);
            }
        }
    }
    
    return mission_files;
}

std::optional<MissionLoader::MissionMetadata> MissionLoader::get_mission_metadata(const std::string& filepath) {
    if (!file_exists(filepath)) {
        return std::nullopt;
    }
    
    GLTF::Document document;
    if (!load_gltf_file(filepath, document)) {
        return std::nullopt;
    }
    
    auto mission_data = extract_mission_data(document);
    if (!mission_data.has_value()) {
        return std::nullopt;
    }
    
    MissionMetadata metadata;
    metadata.name = mission_data->mission_name;
    metadata.description = mission_data->mission_description;
    metadata.author = mission_data->author;
    metadata.version = mission_data->version;
    metadata.tags = mission_data->tags;
    metadata.filepath = filepath;
    metadata.file_size = get_file_size(filepath);
    metadata.last_modified = get_file_modification_time(filepath);
    
    return metadata;
}

std::vector<MissionLoader::MissionMetadata> MissionLoader::scan_mission_directory(const std::string& directory) {
    std::vector<MissionMetadata> missions;
    
    auto mission_files = find_mission_files(directory);
    for (const auto& filepath : mission_files) {
        auto metadata = get_mission_metadata(filepath);
        if (metadata.has_value()) {
            missions.push_back(metadata.value());
        }
    }
    
    return missions;
}

bool MissionLoader::load_gltf_file(const std::string& filepath, GLTF::Document& document) {
    try {
        GLTFLoader loader;
        
        std::string extension = std::filesystem::path(filepath).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        bool success = false;
        if (extension == ".gltf") {
            success = loader.load_gltf(filepath);
        } else if (extension == ".glb") {
            success = loader.load_glb(filepath);
        } else {
            set_error("Unsupported file format: " + extension);
            return false;
        }
        
        if (!success) {
            set_error("Failed to load glTF file: " + loader.get_error());
            return false;
        }
        
        document = loader.get_document();
        return true;
        
    } catch (const std::exception& e) {
        set_error("Exception loading glTF file: " + std::string(e.what()));
        return false;
    }
}

std::optional<CP_Mission_Data> MissionLoader::extract_mission_data(const GLTF::Document& document) {
    if (!document.extensions.contains("CP_mission_data")) {
        return std::nullopt;
    }
    
    CP_Mission_Data mission_data;
    
    try {
        const nlohmann::json& extension_json = document.extensions["CP_mission_data"];
        
        if (!parse_mission_extension(extension_json, mission_data)) {
            return std::nullopt;
        }
        
        return mission_data;
        
    } catch (const std::exception& e) {
        set_error("Exception parsing CP_mission_data extension: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool MissionLoader::parse_mission_extension(const nlohmann::json& extension_json, CP_Mission_Data& mission_data) {
    // Parse basic mission info
    mission_data.mission_name = json_get_or_default(extension_json, "name", std::string(""));
    mission_data.mission_description = json_get_or_default(extension_json, "description", std::string(""));
    mission_data.time_limit = json_get_or_default(extension_json, "time_limit", 0.0f);
    mission_data.require_all_objectives = json_get_or_default(extension_json, "require_all_objectives", true);
    mission_data.success_message = json_get_or_default(extension_json, "success_message", std::string("Mission Completed"));
    mission_data.failure_message = json_get_or_default(extension_json, "failure_message", std::string("Mission Failed"));
    
    // Parse Lua scripts
    mission_data.lua_init_script = json_get_or_default(extension_json, "lua_init_script", std::string(""));
    mission_data.lua_update_script = json_get_or_default(extension_json, "lua_update_script", std::string(""));
    mission_data.lua_cleanup_script = json_get_or_default(extension_json, "lua_cleanup_script", std::string(""));
    
    // Parse metadata
    mission_data.author = json_get_or_default(extension_json, "author", std::string(""));
    mission_data.version = json_get_or_default(extension_json, "version", std::string("1.0"));
    
    if (json_has_field(extension_json, "tags")) {
        mission_data.tags = json_to_string_vector(extension_json["tags"]);
    }
    
    if (json_has_field(extension_json, "custom_properties")) {
        mission_data.custom_properties = json_to_string_map(extension_json["custom_properties"]);
    }
    
    // Parse objectives
    if (json_has_field(extension_json, "objectives")) {
        if (!parse_objectives(extension_json["objectives"], mission_data.objectives)) {
            return false;
        }
    }
    
    // Parse spawn points
    if (json_has_field(extension_json, "spawn_points")) {
        if (!parse_spawn_points(extension_json["spawn_points"], mission_data.spawn_points)) {
            return false;
        }
    }
    
    return true;
}

bool MissionLoader::parse_objectives(const nlohmann::json& objectives_json, std::vector<MissionObjective>& objectives) {
    if (!objectives_json.is_array()) {
        set_error("Objectives must be an array");
        return false;
    }
    
    for (const auto& obj_json : objectives_json) {
        try {
            objectives.push_back(parse_objective(obj_json));
        } catch (const std::exception& e) {
            set_error("Error parsing objective: " + std::string(e.what()));
            return false;
        }
    }
    
    return true;
}

bool MissionLoader::parse_spawn_points(const nlohmann::json& spawn_points_json, std::vector<MissionSpawnPoint>& spawn_points) {
    if (!spawn_points_json.is_array()) {
        set_error("Spawn points must be an array");
        return false;
    }
    
    for (const auto& spawn_json : spawn_points_json) {
        try {
            spawn_points.push_back(parse_spawn_point(spawn_json));
        } catch (const std::exception& e) {
            set_error("Error parsing spawn point: " + std::string(e.what()));
            return false;
        }
    }
    
    return true;
}

MissionObjective MissionLoader::parse_objective(const nlohmann::json& obj_json) {
    MissionObjective objective;
    
    objective.id = json_get_or_default(obj_json, "id", std::string(""));
    objective.description = json_get_or_default(obj_json, "description", std::string(""));
    objective.is_optional = json_get_or_default(obj_json, "is_optional", false);
    objective.target_value = json_get_or_default(obj_json, "target_value", 0.0f);
    
    // Parse objective type
    std::string type_str = json_get_or_default(obj_json, "type", std::string("elimination"));
    objective.type = parse_objective_type(type_str);
    
    // Parse target position
    if (json_has_field(obj_json, "target_position")) {
        objective.target_position = json_to_vec3(obj_json["target_position"]);
    }
    
    // Parse Lua scripts
    objective.lua_script = json_get_or_default(obj_json, "lua_script", std::string(""));
    objective.on_start_script = json_get_or_default(obj_json, "on_start_script", std::string(""));
    objective.on_update_script = json_get_or_default(obj_json, "on_update_script", std::string(""));
    objective.on_complete_script = json_get_or_default(obj_json, "on_complete_script", std::string(""));
    objective.on_fail_script = json_get_or_default(obj_json, "on_fail_script", std::string(""));
    
    return objective;
}

ObjectiveType MissionLoader::parse_objective_type(const std::string& type_str) {
    if (type_str == "elimination") return ObjectiveType::Elimination;
    if (type_str == "escort") return ObjectiveType::Escort;
    if (type_str == "defend") return ObjectiveType::Defend;
    if (type_str == "navigate") return ObjectiveType::Navigate;
    if (type_str == "collect") return ObjectiveType::Collect;
    if (type_str == "timer") return ObjectiveType::Timer;
    if (type_str == "custom") return ObjectiveType::Custom;
    
    return ObjectiveType::Elimination; // Default
}

MissionSpawnPoint MissionLoader::parse_spawn_point(const nlohmann::json& spawn_json) {
    MissionSpawnPoint spawn_point;
    
    spawn_point.name = json_get_or_default(spawn_json, "name", std::string(""));
    spawn_point.entity_type = json_get_or_default(spawn_json, "entity_type", std::string(""));
    spawn_point.is_player_spawn = json_get_or_default(spawn_json, "is_player_spawn", false);
    
    // Parse position and rotation
    if (json_has_field(spawn_json, "position")) {
        spawn_point.position = json_to_vec3(spawn_json["position"]);
    }
    
    if (json_has_field(spawn_json, "rotation")) {
        spawn_point.rotation = json_to_vec3(spawn_json["rotation"]);
    }
    
    // Parse properties
    if (json_has_field(spawn_json, "properties")) {
        spawn_point.properties = json_to_string_map(spawn_json["properties"]);
    }
    
    return spawn_point;
}

std::unique_ptr<Mission> MissionLoader::convert_to_mission(const CP_Mission_Data& mission_data, const std::string& filepath) {
    auto mission = std::make_unique<Mission>();
    
    // Generate mission ID from filename
    mission->id = extract_filename(filepath);
    mission->name = mission_data.mission_name;
    mission->description = mission_data.mission_description;
    mission->filename = filepath;
    mission->time_limit = mission_data.time_limit;
    mission->require_all_objectives = mission_data.require_all_objectives;
    mission->success_message = mission_data.success_message;
    mission->failure_message = mission_data.failure_message;
    
    // Copy Lua scripts
    mission->lua_init_script = mission_data.lua_init_script;
    mission->lua_update_script = mission_data.lua_update_script;
    mission->lua_cleanup_script = mission_data.lua_cleanup_script;
    
    // Copy objectives and spawn points
    mission->objectives = mission_data.objectives;
    mission->spawn_points = mission_data.spawn_points;
    
    // Build required objectives list
    if (mission_data.require_all_objectives) {
        for (const auto& objective : mission_data.objectives) {
            if (!objective.is_optional) {
                mission->required_objective_ids.push_back(objective.id);
            }
        }
    }
    
    return mission;
}

void MissionLoader::validate_objectives(const std::vector<MissionObjective>& objectives, MissionValidationResult& result) {
    std::unordered_set<std::string> objective_ids;
    
    for (const auto& objective : objectives) {
        // Check for duplicate IDs
        if (objective_ids.find(objective.id) != objective_ids.end()) {
            result.add_error("Duplicate objective ID: " + objective.id);
        }
        objective_ids.insert(objective.id);
        
        validate_objective(objective, result);
    }
}

void MissionLoader::validate_spawn_points(const std::vector<MissionSpawnPoint>& spawn_points, MissionValidationResult& result) {
    std::unordered_set<std::string> spawn_names;
    bool has_player_spawn = false;
    
    for (const auto& spawn_point : spawn_points) {
        // Check for duplicate names
        if (!spawn_point.name.empty()) {
            if (spawn_names.find(spawn_point.name) != spawn_names.end()) {
                result.add_error("Duplicate spawn point name: " + spawn_point.name);
            }
            spawn_names.insert(spawn_point.name);
        }
        
        if (spawn_point.is_player_spawn) {
            if (has_player_spawn) {
                result.add_warning("Multiple player spawn points found");
            }
            has_player_spawn = true;
        }
        
        validate_spawn_point(spawn_point, result);
    }
    
    if (!has_player_spawn) {
        result.add_warning("No player spawn point found");
    }
}

void MissionLoader::validate_objective(const MissionObjective& objective, MissionValidationResult& result) {
    if (objective.id.empty()) {
        result.add_error("Objective ID cannot be empty");
    }
    
    if (objective.description.empty()) {
        result.add_warning("Objective description is empty: " + objective.id);
    }
    
    // Validate Lua scripts
    std::string lua_error;
    if (!objective.lua_script.empty() && !validate_lua_script(objective.lua_script, lua_error)) {
        result.add_error("Invalid Lua script in objective " + objective.id + ": " + lua_error);
    }
    
    if (!objective.on_start_script.empty() && !validate_lua_script(objective.on_start_script, lua_error)) {
        result.add_error("Invalid on_start script in objective " + objective.id + ": " + lua_error);
    }
    
    if (!objective.on_update_script.empty() && !validate_lua_script(objective.on_update_script, lua_error)) {
        result.add_error("Invalid on_update script in objective " + objective.id + ": " + lua_error);
    }
    
    if (!objective.on_complete_script.empty() && !validate_lua_script(objective.on_complete_script, lua_error)) {
        result.add_error("Invalid on_complete script in objective " + objective.id + ": " + lua_error);
    }
    
    if (!objective.on_fail_script.empty() && !validate_lua_script(objective.on_fail_script, lua_error)) {
        result.add_error("Invalid on_fail script in objective " + objective.id + ": " + lua_error);
    }
}

void MissionLoader::validate_spawn_point(const MissionSpawnPoint& spawn_point, MissionValidationResult& result) {
    if (spawn_point.name.empty()) {
        result.add_warning("Spawn point has no name");
    }
    
    if (spawn_point.entity_type.empty()) {
        result.add_warning("Spawn point has no entity type: " + spawn_point.name);
    }
}

bool MissionLoader::validate_lua_script(const std::string& script, std::string& error_message) {
    if (script.empty()) {
        return true; // Empty scripts are valid
    }
    
#ifdef COREPULSE_HAS_LUA
    // Basic Lua syntax validation
    lua_State* L = luaL_newstate();
    if (!L) {
        error_message = "Failed to create Lua state";
        return false;
    }
    
    int result = luaL_loadstring(L, script.c_str());
    if (result != LUA_OK) {
        error_message = lua_tostring(L, -1);
        lua_close(L);
        return false;
    }
    
    lua_close(L);
    return true;
#else
    // Without Lua, we can't validate but scripts will be ignored anyway
    error_message = "Lua not available - script will be ignored";
    return true; // Don't fail validation just because Lua is missing
#endif
}

bool MissionLoader::file_exists(const std::string& filepath) {
    return std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath);
}

size_t MissionLoader::get_file_size(const std::string& filepath) {
    try {
        return std::filesystem::file_size(filepath);
    } catch (...) {
        return 0;
    }
}

std::string MissionLoader::get_file_modification_time(const std::string& filepath) {
    try {
        auto ftime = std::filesystem::last_write_time(filepath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        return std::asctime(std::localtime(&cftime));
    } catch (...) {
        return "Unknown";
    }
}

std::string MissionLoader::extract_filename(const std::string& filepath) {
    return std::filesystem::path(filepath).stem().string();
}

std::string MissionLoader::extract_directory(const std::string& filepath) {
    return std::filesystem::path(filepath).parent_path().string();
}

bool MissionLoader::json_has_field(const nlohmann::json& json, const std::string& field) {
    return json.is_object() && json.contains(field);
}

template<typename T>
T MissionLoader::json_get_or_default(const nlohmann::json& json, const std::string& field, const T& default_value) {
    if (json_has_field(json, field)) {
        try {
            return json[field].get<T>();
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

glm::vec3 MissionLoader::json_to_vec3(const nlohmann::json& json, const glm::vec3& default_value) {
    if (!json.is_array() || json.size() < 3) {
        return default_value;
    }
    
    try {
        return glm::vec3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>());
    } catch (...) {
        return default_value;
    }
}

std::vector<uint32_t> MissionLoader::json_to_uint32_vector(const nlohmann::json& json) {
    std::vector<uint32_t> result;
    
    if (!json.is_array()) {
        return result;
    }
    
    for (const auto& item : json) {
        try {
            result.push_back(item.get<uint32_t>());
        } catch (...) {
            // Skip invalid items
        }
    }
    
    return result;
}

std::vector<std::string> MissionLoader::json_to_string_vector(const nlohmann::json& json) {
    std::vector<std::string> result;
    
    if (!json.is_array()) {
        return result;
    }
    
    for (const auto& item : json) {
        try {
            result.push_back(item.get<std::string>());
        } catch (...) {
            // Skip invalid items
        }
    }
    
    return result;
}

std::unordered_map<std::string, std::string> MissionLoader::json_to_string_map(const nlohmann::json& json) {
    std::unordered_map<std::string, std::string> result;
    
    if (!json.is_object()) {
        return result;
    }
    
    for (auto it = json.begin(); it != json.end(); ++it) {
        try {
            result[it.key()] = it.value().get<std::string>();
        } catch (...) {
            // Skip invalid items
        }
    }
    
    return result;
}

} // namespace CorePulse