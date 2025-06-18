#include "Entity.h"
#include <iostream>
#include <algorithm>

namespace CorePulse {

EntityManager::EntityManager() {
    // Pre-populate the available entities queue
    for (Entity entity = 1; entity < MAX_ENTITIES; ++entity) {
        available_entities_.push_back(entity);
    }
}

Entity EntityManager::create_entity() {
    if (available_entities_.empty()) {
        std::cerr << "Error: Maximum number of entities reached!" << std::endl;
        return NULL_ENTITY;
    }
    
    Entity entity = available_entities_.back();
    available_entities_.pop_back();
    
    ++living_entity_count_;
    ++entities_created_;
    
    // Initialize signature to 0 (no components)
    signatures_[entity] = 0;
    
    return entity;
}

void EntityManager::destroy_entity(Entity entity) {
    if (!is_valid(entity)) {
        std::cerr << "Warning: Attempting to destroy invalid entity: " << entity << std::endl;
        return;
    }
    
    // Reset signature
    signatures_[entity] = 0;
    
    // Return entity to available queue
    available_entities_.push_back(entity);
    
    --living_entity_count_;
}

bool EntityManager::is_valid(Entity entity) const {
    if (entity == NULL_ENTITY || entity >= MAX_ENTITIES) {
        return false;
    }
    
    // Check if entity is not in the available queue (meaning it's alive)
    return std::find(available_entities_.begin(), available_entities_.end(), entity) == available_entities_.end();
}

void EntityManager::set_signature(Entity entity, Signature signature) {
    if (!is_valid(entity)) {
        std::cerr << "Warning: Attempting to set signature on invalid entity: " << entity << std::endl;
        return;
    }
    
    signatures_[entity] = signature;
}

Signature EntityManager::get_signature(Entity entity) const {
    if (!is_valid(entity)) {
        return 0;
    }
    
    return signatures_[entity];
}

} // namespace CorePulse