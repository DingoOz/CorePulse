#include "AssetManager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace CorePulse {

AssetManager::AssetManager() {
    config_.load_textures = true;
    config_.load_materials = true;
    config_.validate_on_load = true;
    config_.generate_fallback_materials = true;
    
    std::cout << "AssetManager: Initialized" << std::endl;
}

bool AssetManager::register_asset(const std::string& id, const std::string& filepath, AssetType type, const std::string& name) {
    clear_error();
    
    if (id.empty()) {
        set_error("Asset ID cannot be empty");
        return false;
    }
    
    if (is_registered(id)) {
        set_error("Asset '" + id + "' is already registered");
        return false;
    }
    
    if (!std::filesystem::exists(filepath)) {
        set_error("Asset file does not exist: " + filepath);
        return false;
    }
    
    AssetInfo info;
    info.id = id;
    info.filepath = filepath;
    info.type = type;
    info.name = name.empty() ? generate_asset_name(filepath, type) : name;
    info.loaded = false;
    
    asset_registry_[id] = info;
    
    std::cout << "AssetManager: Registered " << get_asset_type_string(type) 
              << " asset '" << id << "' -> " << filepath << std::endl;
    
    return true;
}

bool AssetManager::load_asset(const std::string& id) {
    clear_error();
    
    if (!is_registered(id)) {
        set_error("Asset '" + id + "' is not registered");
        return false;
    }
    
    if (is_loaded(id)) {
        std::cout << "AssetManager: Asset '" << id << "' already loaded" << std::endl;
        return true;
    }
    
    return load_asset_internal(id);
}

bool AssetManager::load_asset_internal(const std::string& id) {
    const auto& info = asset_registry_[id];
    
    std::cout << "AssetManager: Loading " << get_asset_type_string(info.type) 
              << " asset '" << id << "' from " << info.filepath << std::endl;
    
    // Load GLTF file
    if (!gltf_loader_.load_gltf(info.filepath)) {
        set_error("Failed to load GLTF file: " + gltf_loader_.get_error());
        return false;
    }
    
    // Create loaded asset
    LoadedAsset asset;
    asset.info = info;
    asset.info.loaded = true;
    asset.gltf_document = gltf_loader_.get_document();
    
    // Extract meshes
    if (config_.load_materials) {
        asset.meshes = gltf_loader_.extract_meshes();
        asset.info.mesh_count = asset.meshes.size();
        
        if (asset.meshes.empty()) {
            set_error("No valid meshes found in asset '" + id + "'");
            return false;
        }
    }
    
    // Extract materials and textures
    if (config_.load_materials) {
        asset.materials = gltf_loader_.extract_materials();
        asset.info.material_count = asset.materials.size();
        
        if (config_.load_textures) {
            asset.textures = gltf_loader_.extract_textures();
        }
        
        // Generate fallback materials if needed
        if (config_.generate_fallback_materials) {
            generate_asset_fallbacks(asset);
        }
    }
    
    // Extract mech-specific extensions for MECH assets
    if (asset.info.type == AssetType::MECH) {
        std::cout << "AssetManager: Extracting mech extensions for '" << id << "'" << std::endl;
        
        // Extract hardpoints
        auto hardpoints = gltf_loader_.extract_hardpoints();
        if (hardpoints.has_value()) {
            asset.hardpoints = hardpoints;
            std::cout << "AssetManager: Found " << asset.hardpoints->hardpoints.size() << " hardpoints" << std::endl;
        }
        
        // Extract damage zones
        auto damage_zones = gltf_loader_.extract_damage_zones();
        if (damage_zones.has_value()) {
            asset.damage_zones = damage_zones;
            std::cout << "AssetManager: Found " << asset.damage_zones->zones.size() << " damage zones" << std::endl;
        }
        
        // Generate basic mech configuration if extensions were found
        if (asset.hardpoints.has_value() || asset.damage_zones.has_value()) {
            MechConfiguration config;
            config.mech_id = id;
            config.variant_name = asset.info.name;
            
            // Extract basic stats from damage zones if available
            if (asset.damage_zones.has_value()) {
                float total_armor = 0.0f;
                for (const auto& zone : asset.damage_zones->zones) {
                    total_armor += zone.max_armor;
                }
                // Estimate tonnage based on armor (rough approximation)
                config.tonnage = total_armor / 10.0f; // Example: 1 ton per 10 armor points
            }
            
            asset.mech_config = config;
            std::cout << "AssetManager: Generated mech config for '" << id << "' (" << config.tonnage << " tons)" << std::endl;
        }
    }
    
    // Validate asset if requested
    if (config_.validate_on_load && !validate_asset(asset)) {
        return false;
    }
    
    // Store loaded asset
    loaded_assets_[id] = std::move(asset);
    
    std::cout << "AssetManager: Successfully loaded '" << id << "' - " 
              << asset.info.mesh_count << " meshes, " 
              << asset.info.material_count << " materials" << std::endl;
    
    return true;
}

bool AssetManager::load_all_assets() {
    clear_error();
    
    size_t loaded_count = 0;
    size_t total_count = asset_registry_.size();
    
    std::cout << "AssetManager: Loading all " << total_count << " registered assets..." << std::endl;
    
    for (const auto& [id, info] : asset_registry_) {
        if (!is_loaded(id)) {
            if (load_asset_internal(id)) {
                loaded_count++;
            } else {
                std::cerr << "AssetManager: Failed to load asset '" << id << "': " << get_last_error() << std::endl;
                clear_error(); // Continue loading other assets
            }
        } else {
            loaded_count++;
        }
    }
    
    std::cout << "AssetManager: Loaded " << loaded_count << "/" << total_count << " assets" << std::endl;
    return loaded_count == total_count;
}

void AssetManager::unload_asset(const std::string& id) {
    auto it = loaded_assets_.find(id);
    if (it != loaded_assets_.end()) {
        std::cout << "AssetManager: Unloading asset '" << id << "'" << std::endl;
        loaded_assets_.erase(it);
        
        // Update registry
        if (is_registered(id)) {
            asset_registry_[id].loaded = false;
        }
    }
}

void AssetManager::unload_all_assets() {
    std::cout << "AssetManager: Unloading all " << loaded_assets_.size() << " assets" << std::endl;
    
    loaded_assets_.clear();
    
    // Update registry
    for (auto& [id, info] : asset_registry_) {
        info.loaded = false;
    }
}

const LoadedAsset* AssetManager::get_asset(const std::string& id) const {
    auto it = loaded_assets_.find(id);
    return (it != loaded_assets_.end()) ? &it->second : nullptr;
}

LoadedAsset* AssetManager::get_asset(const std::string& id) {
    auto it = loaded_assets_.find(id);
    return (it != loaded_assets_.end()) ? &it->second : nullptr;
}

std::vector<std::string> AssetManager::get_assets_by_type(AssetType type) const {
    std::vector<std::string> result;
    
    for (const auto& [id, info] : asset_registry_) {
        if (info.type == type) {
            result.push_back(id);
        }
    }
    
    return result;
}

std::vector<std::string> AssetManager::get_loaded_assets() const {
    std::vector<std::string> result;
    result.reserve(loaded_assets_.size());
    
    for (const auto& [id, asset] : loaded_assets_) {
        result.push_back(id);
    }
    
    return result;
}

bool AssetManager::is_registered(const std::string& id) const {
    return asset_registry_.find(id) != asset_registry_.end();
}

bool AssetManager::is_loaded(const std::string& id) const {
    return loaded_assets_.find(id) != loaded_assets_.end();
}

std::string AssetManager::get_asset_type_string(AssetType type) const {
    switch (type) {
        case AssetType::MECH: return "MECH";
        case AssetType::WEAPON: return "WEAPON";
        case AssetType::ENVIRONMENT: return "ENVIRONMENT";
        case AssetType::EQUIPMENT: return "EQUIPMENT";
        case AssetType::EFFECT: return "EFFECT";
        default: return "UNKNOWN";
    }
}

void AssetManager::print_asset_summary() const {
    std::cout << "\n=== AssetManager Summary ===" << std::endl;
    std::cout << "Registered assets: " << asset_registry_.size() << std::endl;
    std::cout << "Loaded assets: " << loaded_assets_.size() << std::endl;
    
    // Count by type
    std::unordered_map<AssetType, size_t> type_counts;
    for (const auto& [id, info] : asset_registry_) {
        type_counts[info.type]++;
    }
    
    std::cout << "\nBy type:" << std::endl;
    for (const auto& [type, count] : type_counts) {
        std::cout << "  " << get_asset_type_string(type) << ": " << count << std::endl;
    }
    
    if (!loaded_assets_.empty()) {
        std::cout << "\nLoaded assets:" << std::endl;
        for (const auto& [id, asset] : loaded_assets_) {
            std::cout << "  " << id << " (" << get_asset_type_string(asset.info.type) << ") - "
                      << asset.meshes.size() << " meshes, " << asset.materials.size() << " materials" << std::endl;
        }
    }
    std::cout << "============================\n" << std::endl;
}

void AssetManager::print_loading_stats() const {
    size_t total_meshes = 0;
    size_t total_materials = 0;
    size_t total_textures = 0;
    
    for (const auto& [id, asset] : loaded_assets_) {
        total_meshes += asset.meshes.size();
        total_materials += asset.materials.size();
        total_textures += asset.textures.size();
    }
    
    std::cout << "AssetManager Stats: " << loaded_assets_.size() << " assets, "
              << total_meshes << " meshes, " << total_materials << " materials, "
              << total_textures << " textures" << std::endl;
}

bool AssetManager::validate_asset(const LoadedAsset& asset) const {
    if (!asset.is_valid()) {
        set_error("Asset validation failed: invalid asset structure");
        return false;
    }
    
    if (asset.meshes.empty()) {
        set_error("Asset validation failed: no meshes found");
        return false;
    }
    
    // Check mesh validity
    for (size_t i = 0; i < asset.meshes.size(); ++i) {
        if (!asset.meshes[i] || !asset.meshes[i]->is_valid()) {
            set_error("Asset validation failed: mesh " + std::to_string(i) + " is invalid");
            return false;
        }
    }
    
    return true;
}

void AssetManager::generate_asset_fallbacks(LoadedAsset& asset) const {
    // Ensure we have at least as many materials as meshes
    while (asset.materials.size() < asset.meshes.size()) {
        auto fallback_material = Material::create_default();
        fallback_material->name = "Fallback_" + std::to_string(asset.materials.size());
        asset.materials.push_back(fallback_material);
    }
}

void AssetManager::set_error(const std::string& error) const {
    last_error_ = error;
    std::cerr << "AssetManager Error: " << error << std::endl;
}

std::string AssetManager::generate_asset_name(const std::string& filepath, AssetType type) const {
    std::filesystem::path path(filepath);
    std::string filename = path.stem().string(); // Filename without extension
    return get_asset_type_string(type) + "_" + filename;
}

} // namespace CorePulse