#include "World.h"
#include <iostream>

namespace CorePulse {

World::World() {
    entity_manager_ = std::make_unique<EntityManager>();
    component_manager_ = std::make_unique<ComponentManager>();
    system_manager_ = std::make_unique<SystemManager>();
}

void World::init() {
    if (initialized_) {
        std::cout << "Warning: World already initialized" << std::endl;
        return;
    }
    
    std::cout << "Initializing ECS World..." << std::endl;
    
    // Register core components
    register_core_components();
    
    // Initialize all systems
    system_manager_->init_all_systems();
    
    initialized_ = true;
    std::cout << "ECS World initialized successfully" << std::endl;
}

void World::shutdown() {
    if (!initialized_) return;
    
    std::cout << "Shutting down ECS World..." << std::endl;
    
    // Shutdown all systems
    system_manager_->shutdown_all_systems();
    
    initialized_ = false;
    std::cout << "ECS World shutdown complete" << std::endl;
}

void World::update(float delta_time) {
    if (!initialized_) return;
    
    // Update all systems
    system_manager_->update_all_systems(delta_time);
}

Entity World::create_entity() {
    return entity_manager_->create_entity();
}

void World::destroy_entity(Entity entity) {
    if (!entity_manager_->is_valid(entity)) {
        return;
    }
    
    entity_manager_->destroy_entity(entity);
    component_manager_->entity_destroyed(entity);
    system_manager_->entity_destroyed(entity);
}

bool World::is_valid_entity(Entity entity) const {
    return entity_manager_->is_valid(entity);
}

size_t World::get_entity_count() const {
    return entity_manager_->get_living_entity_count();
}

size_t World::get_system_count() const {
    return system_manager_->get_system_count();
}

void World::register_core_components() {
    register_component<Transform>();
    register_component<Renderable>();
    register_component<Velocity>();
    register_component<Tag>();
    register_component<Lifetime>();
    register_component<AutoRotate>();
}

} // namespace CorePulse