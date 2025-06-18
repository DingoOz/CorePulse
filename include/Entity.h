#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <vector>
#include <array>
#include <algorithm>

namespace CorePulse {

// Entity is just a unique identifier
using Entity = uint32_t;

// Special entity values
constexpr Entity NULL_ENTITY = 0;
constexpr Entity MAX_ENTITIES = 10000; // Reasonable limit for a game

// Component type identifier
using ComponentType = uint32_t;
constexpr ComponentType MAX_COMPONENTS = 32;

// Signature is a bitset representing which components an entity has
using Signature = uint32_t;

class EntityManager {
public:
    EntityManager();
    ~EntityManager() = default;
    
    // Non-copyable but movable
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = default;
    EntityManager& operator=(EntityManager&&) = default;
    
    // Entity lifecycle
    Entity create_entity();
    void destroy_entity(Entity entity);
    bool is_valid(Entity entity) const;
    
    // Signature management
    void set_signature(Entity entity, Signature signature);
    Signature get_signature(Entity entity) const;
    
    // Statistics
    size_t get_living_entity_count() const { return living_entity_count_; }
    size_t get_total_entities_created() const { return entities_created_; }
    
private:
    // Entity recycling queue
    std::vector<Entity> available_entities_;
    
    // Entity signatures
    std::array<Signature, MAX_ENTITIES> signatures_;
    
    // Statistics
    size_t living_entity_count_ = 0;
    size_t entities_created_ = 0;
    Entity next_entity_id_ = 1; // Start from 1, 0 is NULL_ENTITY
};

} // namespace CorePulse