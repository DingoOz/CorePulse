#pragma once

#include "GLTFLoader.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "MechExtensions.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace CorePulse {

// Asset categories for organization
enum class AssetType {
    MECH,
    WEAPON,
    ENVIRONMENT,
    EQUIPMENT,
    EFFECT
};

// Asset metadata
struct AssetInfo {
    std::string id;
    std::string name;
    std::string filepath;
    AssetType type;
    bool loaded = false;
    size_t mesh_count = 0;
    size_t material_count = 0;
};

// Loaded asset data
struct LoadedAsset {
    AssetInfo info;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Texture>> textures;
    GLTF::Document gltf_document;
    
    // Mech-specific extensions (only populated for MECH assets)
    std::optional<CP_Walker_Hardpoints> hardpoints;
    std::optional<CP_Damage_Zones> damage_zones;
    std::optional<MechConfiguration> mech_config;
    
    bool is_valid() const {
        return !meshes.empty() && info.loaded;
    }
    
    bool is_mech() const {
        return info.type == AssetType::MECH;
    }
    
    bool has_hardpoints() const {
        return hardpoints.has_value() && !hardpoints->hardpoints.empty();
    }
    
    bool has_damage_zones() const {
        return damage_zones.has_value() && !damage_zones->zones.empty();
    }
};

// Asset loading configuration
struct AssetConfig {
    bool load_textures = true;
    bool load_materials = true;
    bool validate_on_load = true;
    bool generate_fallback_materials = true;
};

/**
 * AssetManager - Centralized system for loading and managing GLTF assets
 * 
 * Features:
 * - Multiple asset type support (mechs, weapons, environment)
 * - Efficient loading and caching
 * - Asset organization and categorization
 * - Error handling and validation
 * - Memory management
 */
class AssetManager {
public:
    AssetManager();
    ~AssetManager() = default;
    
    // Non-copyable but movable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = default;
    AssetManager& operator=(AssetManager&&) = default;
    
    // Configuration
    void set_config(const AssetConfig& config) { config_ = config; }
    const AssetConfig& get_config() const { return config_; }
    
    // Asset registration and loading
    bool register_asset(const std::string& id, const std::string& filepath, AssetType type, const std::string& name = "");
    bool load_asset(const std::string& id);
    bool load_all_assets();
    void unload_asset(const std::string& id);
    void unload_all_assets();
    
    // Asset access
    const LoadedAsset* get_asset(const std::string& id) const;
    LoadedAsset* get_asset(const std::string& id);
    std::vector<std::string> get_assets_by_type(AssetType type) const;
    std::vector<std::string> get_loaded_assets() const;
    
    // Asset queries
    bool is_registered(const std::string& id) const;
    bool is_loaded(const std::string& id) const;
    size_t get_registered_count() const { return asset_registry_.size(); }
    size_t get_loaded_count() const { return loaded_assets_.size(); }
    
    // Utility functions
    std::string get_asset_type_string(AssetType type) const;
    void print_asset_summary() const;
    void print_loading_stats() const;
    
    // Error handling
    const std::string& get_last_error() const { return last_error_; }
    bool has_error() const { return !last_error_.empty(); }
    void clear_error() { last_error_.clear(); }

private:
    AssetConfig config_;
    std::unordered_map<std::string, AssetInfo> asset_registry_;
    std::unordered_map<std::string, LoadedAsset> loaded_assets_;
    GLTFLoader gltf_loader_;
    mutable std::string last_error_;
    
    // Internal loading functions
    bool load_asset_internal(const std::string& id);
    bool validate_asset(const LoadedAsset& asset) const;
    void generate_asset_fallbacks(LoadedAsset& asset) const;
    void set_error(const std::string& error) const;
    std::string generate_asset_name(const std::string& filepath, AssetType type) const;
};

} // namespace CorePulse