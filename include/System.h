#pragma once

#include "Entity.h"
#include <set>
#include <memory>
#include <unordered_map>
#include <typeinfo>

namespace CorePulse {

// Base system class
class System {
public:
    virtual ~System() = default;
    
    // Entities that match this system's component requirements
    std::set<Entity> entities;
    
    // Virtual methods for derived systems to override
    virtual void init() {}
    virtual void update(float delta_time) = 0;
    virtual void shutdown() {}
    
    // Called when an entity is added/removed from this system
    virtual void entity_added(Entity entity) {}
    virtual void entity_removed(Entity entity) {}
};

class SystemManager {
public:
    SystemManager() = default;
    ~SystemManager() = default;
    
    // Non-copyable but movable
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = default;
    SystemManager& operator=(SystemManager&&) = default;
    
    template<typename T>
    std::shared_ptr<T> register_system() {
        const char* type_name = typeid(T).name();
        
        if (systems_.contains(type_name)) {
            return std::static_pointer_cast<T>(systems_[type_name]);
        }
        
        auto system = std::make_shared<T>();
        systems_[type_name] = system;
        return system;
    }
    
    template<typename T>
    void set_signature(Signature signature) {
        const char* type_name = typeid(T).name();
        
        if (!systems_.contains(type_name)) {
            throw std::runtime_error("System not registered before setting signature");
        }
        
        signatures_[type_name] = signature;
    }
    
    template<typename T>
    std::shared_ptr<T> get_system() {
        const char* type_name = typeid(T).name();
        
        auto it = systems_.find(type_name);
        if (it == systems_.end()) {
            return nullptr;
        }
        
        return std::static_pointer_cast<T>(it->second);
    }
    
    void entity_destroyed(Entity entity) {
        // Remove the entity from all systems
        for (auto const& pair : systems_) {
            auto const& system = pair.second;
            if (system->entities.contains(entity)) {
                system->entities.erase(entity);
                system->entity_removed(entity);
            }
        }
    }
    
    void entity_signature_changed(Entity entity, Signature entity_signature) {
        // Notify each system that an entity's signature changed
        for (auto const& pair : systems_) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& system_signature = signatures_[type];
            
            // Entity signature matches system signature - insert into set
            if ((entity_signature & system_signature) == system_signature) {
                if (!system->entities.contains(entity)) {
                    system->entities.insert(entity);
                    system->entity_added(entity);
                }
            }
            // Entity signature does not match system signature - erase from set
            else {
                if (system->entities.contains(entity)) {
                    system->entities.erase(entity);
                    system->entity_removed(entity);
                }
            }
        }
    }
    
    void init_all_systems() {
        for (auto const& pair : systems_) {
            pair.second->init();
        }
    }
    
    void update_all_systems(float delta_time) {
        for (auto const& pair : systems_) {
            pair.second->update(delta_time);
        }
    }
    
    void shutdown_all_systems() {
        for (auto const& pair : systems_) {
            pair.second->shutdown();
        }
    }
    
    size_t get_system_count() const {
        return systems_.size();
    }
    
private:
    // Map from system type string to a signature
    std::unordered_map<const char*, Signature> signatures_;
    
    // Map from system type string to a system pointer
    std::unordered_map<const char*, std::shared_ptr<System>> systems_;
};

} // namespace CorePulse