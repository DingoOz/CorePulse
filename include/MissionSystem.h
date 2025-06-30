#pragma once

#include "System.h"
#include "Components.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#ifdef COREPULSE_HAS_LUA
extern "C" {
#include <lua.h>
}
#endif

namespace CorePulse {

// Forward declarations
class World;
class MissionLoader;

// Mission objective types
enum class ObjectiveType {
    Elimination,    // Destroy specific targets
    Escort,         // Protect and guide allies
    Defend,         // Protect a location or object
    Navigate,       // Reach a specific location
    Collect,        // Gather items or data
    Timer,          // Survive for a duration
    Custom          // Lua-scripted objective
};

// Mission objective status
enum class ObjectiveStatus {
    Pending,        // Not yet started
    Active,         // Currently in progress
    Completed,      // Successfully completed
    Failed,         // Failed to complete
    Paused          // Temporarily paused
};

// Mission objective definition
struct MissionObjective {
    std::string id;
    ObjectiveType type;
    ObjectiveStatus status = ObjectiveStatus::Pending;
    std::string description;
    std::vector<Entity> target_entities;    // For elimination/escort objectives
    glm::vec3 target_position{0.0f};       // For navigation objectives
    float target_value = 0.0f;             // For timer/collection objectives  
    float current_value = 0.0f;            // Current progress
    bool is_optional = false;              // Can be skipped
    std::string lua_script;                // Custom Lua logic
    
    // Lua callback functions
    std::string on_start_script;
    std::string on_update_script;
    std::string on_complete_script;
    std::string on_fail_script;
};

// Mission spawn point
struct MissionSpawnPoint {
    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    std::string entity_type;               // Type of entity to spawn
    bool is_player_spawn = false;
    std::unordered_map<std::string, std::string> properties;
};

// Mission state
enum class MissionState {
    NotLoaded,      // Mission not loaded
    Loading,        // Currently loading
    Ready,          // Loaded and ready to start
    Active,         // Mission in progress
    Completed,      // Mission completed successfully
    Failed,         // Mission failed
    Paused          // Mission paused
};

// Mission data structure
struct Mission {
    std::string id;
    std::string name;
    std::string description;
    std::string filename;                  // glTF file path
    MissionState state = MissionState::NotLoaded;
    
    std::vector<MissionObjective> objectives;
    std::vector<MissionSpawnPoint> spawn_points;
    
    // Mission parameters
    float time_limit = 0.0f;               // 0 = no time limit
    float elapsed_time = 0.0f;
    bool allow_save = true;
    std::string success_message;
    std::string failure_message;
    
    // Lua scripting
    std::string lua_init_script;           // Called when mission starts
    std::string lua_update_script;         // Called every frame
    std::string lua_cleanup_script;        // Called when mission ends
    
    // Mission completion criteria
    bool require_all_objectives = true;    // If false, any objective completion wins
    std::vector<std::string> required_objective_ids;
};


// Mission system - manages mission loading, state, and objectives
class MissionSystem : public System {
public:
    MissionSystem();
    ~MissionSystem() override;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    void set_mission_loader(std::shared_ptr<MissionLoader> loader) { mission_loader_ = loader; }
    
    // Mission management
    bool load_mission(const std::string& mission_file);
    bool start_mission();
    bool pause_mission();
    bool resume_mission();
    void abort_mission();
    void complete_mission();
    void fail_mission(const std::string& reason = "");
    
    // Mission state
    const Mission* get_current_mission() const { return current_mission_.get(); }
    MissionState get_mission_state() const;
    bool is_mission_active() const;
    float get_mission_progress() const;        // 0.0 to 1.0
    
    // Objective management
    bool start_objective(const std::string& objective_id);
    bool complete_objective(const std::string& objective_id);
    bool fail_objective(const std::string& objective_id);
    const MissionObjective* get_objective(const std::string& objective_id) const;
    std::vector<const MissionObjective*> get_active_objectives() const;
    
    // Entity management
    Entity spawn_entity_at_point(const std::string& spawn_point_name);
    void register_mission_entity(Entity entity, const std::string& role);
    void unregister_mission_entity(Entity entity);
    bool is_mission_entity(Entity entity) const;
    
    // Lua scripting interface
    bool execute_lua_script(const std::string& script);
    void register_lua_functions();
#ifdef COREPULSE_HAS_LUA
    lua_State* get_lua_state() { return lua_state_; }
#else
    void* get_lua_state() { return nullptr; }
#endif
    
    // Mission events
    using MissionEventCallback = std::function<void(const std::string&)>;
    void set_on_mission_start(MissionEventCallback callback) { on_mission_start_ = callback; }
    void set_on_mission_complete(MissionEventCallback callback) { on_mission_complete_ = callback; }
    void set_on_mission_fail(MissionEventCallback callback) { on_mission_fail_ = callback; }
    void set_on_objective_complete(MissionEventCallback callback) { on_objective_complete_ = callback; }
    
private:
    World* world_ = nullptr;
    std::shared_ptr<MissionLoader> mission_loader_;
    std::unique_ptr<Mission> current_mission_;
    
    // Lua scripting
#ifdef COREPULSE_HAS_LUA
    lua_State* lua_state_ = nullptr;
#else
    void* lua_state_ = nullptr; // Placeholder when Lua is not available
#endif
    
    // Mission event callbacks
    MissionEventCallback on_mission_start_;
    MissionEventCallback on_mission_complete_;
    MissionEventCallback on_mission_fail_;
    MissionEventCallback on_objective_complete_;
    
    // Internal methods
    void update_objectives(float delta_time);
    void update_lua_scripts(float delta_time);
    void check_mission_completion();
    void check_mission_failure();
    void process_objective(MissionObjective& objective, float delta_time);
    
    // Objective processors
    void process_elimination_objective(MissionObjective& objective);
    void process_escort_objective(MissionObjective& objective);
    void process_defend_objective(MissionObjective& objective);
    void process_navigate_objective(MissionObjective& objective);
    void process_collect_objective(MissionObjective& objective);
    void process_timer_objective(MissionObjective& objective, float delta_time);
    void process_custom_objective(MissionObjective& objective, float delta_time);
    
    // Lua script execution
    bool execute_objective_script(const MissionObjective& objective, const std::string& script);
    void handle_lua_error(const std::string& context);
    
    // Utility methods
    void spawn_mission_entities();
    void cleanup_mission_entities();
    float calculate_distance_to_target(Entity entity, const glm::vec3& target) const;
    bool are_all_targets_destroyed(const std::vector<Entity>& targets) const;
    bool is_entity_alive(Entity entity) const;
};

} // namespace CorePulse