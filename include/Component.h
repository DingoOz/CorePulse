#pragma once

#include "Entity.h"
#include <array>
#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <concepts>

namespace CorePulse {

// C++20 concept to ensure a type is a valid component
template<typename T>
concept Component = std::is_default_constructible_v<T> && 
                   std::is_destructible_v<T> && 
                   std::is_copy_constructible_v<T>;

// Base interface for component arrays
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entity_destroyed(Entity entity) = 0;
    virtual size_t size() const = 0;
};

// Templated component array for specific component types
template<Component T>
class ComponentArray : public IComponentArray {
public:
    void insert_data(Entity entity, T component) {
        if (entity_to_index_map_.contains(entity)) {
            // Update existing component
            size_t index = entity_to_index_map_[entity];
            component_array_[index] = component;
            return;
        }
        
        // Add new component
        size_t new_index = size_;
        entity_to_index_map_[entity] = new_index;
        index_to_entity_map_[new_index] = entity;
        component_array_[new_index] = component;
        ++size_;
    }
    
    void remove_data(Entity entity) {
        if (!entity_to_index_map_.contains(entity)) {
            return;
        }
        
        // Copy element at end into deleted element's place to maintain density
        size_t index_of_removed_entity = entity_to_index_map_[entity];
        size_t index_of_last_element = size_ - 1;
        component_array_[index_of_removed_entity] = component_array_[index_of_last_element];
        
        // Update map to point to moved spot
        Entity entity_of_last_element = index_to_entity_map_[index_of_last_element];
        entity_to_index_map_[entity_of_last_element] = index_of_removed_entity;
        index_to_entity_map_[index_of_removed_entity] = entity_of_last_element;
        
        entity_to_index_map_.erase(entity);
        index_to_entity_map_.erase(index_of_last_element);
        
        --size_;
    }
    
    T& get_data(Entity entity) {
        if (!entity_to_index_map_.contains(entity)) {
            throw std::runtime_error("Entity does not have this component");
        }
        
        return component_array_[entity_to_index_map_[entity]];
    }
    
    const T& get_data(Entity entity) const {
        if (!entity_to_index_map_.contains(entity)) {
            throw std::runtime_error("Entity does not have this component");
        }
        
        return component_array_[entity_to_index_map_.at(entity)];
    }
    
    bool has_data(Entity entity) const {
        return entity_to_index_map_.contains(entity);
    }
    
    void entity_destroyed(Entity entity) override {
        if (entity_to_index_map_.contains(entity)) {
            remove_data(entity);
        }
    }
    
    size_t size() const override {
        return size_;
    }
    
    // Iterator support
    auto begin() { return component_array_.begin(); }
    auto end() { return component_array_.begin() + size_; }
    auto begin() const { return component_array_.begin(); }
    auto end() const { return component_array_.begin() + size_; }
    
private:
    // Dense array of components
    std::array<T, MAX_ENTITIES> component_array_;
    
    // Map from entity ID to array index
    std::unordered_map<Entity, size_t> entity_to_index_map_;
    
    // Map from array index to entity ID
    std::unordered_map<size_t, Entity> index_to_entity_map_;
    
    // Total number of valid entries in the array
    size_t size_ = 0;
};

class ComponentManager {
public:
    ComponentManager() = default;
    ~ComponentManager() = default;
    
    // Non-copyable but movable
    ComponentManager(const ComponentManager&) = delete;
    ComponentManager& operator=(const ComponentManager&) = delete;
    ComponentManager(ComponentManager&&) = default;
    ComponentManager& operator=(ComponentManager&&) = default;
    
    template<Component T>
    void register_component() {
        const char* type_name = typeid(T).name();
        
        if (component_types_.contains(type_name)) {
            return; // Already registered
        }
        
        // Add this component type to the component type map
        component_types_[type_name] = next_component_type_;
        
        // Create a ComponentArray pointer and add it to the component arrays map
        component_arrays_[type_name] = std::make_unique<ComponentArray<T>>();
        
        ++next_component_type_;
    }
    
    template<Component T>
    ComponentType get_component_type() {
        const char* type_name = typeid(T).name();
        
        if (!component_types_.contains(type_name)) {
            register_component<T>();
        }
        
        return component_types_[type_name];
    }
    
    template<Component T>
    void add_component(Entity entity, T component) {
        get_component_array<T>()->insert_data(entity, component);
    }
    
    template<Component T>
    void remove_component(Entity entity) {
        get_component_array<T>()->remove_data(entity);
    }
    
    template<Component T>
    T& get_component(Entity entity) {
        return get_component_array<T>()->get_data(entity);
    }
    
    template<Component T>
    const T& get_component(Entity entity) const {
        return get_component_array<T>()->get_data(entity);
    }
    
    template<Component T>
    bool has_component(Entity entity) const {
        return get_component_array<T>()->has_data(entity);
    }
    
    void entity_destroyed(Entity entity) {
        // Notify each component array that an entity has been destroyed
        for (auto const& pair : component_arrays_) {
            auto const& component = pair.second;
            component->entity_destroyed(entity);
        }
    }
    
private:
    // Map from type string pointer to a component type
    std::unordered_map<const char*, ComponentType> component_types_;
    
    // Map from type string pointer to a component array
    std::unordered_map<const char*, std::unique_ptr<IComponentArray>> component_arrays_;
    
    // The component type to be assigned to the next registered component - starting at 0
    ComponentType next_component_type_ = 0;
    
    template<Component T>
    ComponentArray<T>* get_component_array() {
        const char* type_name = typeid(T).name();
        
        if (!component_types_.contains(type_name)) {
            register_component<T>();
        }
        
        return static_cast<ComponentArray<T>*>(component_arrays_[type_name].get());
    }
    
    template<Component T>
    const ComponentArray<T>* get_component_array() const {
        const char* type_name = typeid(T).name();
        
        auto it = component_arrays_.find(type_name);
        if (it == component_arrays_.end()) {
            throw std::runtime_error("Component type not registered");
        }
        
        return static_cast<const ComponentArray<T>*>(it->second.get());
    }
};

} // namespace CorePulse