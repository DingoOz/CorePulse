#pragma once

#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "Components.h"
#include <memory>

namespace CorePulse {

class World {
public:
    World();
    ~World() = default;
    
    // Non-copyable but movable
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;
    
    // Initialization
    void init();
    void shutdown();
    
    // Update loop
    void update(float delta_time);
    
    // Entity management
    Entity create_entity();
    void destroy_entity(Entity entity);
    bool is_valid_entity(Entity entity) const;
    
    // Component management
    template<Component T>
    void register_component() {
        component_manager_->register_component<T>();
    }
    
    template<Component T>
    void add_component(Entity entity, T component) {
        component_manager_->add_component<T>(entity, component);
        
        auto signature = entity_manager_->get_signature(entity);
        signature |= (1 << component_manager_->get_component_type<T>());
        entity_manager_->set_signature(entity, signature);
        
        system_manager_->entity_signature_changed(entity, signature);
    }
    
    template<Component T>
    void remove_component(Entity entity) {
        component_manager_->remove_component<T>(entity);
        
        auto signature = entity_manager_->get_signature(entity);
        signature &= ~(1 << component_manager_->get_component_type<T>());
        entity_manager_->set_signature(entity, signature);
        
        system_manager_->entity_signature_changed(entity, signature);
    }
    
    template<Component T>
    T& get_component(Entity entity) {
        return component_manager_->get_component<T>(entity);
    }
    
    template<Component T>
    const T& get_component(Entity entity) const {
        return component_manager_->get_component<T>(entity);
    }
    
    template<Component T>
    bool has_component(Entity entity) const {
        return component_manager_->has_component<T>(entity);
    }
    
    template<Component T>
    ComponentType get_component_type() {
        return component_manager_->get_component_type<T>();
    }
    
    // System management
    template<typename T>
    std::shared_ptr<T> register_system() {
        return system_manager_->register_system<T>();
    }
    
    template<typename T>
    void set_system_signature(Signature signature) {
        system_manager_->set_signature<T>(signature);
    }
    
    template<typename T>
    std::shared_ptr<T> get_system() {
        return system_manager_->get_system<T>();
    }
    
    // Statistics
    size_t get_entity_count() const;
    size_t get_system_count() const;
    
    // Access to managers (for advanced use cases)
    EntityManager& get_entity_manager() { return *entity_manager_; }
    ComponentManager& get_component_manager() { return *component_manager_; }
    SystemManager& get_system_manager() { return *system_manager_; }
    
private:
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<ComponentManager> component_manager_;
    std::unique_ptr<SystemManager> system_manager_;
    
    bool initialized_ = false;
    
    void register_core_components();
};

} // namespace CorePulse