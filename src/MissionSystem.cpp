#include "MissionSystem.h"
#include "MissionLoader.h"
#include "World.h"
#include <iostream>
#include <algorithm>

#ifdef COREPULSE_HAS_LUA
#include <lua.hpp>
#define LUA_AVAILABLE true
#else
#define LUA_AVAILABLE false
#endif

namespace CorePulse {

MissionSystem::MissionSystem() {
    // Initialize Lua state if available
#ifdef COREPULSE_HAS_LUA
    lua_state_ = luaL_newstate();
    if (lua_state_) {
        luaL_openlibs(lua_state_);
        register_lua_functions();
    }
#else
    lua_state_ = nullptr;
    std::cout << "MissionSystem: Lua not available, scripting disabled" << std::endl;
#endif
}

MissionSystem::~MissionSystem() {
#ifdef COREPULSE_HAS_LUA
    if (lua_state_) {
        lua_close(lua_state_);
        lua_state_ = nullptr;
    }
#endif
}

void MissionSystem::init() {
#ifdef COREPULSE_HAS_LUA
    if (!lua_state_) {
        std::cerr << "MissionSystem: Failed to initialize Lua state" << std::endl;
        return;
    }
#else
    std::cout << "MissionSystem: Initialized without Lua support" << std::endl;
#endif
    
    std::cout << "MissionSystem initialized" << std::endl;
}

void MissionSystem::update(float delta_time) {
    if (!current_mission_ || current_mission_->state != MissionState::Active) {
        return;
    }
    
    // Update mission timer
    current_mission_->elapsed_time += delta_time;
    
    // Check time limit
    if (current_mission_->time_limit > 0.0f && 
        current_mission_->elapsed_time >= current_mission_->time_limit) {
        fail_mission("Time limit exceeded");
        return;
    }
    
    // Update objectives
    update_objectives(delta_time);
    
    // Execute Lua update script
    update_lua_scripts(delta_time);
    
    // Check mission completion/failure conditions
    check_mission_completion();
    check_mission_failure();
}

void MissionSystem::shutdown() {
    if (current_mission_) {
        abort_mission();
    }
    
#ifdef COREPULSE_HAS_LUA
    if (lua_state_) {
        lua_close(lua_state_);
        lua_state_ = nullptr;
    }
#endif
    
    std::cout << "MissionSystem shutdown" << std::endl;
}

bool MissionSystem::load_mission(const std::string& mission_file) {
    if (!mission_loader_) {
        std::cerr << "MissionSystem: No mission loader set" << std::endl;
        return false;
    }
    
    // Clean up current mission
    if (current_mission_) {
        abort_mission();
    }
    
    // Load new mission
    current_mission_ = mission_loader_->load_mission(mission_file);
    if (!current_mission_) {
        std::cerr << "MissionSystem: Failed to load mission: " << mission_loader_->get_last_error() << std::endl;
        return false;
    }
    
    current_mission_->state = MissionState::Ready;
    std::cout << "Mission loaded: " << current_mission_->name << std::endl;
    
    return true;
}

bool MissionSystem::start_mission() {
    if (!current_mission_ || current_mission_->state != MissionState::Ready) {
        std::cerr << "MissionSystem: No mission ready to start" << std::endl;
        return false;
    }
    
    current_mission_->state = MissionState::Active;
    current_mission_->elapsed_time = 0.0f;
    
    // Reset all objectives to pending
    for (auto& objective : current_mission_->objectives) {
        objective.status = ObjectiveStatus::Pending;
        objective.current_value = 0.0f;
    }
    
    // Spawn mission entities
    spawn_mission_entities();
    
    // Execute Lua init script
    if (!current_mission_->lua_init_script.empty()) {
        execute_lua_script(current_mission_->lua_init_script);
    }
    
    // Start first objectives
    for (auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Pending) {
            start_objective(objective.id);
            break; // Start only first objective initially
        }
    }
    
    // Trigger mission start callback
    if (on_mission_start_) {
        on_mission_start_(current_mission_->id);
    }
    
    std::cout << "Mission started: " << current_mission_->name << std::endl;
    return true;
}

bool MissionSystem::pause_mission() {
    if (!current_mission_ || current_mission_->state != MissionState::Active) {
        return false;
    }
    
    current_mission_->state = MissionState::Paused;
    
    // Pause all active objectives
    for (auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Active) {
            objective.status = ObjectiveStatus::Paused;
        }
    }
    
    std::cout << "Mission paused: " << current_mission_->name << std::endl;
    return true;
}

bool MissionSystem::resume_mission() {
    if (!current_mission_ || current_mission_->state != MissionState::Paused) {
        return false;
    }
    
    current_mission_->state = MissionState::Active;
    
    // Resume all paused objectives
    for (auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Paused) {
            objective.status = ObjectiveStatus::Active;
        }
    }
    
    std::cout << "Mission resumed: " << current_mission_->name << std::endl;
    return true;
}

void MissionSystem::abort_mission() {
    if (!current_mission_) {
        return;
    }
    
    // Execute cleanup script
    if (!current_mission_->lua_cleanup_script.empty()) {
        execute_lua_script(current_mission_->lua_cleanup_script);
    }
    
    cleanup_mission_entities();
    
    std::cout << "Mission aborted: " << current_mission_->name << std::endl;
    current_mission_.reset();
}

void MissionSystem::complete_mission() {
    if (!current_mission_) {
        return;
    }
    
    current_mission_->state = MissionState::Completed;
    
    // Execute cleanup script
    if (!current_mission_->lua_cleanup_script.empty()) {
        execute_lua_script(current_mission_->lua_cleanup_script);
    }
    
    // Trigger completion callback
    if (on_mission_complete_) {
        on_mission_complete_(current_mission_->id);
    }
    
    std::cout << "Mission completed: " << current_mission_->name << std::endl;
    
    cleanup_mission_entities();
    current_mission_.reset();
}

void MissionSystem::fail_mission(const std::string& reason) {
    if (!current_mission_) {
        return;
    }
    
    current_mission_->state = MissionState::Failed;
    
    // Execute cleanup script
    if (!current_mission_->lua_cleanup_script.empty()) {
        execute_lua_script(current_mission_->lua_cleanup_script);
    }
    
    // Trigger failure callback
    if (on_mission_fail_) {
        on_mission_fail_(reason.empty() ? "Mission failed" : reason);
    }
    
    std::cout << "Mission failed: " << current_mission_->name;
    if (!reason.empty()) {
        std::cout << " (" << reason << ")";
    }
    std::cout << std::endl;
    
    cleanup_mission_entities();
    current_mission_.reset();
}

MissionState MissionSystem::get_mission_state() const {
    if (!current_mission_) {
        return MissionState::NotLoaded;
    }
    
    return current_mission_->state;
}

bool MissionSystem::is_mission_active() const {
    return current_mission_ && current_mission_->state == MissionState::Active;
}

float MissionSystem::get_mission_progress() const {
    if (!current_mission_) {
        return 0.0f;
    }
    
    int completed_objectives = 0;
    int total_objectives = 0;
    
    for (const auto& objective : current_mission_->objectives) {
        if (!objective.is_optional || 
            std::find(current_mission_->required_objective_ids.begin(),
                     current_mission_->required_objective_ids.end(),
                     objective.id) != current_mission_->required_objective_ids.end()) {
            total_objectives++;
            if (objective.status == ObjectiveStatus::Completed) {
                completed_objectives++;
            }
        }
    }
    
    if (total_objectives == 0) {
        return 1.0f;
    }
    
    return static_cast<float>(completed_objectives) / static_cast<float>(total_objectives);
}

bool MissionSystem::start_objective(const std::string& objective_id) {
    if (!current_mission_) {
        return false;
    }
    
    auto it = std::find_if(current_mission_->objectives.begin(), current_mission_->objectives.end(),
        [&objective_id](const MissionObjective& obj) { return obj.id == objective_id; });
    
    if (it == current_mission_->objectives.end()) {
        std::cerr << "MissionSystem: Objective not found: " << objective_id << std::endl;
        return false;
    }
    
    if (it->status != ObjectiveStatus::Pending) {
        return false; // Already started or completed
    }
    
    it->status = ObjectiveStatus::Active;
    it->current_value = 0.0f;
    
    // Execute on_start script
    if (!it->on_start_script.empty()) {
        execute_objective_script(*it, it->on_start_script);
    }
    
    std::cout << "Objective started: " << it->description << std::endl;
    return true;
}

bool MissionSystem::complete_objective(const std::string& objective_id) {
    if (!current_mission_) {
        return false;
    }
    
    auto it = std::find_if(current_mission_->objectives.begin(), current_mission_->objectives.end(),
        [&objective_id](const MissionObjective& obj) { return obj.id == objective_id; });
    
    if (it == current_mission_->objectives.end()) {
        return false;
    }
    
    if (it->status != ObjectiveStatus::Active) {
        return false; // Not active
    }
    
    it->status = ObjectiveStatus::Completed;
    
    // Execute on_complete script
    if (!it->on_complete_script.empty()) {
        execute_objective_script(*it, it->on_complete_script);
    }
    
    // Trigger objective completion callback
    if (on_objective_complete_) {
        on_objective_complete_(objective_id);
    }
    
    std::cout << "Objective completed: " << it->description << std::endl;
    
    // Check if we should start next objectives
    bool found_next = false;
    for (auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Pending) {
            start_objective(objective.id);
            found_next = true;
            break;
        }
    }
    
    return true;
}

bool MissionSystem::fail_objective(const std::string& objective_id) {
    if (!current_mission_) {
        return false;
    }
    
    auto it = std::find_if(current_mission_->objectives.begin(), current_mission_->objectives.end(),
        [&objective_id](const MissionObjective& obj) { return obj.id == objective_id; });
    
    if (it == current_mission_->objectives.end()) {
        return false;
    }
    
    it->status = ObjectiveStatus::Failed;
    
    // Execute on_fail script
    if (!it->on_fail_script.empty()) {
        execute_objective_script(*it, it->on_fail_script);
    }
    
    std::cout << "Objective failed: " << it->description << std::endl;
    return true;
}

const MissionObjective* MissionSystem::get_objective(const std::string& objective_id) const {
    if (!current_mission_) {
        return nullptr;
    }
    
    auto it = std::find_if(current_mission_->objectives.begin(), current_mission_->objectives.end(),
        [&objective_id](const MissionObjective& obj) { return obj.id == objective_id; });
    
    return (it != current_mission_->objectives.end()) ? &(*it) : nullptr;
}

std::vector<const MissionObjective*> MissionSystem::get_active_objectives() const {
    std::vector<const MissionObjective*> active_objectives;
    
    if (!current_mission_) {
        return active_objectives;
    }
    
    for (const auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Active) {
            active_objectives.push_back(&objective);
        }
    }
    
    return active_objectives;
}

Entity MissionSystem::spawn_entity_at_point(const std::string& spawn_point_name) {
    if (!current_mission_ || !world_) {
        return 0; // Invalid entity
    }
    
    auto it = std::find_if(current_mission_->spawn_points.begin(), current_mission_->spawn_points.end(),
        [&spawn_point_name](const MissionSpawnPoint& sp) { return sp.name == spawn_point_name; });
    
    if (it == current_mission_->spawn_points.end()) {
        std::cerr << "MissionSystem: Spawn point not found: " << spawn_point_name << std::endl;
        return 0;
    }
    
    Entity entity = world_->create_entity();
    
    // Add Transform component
    Transform transform;
    transform.position = it->position;
    transform.rotation = it->rotation;
    world_->add_component(entity, transform);
    
    // Add MissionComponent
    MissionComponent mission_comp;
    mission_comp.mission_id = current_mission_->id;
    mission_comp.role = it->entity_type;
    world_->add_component(entity, mission_comp);
    
    // Add Tag component
    Tag tag(it->entity_type);
    world_->add_component(entity, tag);
    
    std::cout << "Spawned entity at " << spawn_point_name << ": " << it->entity_type << std::endl;
    
    return entity;
}

void MissionSystem::register_mission_entity(Entity entity, const std::string& role) {
    if (!world_ || !current_mission_) {
        return;
    }
    
    MissionComponent mission_comp;
    mission_comp.mission_id = current_mission_->id;
    mission_comp.role = role;
    
    if (world_->has_component<MissionComponent>(entity)) {
        world_->get_component<MissionComponent>(entity) = mission_comp;
    } else {
        world_->add_component(entity, mission_comp);
    }
}

void MissionSystem::unregister_mission_entity(Entity entity) {
    if (!world_) {
        return;
    }
    
    if (world_->has_component<MissionComponent>(entity)) {
        world_->remove_component<MissionComponent>(entity);
    }
}

bool MissionSystem::is_mission_entity(Entity entity) const {
    if (!world_) {
        return false;
    }
    
    return world_->has_component<MissionComponent>(entity);
}

bool MissionSystem::execute_lua_script(const std::string& script) {
#ifdef COREPULSE_HAS_LUA
    if (!lua_state_ || script.empty()) {
        return false;
    }
    
    int result = luaL_dostring(lua_state_, script.c_str());
    if (result != LUA_OK) {
        handle_lua_error("Script execution");
        return false;
    }
    
    return true;
#else
    if (!script.empty()) {
        std::cout << "MissionSystem: Lua script skipped (no Lua support): " << script.substr(0, 50) << "..." << std::endl;
    }
    return false;
#endif
}

void MissionSystem::register_lua_functions() {
#ifdef COREPULSE_HAS_LUA
    if (!lua_state_) {
        return;
    }
    
    // Register mission system functions for Lua
    // This would include functions like:
    // - complete_objective(id)
    // - fail_objective(id)
    // - spawn_entity(spawn_point)
    // - get_mission_time()
    // - etc.
    
    // Example registration (simplified):
    lua_pushcfunction(lua_state_, [](lua_State* L) -> int {
        // This would be a proper C function that calls back to MissionSystem
        return 0;
    });
    lua_setglobal(lua_state_, "mission_complete_objective");
#endif
}

void MissionSystem::update_objectives(float delta_time) {
    if (!current_mission_) {
        return;
    }
    
    for (auto& objective : current_mission_->objectives) {
        if (objective.status == ObjectiveStatus::Active) {
            process_objective(objective, delta_time);
        }
    }
}

void MissionSystem::update_lua_scripts(float delta_time) {
    if (!current_mission_ || current_mission_->lua_update_script.empty()) {
        return;
    }
    
#ifdef COREPULSE_HAS_LUA
    // Set delta_time variable in Lua
    lua_pushnumber(lua_state_, delta_time);
    lua_setglobal(lua_state_, "delta_time");
#endif
    
    // Execute update script
    execute_lua_script(current_mission_->lua_update_script);
}

void MissionSystem::check_mission_completion() {
    if (!current_mission_) {
        return;
    }
    
    if (current_mission_->require_all_objectives) {
        // Check if all required objectives are completed
        for (const auto& objective_id : current_mission_->required_objective_ids) {
            const auto* objective = get_objective(objective_id);
            if (!objective || objective->status != ObjectiveStatus::Completed) {
                return; // Not all required objectives completed
            }
        }
        complete_mission();
    } else {
        // Check if any objective is completed
        for (const auto& objective : current_mission_->objectives) {
            if (objective.status == ObjectiveStatus::Completed && !objective.is_optional) {
                complete_mission();
                return;
            }
        }
    }
}

void MissionSystem::check_mission_failure() {
    if (!current_mission_) {
        return;
    }
    
    // Check if any required objective has failed
    for (const auto& objective_id : current_mission_->required_objective_ids) {
        const auto* objective = get_objective(objective_id);
        if (objective && objective->status == ObjectiveStatus::Failed) {
            fail_mission("Required objective failed: " + objective->description);
            return;
        }
    }
}

void MissionSystem::process_objective(MissionObjective& objective, float delta_time) {
    switch (objective.type) {
        case ObjectiveType::Elimination:
            process_elimination_objective(objective);
            break;
        case ObjectiveType::Escort:
            process_escort_objective(objective);
            break;
        case ObjectiveType::Defend:
            process_defend_objective(objective);
            break;
        case ObjectiveType::Navigate:
            process_navigate_objective(objective);
            break;
        case ObjectiveType::Collect:
            process_collect_objective(objective);
            break;
        case ObjectiveType::Timer:
            process_timer_objective(objective, delta_time);
            break;
        case ObjectiveType::Custom:
            process_custom_objective(objective, delta_time);
            break;
    }
    
    // Execute on_update script
    if (!objective.on_update_script.empty()) {
        execute_objective_script(objective, objective.on_update_script);
    }
}

void MissionSystem::process_elimination_objective(MissionObjective& objective) {
    if (are_all_targets_destroyed(objective.target_entities)) {
        complete_objective(objective.id);
    }
}

void MissionSystem::process_escort_objective(MissionObjective& objective) {
    // Check if escort targets are still alive and at destination
    // This would require more sophisticated logic
}

void MissionSystem::process_defend_objective(MissionObjective& objective) {
    // Check if defended objects are still alive
    // This would require more sophisticated logic
}

void MissionSystem::process_navigate_objective(MissionObjective& objective) {
    // Check if player is close enough to target position
    // This would require access to player entity
}

void MissionSystem::process_collect_objective(MissionObjective& objective) {
    // Check collection progress
    // This would require tracking collected items
}

void MissionSystem::process_timer_objective(MissionObjective& objective, float delta_time) {
    objective.current_value += delta_time;
    if (objective.current_value >= objective.target_value) {
        complete_objective(objective.id);
    }
}

void MissionSystem::process_custom_objective(MissionObjective& objective, float delta_time) {
    if (!objective.lua_script.empty()) {
        execute_objective_script(objective, objective.lua_script);
    }
}

bool MissionSystem::execute_objective_script(const MissionObjective& objective, const std::string& script) {
    if (script.empty()) {
        return false;
    }
    
#ifdef COREPULSE_HAS_LUA
    if (!lua_state_) {
        return false;
    }
    
    // Set objective context in Lua
    lua_pushstring(lua_state_, objective.id.c_str());
    lua_setglobal(lua_state_, "current_objective_id");
    
    lua_pushnumber(lua_state_, objective.current_value);
    lua_setglobal(lua_state_, "current_objective_value");
    
    lua_pushnumber(lua_state_, objective.target_value);
    lua_setglobal(lua_state_, "target_objective_value");
#endif
    
    return execute_lua_script(script);
}

void MissionSystem::handle_lua_error(const std::string& context) {
#ifdef COREPULSE_HAS_LUA
    if (!lua_state_) {
        return;
    }
    
    const char* error_msg = lua_tostring(lua_state_, -1);
    std::cerr << "MissionSystem Lua Error (" << context << "): " << 
                 (error_msg ? error_msg : "Unknown error") << std::endl;
    lua_pop(lua_state_, 1); // Remove error message from stack
#else
    std::cerr << "MissionSystem Error (" << context << "): Lua not available" << std::endl;
#endif
}

void MissionSystem::spawn_mission_entities() {
    if (!current_mission_) {
        return;
    }
    
    // Spawn entities at all spawn points
    for (const auto& spawn_point : current_mission_->spawn_points) {
        if (!spawn_point.is_player_spawn) { // Don't auto-spawn player
            spawn_entity_at_point(spawn_point.name);
        }
    }
}

void MissionSystem::cleanup_mission_entities() {
    if (!world_) {
        return;
    }
    
    // This would require iterating through all entities with MissionComponent
    // and destroying them. For simplicity, we'll leave this as a placeholder.
    // In a full implementation, you'd use the ECS to query for entities with
    // MissionComponent and destroy them.
}

float MissionSystem::calculate_distance_to_target(Entity entity, const glm::vec3& target) const {
    if (!world_ || !world_->has_component<Transform>(entity)) {
        return std::numeric_limits<float>::max();
    }
    
    const auto& transform = world_->get_component<Transform>(entity);
    return glm::distance(transform.position, target);
}

bool MissionSystem::are_all_targets_destroyed(const std::vector<Entity>& targets) const {
    if (!world_) {
        return false;
    }
    
    for (Entity target : targets) {
        if (is_entity_alive(target)) {
            return false;
        }
    }
    
    return !targets.empty(); // True if we had targets and they're all gone
}

bool MissionSystem::is_entity_alive(Entity entity) const {
    if (!world_) {
        return false;
    }
    
    return world_->is_valid_entity(entity);
}

} // namespace CorePulse