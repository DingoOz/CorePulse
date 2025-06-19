#include "Renderer.h"
#include <iostream>
#include <GL/glew.h>

namespace CorePulse {

const std::string Renderer::default_vertex_shader_ = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;

void main() {
    frag_pos = vec3(u_model * vec4(a_position, 1.0));
    normal = u_normal_matrix * a_normal;
    tex_coords = a_tex_coords;
    
    gl_Position = u_projection * u_view * vec4(frag_pos, 1.0);
}
)";

const std::string Renderer::default_fragment_shader_ = R"(
#version 330 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform vec3 u_color = vec3(0.8, 0.8, 0.8);
uniform vec3 u_light_pos = vec3(5.0, 5.0, 5.0);
uniform vec3 u_light_color = vec3(1.0, 1.0, 1.0);
uniform vec3 u_view_pos = vec3(0.0, 0.0, 3.0);

void main() {
    // Ambient lighting
    float ambient_strength = 0.2;
    vec3 ambient = ambient_strength * u_light_color;
    
    // Diffuse lighting
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(u_light_pos - frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * u_light_color;
    
    // Specular lighting
    float specular_strength = 0.5;
    vec3 view_dir = normalize(u_view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * u_light_color;
    
    vec3 result = (ambient + diffuse + specular) * u_color;
    frag_color = vec4(result, 1.0);
}
)";

const std::string Renderer::pbr_vertex_shader_ = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;

void main() {
    frag_pos = vec3(u_model * vec4(a_position, 1.0));
    normal = u_normal_matrix * a_normal;
    tex_coords = a_tex_coords;
    
    gl_Position = u_projection * u_view * vec4(frag_pos, 1.0);
}
)";

const std::string Renderer::pbr_fragment_shader_ = R"(
#version 330 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

// Material uniforms
struct Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    vec3 emissiveFactor;
    float alphaCutoff;
    int alphaMode;
    bool doubleSided;
    
    bool hasBaseColorTexture;
    bool hasMetallicRoughnessTexture;
    bool hasNormalTexture;
    bool hasOcclusionTexture;
    bool hasEmissiveTexture;
    
    sampler2D baseColorTexture;
    sampler2D metallicRoughnessTexture;
    sampler2D normalTexture;
    sampler2D occlusionTexture;
    sampler2D emissiveTexture;
};

uniform Material u_material;

// Lighting uniforms
uniform vec3 u_light_pos = vec3(5.0, 5.0, 5.0);
uniform vec3 u_light_color = vec3(1.0, 1.0, 1.0);
uniform vec3 u_view_pos = vec3(0.0, 0.0, 3.0);

const float PI = 3.14159265359;

// PBR functions
vec3 getNormalFromMap() {
    if (!u_material.hasNormalTexture) {
        return normalize(normal);
    }
    
    vec3 tangentNormal = texture(u_material.normalTexture, tex_coords).xyz * 2.0 - 1.0;
    
    vec3 Q1 = dFdx(frag_pos);
    vec3 Q2 = dFdy(frag_pos);
    vec2 st1 = dFdx(tex_coords);
    vec2 st2 = dFdy(tex_coords);
    
    vec3 N = normalize(normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Sample material properties
    vec4 baseColor = u_material.baseColorFactor;
    if (u_material.hasBaseColorTexture) {
        baseColor *= texture(u_material.baseColorTexture, tex_coords);
    }
    
    // Alpha test
    if (u_material.alphaMode == 1) { // MASK
        if (baseColor.a < u_material.alphaCutoff) {
            discard;
        }
    }
    
    float metallic = u_material.metallicFactor;
    float roughness = u_material.roughnessFactor;
    if (u_material.hasMetallicRoughnessTexture) {
        vec3 metallicRoughness = texture(u_material.metallicRoughnessTexture, tex_coords).rgb;
        metallic *= metallicRoughness.b;
        roughness *= metallicRoughness.g;
    }
    
    vec3 emissive = u_material.emissiveFactor;
    if (u_material.hasEmissiveTexture) {
        emissive *= texture(u_material.emissiveTexture, tex_coords).rgb;
    }
    
    float occlusion = 1.0;
    if (u_material.hasOcclusionTexture) {
        occlusion = texture(u_material.occlusionTexture, tex_coords).r;
    }
    
    // Calculate lighting
    vec3 N = getNormalFromMap();
    vec3 V = normalize(u_view_pos - frag_pos);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metallic);
    
    // Simple directional light
    vec3 L = normalize(u_light_pos - frag_pos);
    vec3 H = normalize(V + L);
    float distance = length(u_light_pos - frag_pos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_light_color * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * baseColor.rgb * occlusion;
    vec3 color = ambient + Lo + emissive;
    
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    frag_color = vec4(color, baseColor.a);
}
)";

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (initialized_) {
        std::cout << "Renderer already initialized\n";
        return true;
    }
    
    // Initialize GLEW
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(glew_error) << std::endl;
        return false;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Check OpenGL version
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 or higher is required" << std::endl;
        return false;
    }
    
    // Create default shader
    if (!create_default_shader()) {
        std::cerr << "Failed to create default shader" << std::endl;
        return false;
    }
    
    // Create PBR shader
    if (!create_pbr_shader()) {
        std::cerr << "Failed to create PBR shader" << std::endl;
        return false;
    }
    
    // Set initial render state
    update_render_state();
    
    initialized_ = true;
    std::cout << "Renderer initialized successfully\n";
    return true;
}

void Renderer::shutdown() {
    if (!initialized_) return;
    
    default_shader_ = Shader{};
    pbr_shader_ = Shader{};
    initialized_ = false;
    std::cout << "Renderer shutdown complete\n";
}

void Renderer::begin_frame() {
    if (!initialized_) return;
    
    stats_.reset();
    update_render_state();
}

void Renderer::end_frame() {
    // Nothing specific needed for end frame currently
}

void Renderer::clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera) {
    render_mesh(mesh, model_matrix, camera, default_shader_);
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, Shader& shader) {
    if (!initialized_ || !mesh.is_valid() || !shader.is_valid()) return;
    
    shader.use();
    
    // Set matrices
    shader.set_mat4("u_model", model_matrix);
    shader.set_mat4("u_view", camera.get_view_matrix());
    shader.set_mat4("u_projection", camera.get_projection_matrix());
    
    // Calculate and set normal matrix
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
    shader.set_mat3("u_normal_matrix", normal_matrix);
    
    // Set camera position for lighting
    shader.set_vec3("u_view_pos", camera.get_position());
    
    // Render mesh
    mesh.draw();
    
    // Update statistics
    stats_.draw_calls++;
    stats_.vertices_rendered += mesh.get_vertex_count();
    stats_.triangles_rendered += mesh.get_index_count() / 3;
    
    shader.unuse();
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, const Material& material) {
    render_mesh(mesh, model_matrix, camera, material, pbr_shader_);
}

void Renderer::render_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Camera& camera, const Material& material, Shader& shader) {
    if (!initialized_ || !mesh.is_valid() || !shader.is_valid()) {
        static int warning_count = 0;
        if (warning_count < 3) {
            std::cout << "PBR render skipped - initialized: " << initialized_ 
                     << ", mesh valid: " << mesh.is_valid() 
                     << ", shader valid: " << shader.is_valid() << std::endl;
            warning_count++;
        }
        return;
    }
    
    shader.use();
    
    // Set matrices
    shader.set_mat4("u_model", model_matrix);
    shader.set_mat4("u_view", camera.get_view_matrix());
    shader.set_mat4("u_projection", camera.get_projection_matrix());
    
    // Calculate and set normal matrix
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
    shader.set_mat3("u_normal_matrix", normal_matrix);
    
    // Set camera position for lighting
    shader.set_vec3("u_view_pos", camera.get_position());
    
    // Apply material properties to shader
    material.apply_to_shader(shader);
    
    // Bind material textures
    material.bind_textures();
    
    // Render mesh
    mesh.draw();
    
    // Unbind textures
    material.unbind_textures();
    
    // Update statistics
    stats_.draw_calls++;
    stats_.vertices_rendered += mesh.get_vertex_count();
    stats_.triangles_rendered += mesh.get_index_count() / 3;
    
    shader.unuse();
}

void Renderer::set_wireframe_mode(bool enabled) {
    wireframe_mode_ = enabled;
    update_render_state();
}

void Renderer::set_depth_test(bool enabled) {
    depth_test_enabled_ = enabled;
    update_render_state();
}

void Renderer::set_face_culling(bool enabled) {
    face_culling_enabled_ = enabled;
    update_render_state();
}

bool Renderer::create_default_shader() {
    return default_shader_.load_from_strings(default_vertex_shader_, default_fragment_shader_);
}

bool Renderer::create_pbr_shader() {
    bool success = pbr_shader_.load_from_strings(pbr_vertex_shader_, pbr_fragment_shader_);
    if (success) {
        std::cout << "PBR shader compiled successfully" << std::endl;
    } else {
        std::cerr << "PBR shader compilation failed!" << std::endl;
    }
    return success;
}

void Renderer::update_render_state() {
    // Wireframe mode
    if (wireframe_mode_) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Depth testing
    if (depth_test_enabled_) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Face culling
    if (face_culling_enabled_) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

} // namespace CorePulse