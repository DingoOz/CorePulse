#include "GLTFLoader.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace CorePulse {

GLTFLoader::GLTFLoader() = default;

bool GLTFLoader::load_gltf(const std::string& filepath) {
    loaded_ = false;
    error_message_.clear();
    
    std::cout << "GLTFLoader: Loading glTF file: " << filepath << std::endl;
    
    // Extract base path for relative file references
    base_path_ = std::filesystem::path(filepath).parent_path().string();
    if (!base_path_.empty() && base_path_.back() != '/') {
        base_path_ += '/';
    }
    
    // Read JSON file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        set_error("Could not open file: " + filepath);
        return false;
    }
    
    nlohmann::json json;
    try {
        file >> json;
    } catch (const std::exception& e) {
        set_error("JSON parsing error: " + std::string(e.what()));
        return false;
    }
    
    // Parse the JSON document
    if (!parse_json(json)) {
        return false;
    }
    
    // Load buffer data
    if (!load_buffer_data()) {
        return false;
    }
    
    // Validate the document
    if (!validate_document()) {
        return false;
    }
    
    loaded_ = true;
    std::cout << "GLTFLoader: Successfully loaded glTF file" << std::endl;
    return true;
}

bool GLTFLoader::load_glb(const std::string& filepath) {
    // GLB (binary glTF) support - placeholder for now
    set_error("GLB format not yet implemented");
    return false;
}

bool GLTFLoader::parse_json(const nlohmann::json& json) {
    try {
        // Parse required asset field
        if (!json.contains("asset")) {
            set_error("Missing required 'asset' field");
            return false;
        }
        if (!parse_asset(json["asset"])) return false;
        
        // Parse optional fields
        if (json.contains("scene")) {
            document_.scene = json["scene"].get<uint32_t>();
        }
        
        if (json.contains("scenes")) {
            if (!parse_scenes(json["scenes"])) return false;
        }
        
        if (json.contains("nodes")) {
            if (!parse_nodes(json["nodes"])) return false;
        }
        
        if (json.contains("meshes")) {
            if (!parse_meshes(json["meshes"])) return false;
        }
        
        if (json.contains("materials")) {
            if (!parse_materials(json["materials"])) return false;
        }
        
        if (json.contains("textures")) {
            if (!parse_textures(json["textures"])) return false;
        }
        
        if (json.contains("images")) {
            if (!parse_images(json["images"])) return false;
        }
        
        if (json.contains("samplers")) {
            if (!parse_samplers(json["samplers"])) return false;
        }
        
        if (json.contains("accessors")) {
            if (!parse_accessors(json["accessors"])) return false;
        }
        
        if (json.contains("bufferViews")) {
            if (!parse_buffer_views(json["bufferViews"])) return false;
        }
        
        if (json.contains("buffers")) {
            if (!parse_buffers(json["buffers"])) return false;
        }
        
        // Store extensions and extras
        if (json.contains("extensions")) {
            document_.extensions = json["extensions"];
        }
        
        if (json.contains("extras")) {
            document_.extras = json["extras"];
        }
        
    } catch (const std::exception& e) {
        set_error("JSON parsing error: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_asset(const nlohmann::json& json) {
    try {
        if (json.contains("version")) {
            document_.asset.version = json["version"].get<std::string>();
        }
        
        if (json.contains("generator")) {
            document_.asset.generator = json["generator"].get<std::string>();
        }
        
        if (json.contains("copyright")) {
            document_.asset.copyright = json["copyright"].get<std::string>();
        }
        
        if (json.contains("minVersion")) {
            document_.asset.min_version = json["minVersion"].get<std::string>();
        }
        
        // Validate version
        if (document_.asset.version != "2.0") {
            set_error("Unsupported glTF version: " + document_.asset.version);
            return false;
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing asset: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_scenes(const nlohmann::json& json) {
    try {
        document_.scenes.reserve(json.size());
        
        for (const auto& scene_json : json) {
            GLTF::Scene scene;
            
            if (scene_json.contains("name")) {
                scene.name = scene_json["name"].get<std::string>();
            }
            
            if (scene_json.contains("nodes")) {
                for (const auto& node_index : scene_json["nodes"]) {
                    scene.nodes.push_back(node_index.get<uint32_t>());
                }
            }
            
            document_.scenes.push_back(scene);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing scenes: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_nodes(const nlohmann::json& json) {
    try {
        document_.nodes.reserve(json.size());
        
        for (const auto& node_json : json) {
            GLTF::Node node;
            
            if (node_json.contains("name")) {
                node.name = node_json["name"].get<std::string>();
            }
            
            if (node_json.contains("children")) {
                for (const auto& child_index : node_json["children"]) {
                    node.children.push_back(child_index.get<uint32_t>());
                }
            }
            
            if (node_json.contains("mesh")) {
                node.mesh = node_json["mesh"].get<uint32_t>();
            }
            
            // Parse transformation
            if (node_json.contains("matrix")) {
                auto matrix_data = node_json["matrix"].get<std::vector<float>>();
                if (matrix_data.size() == 16) {
                    // glTF uses column-major order
                    for (int col = 0; col < 4; ++col) {
                        for (int row = 0; row < 4; ++row) {
                            node.matrix[col][row] = matrix_data[col * 4 + row];
                        }
                    }
                }
            } else {
                // Parse TRS components
                if (node_json.contains("translation")) {
                    auto trans = node_json["translation"].get<std::vector<float>>();
                    if (trans.size() == 3) {
                        node.translation = glm::vec3(trans[0], trans[1], trans[2]);
                    }
                }
                
                if (node_json.contains("rotation")) {
                    auto rot = node_json["rotation"].get<std::vector<float>>();
                    if (rot.size() == 4) {
                        node.rotation = glm::vec4(rot[0], rot[1], rot[2], rot[3]);
                    }
                }
                
                if (node_json.contains("scale")) {
                    auto scale = node_json["scale"].get<std::vector<float>>();
                    if (scale.size() == 3) {
                        node.scale = glm::vec3(scale[0], scale[1], scale[2]);
                    }
                }
                
                // Build matrix from TRS
                glm::mat4 T = glm::translate(glm::mat4(1.0f), node.translation);
                glm::mat4 R = glm::mat4_cast(glm::quat(node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z));
                glm::mat4 S = glm::scale(glm::mat4(1.0f), node.scale);
                node.matrix = T * R * S;
            }
            
            document_.nodes.push_back(node);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing nodes: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_meshes(const nlohmann::json& json) {
    try {
        document_.meshes.reserve(json.size());
        
        for (const auto& mesh_json : json) {
            GLTF::Mesh mesh;
            
            if (mesh_json.contains("name")) {
                mesh.name = mesh_json["name"].get<std::string>();
            }
            
            if (mesh_json.contains("primitives")) {
                for (const auto& primitive_json : mesh_json["primitives"]) {
                    GLTF::Primitive primitive;
                    
                    if (primitive_json.contains("attributes")) {
                        for (const auto& [attr_name, accessor_index] : primitive_json["attributes"].items()) {
                            primitive.attributes[attr_name] = accessor_index.get<uint32_t>();
                        }
                    }
                    
                    if (primitive_json.contains("indices")) {
                        primitive.indices = primitive_json["indices"].get<uint32_t>();
                    }
                    
                    if (primitive_json.contains("material")) {
                        primitive.material = primitive_json["material"].get<uint32_t>();
                    }
                    
                    if (primitive_json.contains("mode")) {
                        primitive.mode = primitive_json["mode"].get<uint32_t>();
                    }
                    
                    mesh.primitives.push_back(primitive);
                }
            }
            
            document_.meshes.push_back(mesh);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing meshes: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_materials(const nlohmann::json& json) {
    try {
        document_.materials.reserve(json.size());
        
        for (const auto& material_json : json) {
            GLTF::Material material;
            
            if (material_json.contains("name")) {
                material.name = material_json["name"].get<std::string>();
            }
            
            // Parse PBR metallic-roughness
            if (material_json.contains("pbrMetallicRoughness")) {
                const auto& pbr = material_json["pbrMetallicRoughness"];
                
                if (pbr.contains("baseColorFactor")) {
                    auto factor = pbr["baseColorFactor"].get<std::vector<float>>();
                    if (factor.size() >= 4) {
                        material.pbr_metallic_roughness.base_color_factor = 
                            glm::vec4(factor[0], factor[1], factor[2], factor[3]);
                    }
                }
                
                if (pbr.contains("metallicFactor")) {
                    material.pbr_metallic_roughness.metallic_factor = pbr["metallicFactor"].get<float>();
                }
                
                if (pbr.contains("roughnessFactor")) {
                    material.pbr_metallic_roughness.roughness_factor = pbr["roughnessFactor"].get<float>();
                }
                
                if (pbr.contains("baseColorTexture")) {
                    const auto& tex_info = pbr["baseColorTexture"];
                    if (tex_info.contains("index")) {
                        material.pbr_metallic_roughness.base_color_texture = tex_info["index"].get<uint32_t>();
                    }
                }
                
                if (pbr.contains("metallicRoughnessTexture")) {
                    const auto& tex_info = pbr["metallicRoughnessTexture"];
                    if (tex_info.contains("index")) {
                        material.pbr_metallic_roughness.metallic_roughness_texture = tex_info["index"].get<uint32_t>();
                    }
                }
            }
            
            // Parse normal texture
            if (material_json.contains("normalTexture")) {
                const auto& tex_info = material_json["normalTexture"];
                if (tex_info.contains("index")) {
                    material.normal_texture = tex_info["index"].get<uint32_t>();
                }
            }
            
            // Parse occlusion texture
            if (material_json.contains("occlusionTexture")) {
                const auto& tex_info = material_json["occlusionTexture"];
                if (tex_info.contains("index")) {
                    material.occlusion_texture = tex_info["index"].get<uint32_t>();
                }
            }
            
            // Parse emissive properties
            if (material_json.contains("emissiveTexture")) {
                const auto& tex_info = material_json["emissiveTexture"];
                if (tex_info.contains("index")) {
                    material.emissive_texture = tex_info["index"].get<uint32_t>();
                }
            }
            
            if (material_json.contains("emissiveFactor")) {
                auto factor = material_json["emissiveFactor"].get<std::vector<float>>();
                if (factor.size() >= 3) {
                    material.emissive_factor = glm::vec3(factor[0], factor[1], factor[2]);
                }
            }
            
            // Parse alpha properties
            if (material_json.contains("alphaMode")) {
                material.alpha_mode = material_json["alphaMode"].get<std::string>();
            }
            
            if (material_json.contains("alphaCutoff")) {
                material.alpha_cutoff = material_json["alphaCutoff"].get<float>();
            }
            
            if (material_json.contains("doubleSided")) {
                material.double_sided = material_json["doubleSided"].get<bool>();
            }
            
            document_.materials.push_back(material);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing materials: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_textures(const nlohmann::json& json) {
    try {
        document_.textures.reserve(json.size());
        
        for (const auto& texture_json : json) {
            GLTF::Texture texture;
            
            if (texture_json.contains("sampler")) {
                texture.sampler = texture_json["sampler"].get<uint32_t>();
            }
            
            if (texture_json.contains("source")) {
                texture.source = texture_json["source"].get<uint32_t>();
            }
            
            document_.textures.push_back(texture);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing textures: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_images(const nlohmann::json& json) {
    try {
        document_.images.reserve(json.size());
        
        for (const auto& image_json : json) {
            GLTF::Image image;
            
            if (image_json.contains("uri")) {
                image.uri = image_json["uri"].get<std::string>();
            }
            
            if (image_json.contains("mimeType")) {
                image.mime_type = image_json["mimeType"].get<std::string>();
            }
            
            if (image_json.contains("bufferView")) {
                image.buffer_view = image_json["bufferView"].get<uint32_t>();
            }
            
            document_.images.push_back(image);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing images: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_samplers(const nlohmann::json& json) {
    try {
        document_.samplers.reserve(json.size());
        
        for (const auto& sampler_json : json) {
            GLTF::Sampler sampler;
            
            if (sampler_json.contains("magFilter")) {
                sampler.mag_filter = sampler_json["magFilter"].get<uint32_t>();
            }
            
            if (sampler_json.contains("minFilter")) {
                sampler.min_filter = sampler_json["minFilter"].get<uint32_t>();
            }
            
            if (sampler_json.contains("wrapS")) {
                sampler.wrap_s = sampler_json["wrapS"].get<uint32_t>();
            }
            
            if (sampler_json.contains("wrapT")) {
                sampler.wrap_t = sampler_json["wrapT"].get<uint32_t>();
            }
            
            document_.samplers.push_back(sampler);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing samplers: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_accessors(const nlohmann::json& json) {
    try {
        document_.accessors.reserve(json.size());
        
        for (const auto& accessor_json : json) {
            GLTF::Accessor accessor;
            
            if (accessor_json.contains("bufferView")) {
                accessor.buffer_view = accessor_json["bufferView"].get<uint32_t>();
            }
            
            if (accessor_json.contains("byteOffset")) {
                accessor.byte_offset = accessor_json["byteOffset"].get<uint32_t>();
            }
            
            if (accessor_json.contains("componentType")) {
                accessor.component_type = static_cast<GLTF::Accessor::ComponentType>(
                    accessor_json["componentType"].get<uint32_t>());
            }
            
            if (accessor_json.contains("normalized")) {
                accessor.normalized = accessor_json["normalized"].get<bool>();
            }
            
            if (accessor_json.contains("count")) {
                accessor.count = accessor_json["count"].get<uint32_t>();
            }
            
            if (accessor_json.contains("type")) {
                std::string type_str = accessor_json["type"].get<std::string>();
                if (type_str == "SCALAR") accessor.type = GLTF::Accessor::Type::SCALAR;
                else if (type_str == "VEC2") accessor.type = GLTF::Accessor::Type::VEC2;
                else if (type_str == "VEC3") accessor.type = GLTF::Accessor::Type::VEC3;
                else if (type_str == "VEC4") accessor.type = GLTF::Accessor::Type::VEC4;
                else if (type_str == "MAT2") accessor.type = GLTF::Accessor::Type::MAT2;
                else if (type_str == "MAT3") accessor.type = GLTF::Accessor::Type::MAT3;
                else if (type_str == "MAT4") accessor.type = GLTF::Accessor::Type::MAT4;
            }
            
            document_.accessors.push_back(accessor);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing accessors: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_buffer_views(const nlohmann::json& json) {
    try {
        document_.buffer_views.reserve(json.size());
        
        for (const auto& buffer_view_json : json) {
            GLTF::BufferView buffer_view;
            
            if (buffer_view_json.contains("buffer")) {
                buffer_view.buffer = buffer_view_json["buffer"].get<uint32_t>();
            }
            
            if (buffer_view_json.contains("byteOffset")) {
                buffer_view.byte_offset = buffer_view_json["byteOffset"].get<uint32_t>();
            }
            
            if (buffer_view_json.contains("byteLength")) {
                buffer_view.byte_length = buffer_view_json["byteLength"].get<uint32_t>();
            }
            
            if (buffer_view_json.contains("byteStride")) {
                buffer_view.byte_stride = buffer_view_json["byteStride"].get<uint32_t>();
            }
            
            if (buffer_view_json.contains("target")) {
                buffer_view.target = buffer_view_json["target"].get<uint32_t>();
            }
            
            document_.buffer_views.push_back(buffer_view);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing buffer views: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::parse_buffers(const nlohmann::json& json) {
    try {
        document_.buffers.reserve(json.size());
        
        for (const auto& buffer_json : json) {
            GLTF::Buffer buffer;
            
            if (buffer_json.contains("uri")) {
                buffer.uri = buffer_json["uri"].get<std::string>();
            }
            
            if (buffer_json.contains("byteLength")) {
                buffer.byte_length = buffer_json["byteLength"].get<uint32_t>();
            }
            
            document_.buffers.push_back(buffer);
        }
        
    } catch (const std::exception& e) {
        set_error("Error parsing buffers: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool GLTFLoader::load_buffer_data() {
    for (auto& buffer : document_.buffers) {
        if (!buffer.uri.empty()) {
            if (!load_external_buffer(buffer)) {
                return false;
            }
        }
    }
    return true;
}

bool GLTFLoader::load_external_buffer(GLTF::Buffer& buffer) {
    std::string buffer_path = base_path_ + buffer.uri;
    
    std::ifstream file(buffer_path, std::ios::binary);
    if (!file.is_open()) {
        set_error("Could not open buffer file: " + buffer_path);
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (file_size != buffer.byte_length) {
        set_error("Buffer file size mismatch for: " + buffer_path);
        return false;
    }
    
    // Read buffer data
    buffer.data.resize(buffer.byte_length);
    file.read(reinterpret_cast<char*>(buffer.data.data()), buffer.byte_length);
    
    if (!file) {
        set_error("Failed to read buffer file: " + buffer_path);
        return false;
    }
    
    std::cout << "GLTFLoader: Loaded buffer " << buffer.uri << " (" << buffer.byte_length << " bytes)" << std::endl;
    return true;
}

std::vector<std::shared_ptr<Mesh>> GLTFLoader::extract_meshes() {
    std::vector<std::shared_ptr<Mesh>> meshes;
    
    if (!loaded_) {
        std::cerr << "GLTFLoader: Cannot extract meshes - no file loaded" << std::endl;
        return meshes;
    }
    
    std::cout << "GLTFLoader: Extracting " << document_.meshes.size() << " meshes..." << std::endl;
    
    for (size_t mesh_index = 0; mesh_index < document_.meshes.size(); ++mesh_index) {
        const auto& gltf_mesh = document_.meshes[mesh_index];
        
        for (size_t prim_index = 0; prim_index < gltf_mesh.primitives.size(); ++prim_index) {
            const auto& primitive = gltf_mesh.primitives[prim_index];
            
            // Extract vertex data
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            
            if (!extract_primitive_data(primitive, vertices, indices)) {
                std::cerr << "GLTFLoader: Failed to extract primitive " << prim_index 
                         << " from mesh " << mesh_index << std::endl;
                continue;
            }
            
            // Create mesh
            auto mesh = std::make_shared<Mesh>();
            if (mesh->create(vertices, indices)) {
                std::string mesh_name = gltf_mesh.name;
                if (gltf_mesh.primitives.size() > 1) {
                    mesh_name += "_primitive_" + std::to_string(prim_index);
                }
                
                meshes.push_back(mesh);
                std::cout << "GLTFLoader: Created mesh '" << mesh_name 
                         << "' with " << vertices.size() << " vertices and " 
                         << indices.size() << " indices" << std::endl;
            } else {
                std::cerr << "GLTFLoader: Failed to create mesh from primitive data" << std::endl;
            }
        }
    }
    
    std::cout << "GLTFLoader: Successfully extracted " << meshes.size() << " mesh objects" << std::endl;
    return meshes;
}

std::vector<std::shared_ptr<Material>> GLTFLoader::extract_materials() {
    std::vector<std::shared_ptr<Material>> materials;
    
    if (!loaded_) {
        std::cerr << "GLTFLoader: Cannot extract materials - no file loaded" << std::endl;
        return materials;
    }
    
    std::cout << "GLTFLoader: Extracting " << document_.materials.size() << " materials..." << std::endl;
    
    // First extract textures for material use
    auto textures = extract_textures();
    
    for (size_t i = 0; i < document_.materials.size(); ++i) {
        const auto& gltf_material = document_.materials[i];
        
        auto material = std::make_shared<Material>();
        material->name = gltf_material.name.empty() ? "Material_" + std::to_string(i) : gltf_material.name;
        
        // Copy PBR properties
        material->base_color_factor = gltf_material.pbr_metallic_roughness.base_color_factor;
        material->metallic_factor = gltf_material.pbr_metallic_roughness.metallic_factor;
        material->roughness_factor = gltf_material.pbr_metallic_roughness.roughness_factor;
        material->emissive_factor = gltf_material.emissive_factor;
        material->alpha_cutoff = gltf_material.alpha_cutoff;
        material->double_sided = gltf_material.double_sided;
        
        // Convert alpha mode
        if (gltf_material.alpha_mode == "OPAQUE") {
            material->alpha_mode = AlphaMode::OPAQUE;
        } else if (gltf_material.alpha_mode == "MASK") {
            material->alpha_mode = AlphaMode::MASK;
        } else if (gltf_material.alpha_mode == "BLEND") {
            material->alpha_mode = AlphaMode::BLEND;
        }
        
        // Assign textures if available
        if (gltf_material.pbr_metallic_roughness.base_color_texture < textures.size()) {
            material->base_color_texture = textures[gltf_material.pbr_metallic_roughness.base_color_texture];
        }
        
        if (gltf_material.pbr_metallic_roughness.metallic_roughness_texture < textures.size()) {
            material->metallic_roughness_texture = textures[gltf_material.pbr_metallic_roughness.metallic_roughness_texture];
        }
        
        if (gltf_material.normal_texture < textures.size()) {
            material->normal_texture = textures[gltf_material.normal_texture];
        }
        
        if (gltf_material.occlusion_texture < textures.size()) {
            material->occlusion_texture = textures[gltf_material.occlusion_texture];
        }
        
        if (gltf_material.emissive_texture < textures.size()) {
            material->emissive_texture = textures[gltf_material.emissive_texture];
        }
        
        materials.push_back(material);
        std::cout << "GLTFLoader: Created material '" << material->name << "'" << std::endl;
    }
    
    std::cout << "GLTFLoader: Successfully extracted " << materials.size() << " materials" << std::endl;
    return materials;
}

std::vector<std::shared_ptr<Texture>> GLTFLoader::extract_textures() {
    std::vector<std::shared_ptr<Texture>> textures;
    
    if (!loaded_) {
        std::cerr << "GLTFLoader: Cannot extract textures - no file loaded" << std::endl;
        return textures;
    }
    
    std::cout << "GLTFLoader: Extracting " << document_.textures.size() << " textures..." << std::endl;
    
    for (size_t i = 0; i < document_.textures.size(); ++i) {
        const auto& gltf_texture = document_.textures[i];
        
        // Get the image source
        if (gltf_texture.source >= document_.images.size()) {
            std::cerr << "GLTFLoader: Invalid image source index " << gltf_texture.source << " for texture " << i << std::endl;
            textures.push_back(nullptr);
            continue;
        }
        
        const auto& gltf_image = document_.images[gltf_texture.source];
        
        std::shared_ptr<Texture> texture = nullptr;
        
        // Load texture from URI if available
        if (!gltf_image.uri.empty()) {
            std::string image_path = base_path_ + gltf_image.uri;
            texture = Texture::create_from_file(image_path);
            
            if (!texture) {
                std::cerr << "GLTFLoader: Failed to load texture image: " << image_path << std::endl;
                texture = Texture::create_white_texture(); // Fallback
            }
        } else if (gltf_image.buffer_view < document_.buffer_views.size()) {
            // Load texture from buffer view (embedded image data)
            std::cerr << "GLTFLoader: Embedded texture loading not yet implemented for texture " << i << std::endl;
            texture = Texture::create_white_texture(); // Fallback
        } else {
            std::cerr << "GLTFLoader: No valid image source for texture " << i << std::endl;
            texture = Texture::create_white_texture(); // Fallback
        }
        
        // Apply sampler settings if available
        if (texture && gltf_texture.sampler < document_.samplers.size()) {
            const auto& sampler = document_.samplers[gltf_texture.sampler];
            
            // Convert glTF filter modes to our enum
            switch (sampler.mag_filter) {
                case 9728: texture->set_mag_filter(TextureFilter::NEAREST); break;
                case 9729: texture->set_mag_filter(TextureFilter::LINEAR); break;
                default: texture->set_mag_filter(TextureFilter::LINEAR); break;
            }
            
            switch (sampler.min_filter) {
                case 9728: texture->set_min_filter(TextureFilter::NEAREST); break;
                case 9729: texture->set_min_filter(TextureFilter::LINEAR); break;
                case 9984: texture->set_min_filter(TextureFilter::NEAREST_MIPMAP_NEAREST); break;
                case 9985: texture->set_min_filter(TextureFilter::LINEAR_MIPMAP_NEAREST); break;
                case 9986: texture->set_min_filter(TextureFilter::NEAREST_MIPMAP_LINEAR); break;
                case 9987: texture->set_min_filter(TextureFilter::LINEAR_MIPMAP_LINEAR); break;
                default: texture->set_min_filter(TextureFilter::LINEAR_MIPMAP_LINEAR); break;
            }
            
            // Convert wrap modes
            switch (sampler.wrap_s) {
                case 33071: texture->set_wrap_s(TextureWrap::CLAMP_TO_EDGE); break;
                case 33648: texture->set_wrap_s(TextureWrap::MIRRORED_REPEAT); break;
                case 10497: texture->set_wrap_s(TextureWrap::REPEAT); break;
                default: texture->set_wrap_s(TextureWrap::REPEAT); break;
            }
            
            switch (sampler.wrap_t) {
                case 33071: texture->set_wrap_t(TextureWrap::CLAMP_TO_EDGE); break;
                case 33648: texture->set_wrap_t(TextureWrap::MIRRORED_REPEAT); break;
                case 10497: texture->set_wrap_t(TextureWrap::REPEAT); break;
                default: texture->set_wrap_t(TextureWrap::REPEAT); break;
            }
        }
        
        textures.push_back(texture);
        
        if (texture) {
            std::cout << "GLTFLoader: Created texture " << i << " from " << gltf_image.uri << std::endl;
        }
    }
    
    std::cout << "GLTFLoader: Successfully extracted " << textures.size() << " textures" << std::endl;
    return textures;
}

bool GLTFLoader::validate_document() const {
    // Basic validation
    if (document_.asset.version != "2.0") {
        return false;
    }
    
    // Validate scene references
    if (document_.scene >= document_.scenes.size()) {
        return false;
    }
    
    std::cout << "GLTFLoader: Document validation passed" << std::endl;
    return true;
}

bool GLTFLoader::extract_primitive_data(const GLTF::Primitive& primitive, 
                                        std::vector<Vertex>& vertices, 
                                        std::vector<uint32_t>& indices) const {
    // Extract positions (required)
    auto pos_it = primitive.attributes.find("POSITION");
    if (pos_it == primitive.attributes.end()) {
        set_error("Primitive missing POSITION attribute");
        return false;
    }
    
    std::vector<glm::vec3> positions = extract_positions(pos_it->second);
    if (positions.empty()) {
        return false;
    }
    
    // Extract normals (optional)
    std::vector<glm::vec3> normals;
    auto norm_it = primitive.attributes.find("NORMAL");
    if (norm_it != primitive.attributes.end()) {
        normals = extract_normals(norm_it->second);
    }
    
    // If no normals provided, create default ones
    if (normals.empty()) {
        normals.resize(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    // Extract texture coordinates (optional)
    std::vector<glm::vec2> tex_coords;
    auto tex_it = primitive.attributes.find("TEXCOORD_0");
    if (tex_it != primitive.attributes.end()) {
        tex_coords = extract_tex_coords(tex_it->second);
    }
    
    // If no texture coordinates provided, create default ones
    if (tex_coords.empty()) {
        tex_coords.resize(positions.size(), glm::vec2(0.0f, 0.0f));
    }
    
    // Ensure all arrays have the same size
    size_t vertex_count = positions.size();
    if (normals.size() != vertex_count || tex_coords.size() != vertex_count) {
        set_error("Vertex attribute arrays have mismatched sizes");
        return false;
    }
    
    // Build vertices
    vertices.clear();
    vertices.reserve(vertex_count);
    
    for (size_t i = 0; i < vertex_count; ++i) {
        vertices.emplace_back(positions[i], normals[i], tex_coords[i]);
    }
    
    // Extract indices
    if (primitive.indices < document_.accessors.size()) {
        indices = extract_indices(primitive.indices);
    } else {
        // Generate sequential indices if none provided
        indices.clear();
        indices.reserve(vertex_count);
        for (size_t i = 0; i < vertex_count; ++i) {
            indices.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return true;
}

std::vector<glm::vec3> GLTFLoader::extract_positions(uint32_t accessor_index) const {
    if (accessor_index >= document_.accessors.size()) {
        set_error("Invalid position accessor index");
        return {};
    }
    
    const auto& accessor = document_.accessors[accessor_index];
    
    // Validate accessor type
    if (accessor.type != GLTF::Accessor::Type::VEC3 || 
        accessor.component_type != GLTF::Accessor::ComponentType::FLOAT) {
        set_error("Position accessor must be VEC3 FLOAT");
        return {};
    }
    
    // Extract raw float data
    auto float_data = extract_accessor_data<float>(accessor_index);
    if (float_data.size() != accessor.count * 3) {
        set_error("Position accessor data size mismatch");
        return {};
    }
    
    // Convert to vec3 array
    std::vector<glm::vec3> positions;
    positions.reserve(accessor.count);
    
    for (size_t i = 0; i < accessor.count; ++i) {
        size_t base = i * 3;
        positions.emplace_back(float_data[base], float_data[base + 1], float_data[base + 2]);
    }
    
    return positions;
}

std::vector<glm::vec3> GLTFLoader::extract_normals(uint32_t accessor_index) const {
    if (accessor_index >= document_.accessors.size()) {
        return {};
    }
    
    const auto& accessor = document_.accessors[accessor_index];
    
    if (accessor.type != GLTF::Accessor::Type::VEC3 || 
        accessor.component_type != GLTF::Accessor::ComponentType::FLOAT) {
        return {};
    }
    
    auto float_data = extract_accessor_data<float>(accessor_index);
    if (float_data.size() != accessor.count * 3) {
        return {};
    }
    
    std::vector<glm::vec3> normals;
    normals.reserve(accessor.count);
    
    for (size_t i = 0; i < accessor.count; ++i) {
        size_t base = i * 3;
        normals.emplace_back(float_data[base], float_data[base + 1], float_data[base + 2]);
    }
    
    return normals;
}

std::vector<glm::vec2> GLTFLoader::extract_tex_coords(uint32_t accessor_index) const {
    if (accessor_index >= document_.accessors.size()) {
        return {};
    }
    
    const auto& accessor = document_.accessors[accessor_index];
    
    if (accessor.type != GLTF::Accessor::Type::VEC2 || 
        accessor.component_type != GLTF::Accessor::ComponentType::FLOAT) {
        return {};
    }
    
    auto float_data = extract_accessor_data<float>(accessor_index);
    if (float_data.size() != accessor.count * 2) {
        return {};
    }
    
    std::vector<glm::vec2> tex_coords;
    tex_coords.reserve(accessor.count);
    
    for (size_t i = 0; i < accessor.count; ++i) {
        size_t base = i * 2;
        tex_coords.emplace_back(float_data[base], float_data[base + 1]);
    }
    
    return tex_coords;
}

std::vector<uint32_t> GLTFLoader::extract_indices(uint32_t accessor_index) const {
    if (accessor_index >= document_.accessors.size()) {
        return {};
    }
    
    const auto& accessor = document_.accessors[accessor_index];
    
    if (accessor.type != GLTF::Accessor::Type::SCALAR) {
        return {};
    }
    
    std::vector<uint32_t> indices;
    indices.reserve(accessor.count);
    
    switch (accessor.component_type) {
        case GLTF::Accessor::ComponentType::UNSIGNED_BYTE: {
            auto data = extract_accessor_data<uint8_t>(accessor_index);
            for (uint8_t index : data) {
                indices.push_back(static_cast<uint32_t>(index));
            }
            break;
        }
        case GLTF::Accessor::ComponentType::UNSIGNED_SHORT: {
            auto data = extract_accessor_data<uint16_t>(accessor_index);
            for (uint16_t index : data) {
                indices.push_back(static_cast<uint32_t>(index));
            }
            break;
        }
        case GLTF::Accessor::ComponentType::UNSIGNED_INT: {
            indices = extract_accessor_data<uint32_t>(accessor_index);
            break;
        }
        default:
            set_error("Unsupported index component type");
            return {};
    }
    
    return indices;
}

template<typename T>
std::vector<T> GLTFLoader::extract_accessor_data(uint32_t accessor_index) const {
    if (accessor_index >= document_.accessors.size()) {
        return {};
    }
    
    const auto& accessor = document_.accessors[accessor_index];
    
    if (accessor.buffer_view >= document_.buffer_views.size()) {
        return {};
    }
    
    const auto& buffer_view = document_.buffer_views[accessor.buffer_view];
    
    if (buffer_view.buffer >= document_.buffers.size()) {
        return {};
    }
    
    const auto& buffer = document_.buffers[buffer_view.buffer];
    
    // Calculate byte offset into buffer
    size_t byte_offset = buffer_view.byte_offset + accessor.byte_offset;
    size_t component_size = sizeof(T);
    size_t total_components = accessor.count;
    
    // Determine number of components per element
    switch (accessor.type) {
        case GLTF::Accessor::Type::SCALAR: total_components *= 1; break;
        case GLTF::Accessor::Type::VEC2: total_components *= 2; break;
        case GLTF::Accessor::Type::VEC3: total_components *= 3; break;
        case GLTF::Accessor::Type::VEC4: total_components *= 4; break;
        case GLTF::Accessor::Type::MAT2: total_components *= 4; break;
        case GLTF::Accessor::Type::MAT3: total_components *= 9; break;
        case GLTF::Accessor::Type::MAT4: total_components *= 16; break;
    }
    
    // Validate buffer bounds
    if (byte_offset + total_components * component_size > buffer.data.size()) {
        return {};
    }
    
    // Extract data
    std::vector<T> result;
    result.reserve(total_components);
    
    const uint8_t* data_ptr = buffer.data.data() + byte_offset;
    
    for (size_t i = 0; i < total_components; ++i) {
        T value;
        std::memcpy(&value, data_ptr + i * component_size, component_size);
        result.push_back(value);
    }
    
    return result;
}

void GLTFLoader::set_error(const std::string& message) const {
    error_message_ = message;
    std::cerr << "GLTFLoader Error: " << message << std::endl;
}

} // namespace CorePulse