#include "CorePulseApp.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>

namespace CorePulse {

CorePulseApp::CorePulseApp() : Application([](){
    ApplicationConfig config;
    config.window_config.title = "CorePulse - Phase 1 Demo";
    config.window_config.width = 1024;
    config.window_config.height = 768;
    config.target_fps = 60;
    return config;
}()) {
}

bool CorePulseApp::on_initialize() {
    std::cout << "CorePulseApp: ECS Demo Initialization\n";
    
    // Initialize renderer
    renderer_ = std::make_shared<Renderer>();
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }
    
    // Initialize audio system
    audio_manager_ = std::make_shared<AudioManager>();
    if (!audio_manager_->initialize()) {
        std::cerr << "Failed to initialize audio system\n";
        return false;
    }
    
    // Load audio files
    std::cout << "Loading audio assets...\n";
    audio_manager_->load_audio_clip("assets/audio/bounce.wav", "bounce");
    audio_manager_->load_audio_clip("assets/audio/collision_metal.wav", "collision_metal");
    audio_manager_->load_audio_clip("assets/audio/collision_soft.wav", "collision_soft");
    audio_manager_->load_audio_clip("assets/audio/ambient_hum.wav", "ambient_hum");
    audio_manager_->load_audio_clip("assets/audio/ambient_wind.wav", "ambient_wind");
    
    // Initialize camera
    camera_ = std::make_shared<Camera>();
    camera_->set_position(glm::vec3(0.0f, 4.0f, 12.0f)); // Higher and further back to see both demos
    camera_->set_target(glm::vec3(0.0f, 2.0f, 0.0f)); // Look at a point between the ground and helmet
    camera_->set_perspective(45.0f, get_window().get_aspect_ratio(), 0.1f, 100.0f);
    
    // Create test meshes - make cube larger for better visibility
    cube_mesh_ = std::make_shared<Mesh>(Mesh::create_cube(2.0f));
    sphere_mesh_ = std::make_shared<Mesh>(Mesh::create_sphere(1.0f, 32));
    plane_mesh_ = std::make_shared<Mesh>(Mesh::create_plane(2.0f, 2.0f));
    
    std::cout << "Cube mesh valid: " << (cube_mesh_ && cube_mesh_->is_valid()) << std::endl;
    std::cout << "Cube mesh vertex count: " << (cube_mesh_ ? cube_mesh_->get_vertex_count() : 0) << std::endl;
    std::cout << "Cube mesh index count: " << (cube_mesh_ ? cube_mesh_->get_index_count() : 0) << std::endl;
    
    // Initialize ECS World
    std::cout << "Initializing ECS World...\n";
    try {
        world_ = std::make_unique<World>();
        world_->init();
        std::cout << "ECS World initialized successfully\n";
        
        // Set up ECS systems
        std::cout << "Setting up ECS systems...\n";
        setup_ecs_systems();
        
        // Test glTF loader
        std::cout << "Testing glTF loader...\n";
        test_gltf_loader();
        
        // Create demo entities
        std::cout << "Creating demo entities...\n";
        create_demo_entities();
        std::cout << "Entity count: " << world_->get_entity_count() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "ECS operation failed: " << e.what() << std::endl;
        world_.reset();
    }
    
    std::cout << "Controls:\n";
    std::cout << "  ESC - Quit\n";
    std::cout << "  F11 - Toggle Fullscreen\n";
    std::cout << "  I   - Toggle Info Display\n";
    std::cout << "  W   - Toggle Wireframe Mode\n";
    std::cout << "  SPACE - Drop sphere (reset physics demo)\n";
    std::cout << "  Mouse wheel - Zoom in/out (radius: 2-20)\n";
    std::cout << "  Arrow keys - Rotate camera around scene\n";
    std::cout << "  A/D - Rotate camera left/right manually\n";
    std::cout << "  Q/E - Move camera up/down\n";
    std::cout << "  P - Pause/resume auto camera rotation\n";
    std::cout << "  H - Move FlightHelmet to origin for debugging\n";
    std::cout << "  J - Toggle wireframe mode (debug)\n";
    std::cout << "  K - Move FlightHelmet in front of camera\n";
    std::cout << "\nDemo features:\n";
    std::cout << "  - Physics sphere (red, bouncing on blue platform)\n";
    std::cout << "  - FlightHelmet glTF model (right side, PBR materials)\n";
    std::cout << "  - 3D spatial audio with collision sounds\n";
    
    return true;
}

void CorePulseApp::on_update(float delta_time) {
    // Update camera rotation (only if auto rotation is enabled)
    if (auto_rotate_camera_) {
        camera_angle_ += 45.0f * delta_time; // Rotation for testing
        if (camera_angle_ > 360.0f) {
            camera_angle_ -= 360.0f;
        }
    }
    
    update_camera_position();
    
    // Update ECS World
    if (world_) {
        world_->update(delta_time);
    }
    
    // Manually update physics system since it's not registered with SystemManager yet
    if (physics_system_) {
        physics_system_->update(delta_time);
    }
    
    // Update audio system
    if (audio_system_) {
        // Update listener position to follow camera
        if (camera_) {
            glm::vec3 cam_pos = camera_->get_position();
            glm::vec3 cam_forward = glm::normalize(camera_->get_target() - cam_pos);
            glm::vec3 cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
            audio_system_->set_listener_to_camera(cam_pos, cam_forward, cam_up);
        }
        
        audio_system_->update(delta_time);
    }
    
    // Update window title with FPS info
    if (show_info_) {
        update_window_title();
    }
}

void CorePulseApp::on_render() {
    static int render_count = 0;
    render_count++;
    
    // Simple direct rendering for testing
    if (!renderer_ || !camera_) {
        std::cout << "Renderer or camera null in on_render!" << std::endl;
        return;
    }
    
    if (render_count % 60 == 1) {  // Print every 60 frames (once per second at 60fps)
        std::cout << "Rendering frame " << render_count << ", angle: " << camera_angle_ << std::endl;
        std::cout << "Camera position: (" << camera_->get_position().x << ", " << camera_->get_position().y << ", " << camera_->get_position().z << ")" << std::endl;
        std::cout << "Camera radius: " << camera_radius_ << std::endl;
        if (world_) {
            std::cout << "ECS entities: " << world_->get_entity_count() << std::endl;
        }
    }
    
    renderer_->begin_frame();
    renderer_->clear(glm::vec4(0.2f, 0.2f, 0.3f, 1.0f)); // Dark blue-gray background
    
    // Use ECS RenderSystem if available
    if (render_system_) {
        render_system_->update(0.0f); // RenderSystem doesn't use delta_time
        if (render_count % 60 == 1) {
            std::cout << "ECS rendering complete" << std::endl;
        }
    } else {
        // Fallback: manual cube rendering
        if (cube_mesh_ && cube_mesh_->is_valid()) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(camera_angle_), glm::vec3(0.0f, 1.0f, 0.0f));
            
            auto& shader = renderer_->get_default_shader();
            shader.use();
            shader.set_vec3("u_color", glm::vec3(1.0f, 0.3f, 0.3f)); // Bright red for visibility
            shader.unuse();
            
            renderer_->render_mesh(*cube_mesh_, model, *camera_);
            
            if (render_count % 60 == 1) {
                std::cout << "Manual cube rendered" << std::endl;
            }
        }
    }
    
    // Render a debug cube at the same position as the helmet to verify positioning
    if (!gltf_entities_.empty() && world_ && world_->is_valid_entity(gltf_entities_[0])) {
        const auto& transform = world_->get_component<Transform>(gltf_entities_[0]);
        
        glm::mat4 debug_model = glm::mat4(1.0f);
        debug_model = glm::translate(debug_model, transform.position + glm::vec3(2.0f, 0.0f, 0.0f)); // Offset to the right
        debug_model = glm::scale(debug_model, glm::vec3(1.0f)); // Normal size cube
        
        auto& shader = renderer_->get_default_shader();
        shader.use();
        shader.set_vec3("u_color", glm::vec3(1.0f, 1.0f, 0.0f)); // Bright yellow for visibility
        shader.unuse();
        
        renderer_->render_mesh(*cube_mesh_, debug_model, *camera_);
        
        if (render_count % 60 == 1) {
            std::cout << "Debug yellow cube at (" 
                     << (transform.position.x + 2.0f) << ", " << transform.position.y << ", " << transform.position.z 
                     << ")" << std::endl;
        }
    }
    
    // Render FlightHelmet glTF entities with PBR materials
    if (!gltf_meshes_.empty() && !gltf_materials_.empty() && !gltf_entities_.empty() && world_) {
        static int helmet_rendered_count = 0;
        
        if (render_count % 60 == 1) {
            std::cout << "Starting FlightHelmet PBR rendering for " << gltf_meshes_.size() << " parts" << std::endl;
        }
        
        for (size_t i = 0; i < gltf_meshes_.size(); ++i) {
            if (i < gltf_entities_.size() && world_->is_valid_entity(gltf_entities_[i])) {
                const auto& transform = world_->get_component<Transform>(gltf_entities_[i]);
                
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, transform.position);
                model = glm::rotate(model, glm::radians(camera_angle_ * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f)); // Slow rotation to show off the model
                model = glm::scale(model, glm::vec3(5.0f)); // Scale up the helmet even more to make it clearly visible
                
                // Debug shader validity and try PBR, fallback to default if needed
                auto& pbr_shader = renderer_->get_pbr_shader();
                bool use_pbr = pbr_shader.is_valid();
                
                if (render_count % 60 == 1) {
                    std::cout << "  Rendering part " << i << " - PBR shader valid: " << use_pbr 
                             << ", material: '" << (i < gltf_materials_.size() ? gltf_materials_[i]->name : "default") << "'" << std::endl;
                }
                
                // Force fallback to default shader for Intel GPU compatibility testing
                use_pbr = false;
                
                if (use_pbr) {
                    // Set up lighting uniforms for PBR shader
                    pbr_shader.use();
                    pbr_shader.set_vec3("u_light_pos", glm::vec3(5.0f, 5.0f, 5.0f));
                    pbr_shader.set_vec3("u_light_color", glm::vec3(1.0f, 1.0f, 1.0f));
                    pbr_shader.unuse();
                    
                    // Use the material that corresponds to this mesh
                    if (i < gltf_materials_.size()) {
                        renderer_->render_mesh(*gltf_meshes_[i], model, *camera_, *gltf_materials_[i]);
                    } else {
                        auto default_material = Material::create_default();
                        renderer_->render_mesh(*gltf_meshes_[i], model, *camera_, *default_material);
                    }
                } else {
                    // Fallback to default shader with lighting
                    auto& shader = renderer_->get_default_shader();
                    shader.use();
                    shader.set_vec3("u_light_pos", glm::vec3(5.0f, 5.0f, 5.0f));
                    shader.set_vec3("u_light_color", glm::vec3(1.0f, 1.0f, 1.0f));
                    // Use material base color for fallback, with variety if all white
                    if (i < gltf_materials_.size()) {
                        glm::vec3 color = glm::vec3(gltf_materials_[i]->base_color_factor);
                        
                        // If material color is too close to white, add some variety
                        if (color.r > 0.9f && color.g > 0.9f && color.b > 0.9f) {
                            // Use different colors for different helmet parts
                            glm::vec3 part_colors[] = {
                                glm::vec3(0.8f, 0.6f, 0.4f),  // Brown leather
                                glm::vec3(0.7f, 0.7f, 0.8f),  // Blue-grey metal
                                glm::vec3(0.6f, 0.8f, 0.6f),  // Green tint
                                glm::vec3(0.8f, 0.7f, 0.6f),  // Tan
                                glm::vec3(0.5f, 0.5f, 0.7f),  // Purple tint
                                glm::vec3(0.8f, 0.8f, 0.6f)   // Yellow tint
                            };
                            color = part_colors[i % 6];
                            
                            if (render_count % 60 == 1) {
                                std::cout << "    Using variety color for white material " << i 
                                         << ": (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                            }
                        } else {
                            if (render_count % 60 == 1) {
                                std::cout << "    Using material color " << i 
                                         << ": (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
                            }
                        }
                        
                        shader.set_vec3("u_color", color);
                    } else {
                        shader.set_vec3("u_color", glm::vec3(0.8f, 0.8f, 0.8f));
                    }
                    shader.unuse();
                    
                    renderer_->render_mesh(*gltf_meshes_[i], model, *camera_);
                }
                helmet_rendered_count++;
            }
        }
        
        if (render_count % 60 == 1 && !gltf_meshes_.empty()) {
            const auto& first_transform = world_->get_component<Transform>(gltf_entities_[0]);
            std::cout << "FlightHelmet rendered with " << gltf_meshes_.size() << " parts at position (" 
                     << first_transform.position.x << ", " << first_transform.position.y << ", " << first_transform.position.z 
                     << ") using PBR materials, scale 5x" << std::endl;
        }
    }
    
    renderer_->end_frame();
}

void CorePulseApp::on_shutdown() {
    std::cout << "CorePulseApp: Shutting down\n";
    
    // Clean up ECS
    if (world_) {
        world_->shutdown();
        world_.reset();
    }
    
    // Clean up systems
    render_system_.reset();
    movement_system_.reset();
    auto_rotate_system_.reset();
    lifetime_system_.reset();
    physics_system_.reset();
    audio_system_.reset();
    
    // Clean up audio
    if (audio_manager_) {
        audio_manager_->shutdown();
        audio_manager_.reset();
    }
    
    // Clean up resources
    cube_mesh_.reset();
    sphere_mesh_.reset();
    plane_mesh_.reset();
    gltf_meshes_.clear();
    gltf_materials_.clear();
    gltf_entities_.clear();
    camera_.reset();
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
}

void CorePulseApp::on_key_pressed(SDL_Scancode key) {
    switch (key) {
        case SDL_SCANCODE_I:
            show_info_ = !show_info_;
            if (!show_info_) {
                get_window().set_title("CorePulse - OpenGL Renderer Demo");
            }
            std::cout << "Info display: " << (show_info_ ? "ON" : "OFF") << std::endl;
            break;
            
        case SDL_SCANCODE_W:
            wireframe_mode_ = !wireframe_mode_;
            if (renderer_) {
                renderer_->set_wireframe_mode(wireframe_mode_);
            }
            std::cout << "Wireframe mode: " << (wireframe_mode_ ? "ON" : "OFF") << std::endl;
            break;
            
        case SDL_SCANCODE_SPACE:
            trigger_sphere_drop();
            break;
            
        case SDL_SCANCODE_C:
            // Clear all demo entities
            for (Entity entity : demo_entities_) {
                if (world_->is_valid_entity(entity)) {
                    world_->destroy_entity(entity);
                }
            }
            demo_entities_.clear();
            std::cout << "Cleared all entities\n";
            break;
            
        case SDL_SCANCODE_UP:
            // Rotate camera up (decrease angle)
            camera_angle_ -= 10.0f;
            if (camera_angle_ < 0.0f) camera_angle_ += 360.0f;
            std::cout << "Camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_DOWN:
            // Rotate camera down (increase angle)
            camera_angle_ += 10.0f;
            if (camera_angle_ >= 360.0f) camera_angle_ -= 360.0f;
            std::cout << "Camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_LEFT:
            // Rotate camera left
            camera_angle_ -= 10.0f;
            if (camera_angle_ < 0.0f) camera_angle_ += 360.0f;
            std::cout << "Camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_RIGHT:
            // Rotate camera right
            camera_angle_ += 10.0f;
            if (camera_angle_ >= 360.0f) camera_angle_ -= 360.0f;
            std::cout << "Camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_A:
            // Manual rotate left
            camera_angle_ -= 5.0f;
            if (camera_angle_ < 0.0f) camera_angle_ += 360.0f;
            auto_rotate_camera_ = false; // Stop auto rotation when manually controlling
            std::cout << "Manual camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_D:
            // Manual rotate right
            camera_angle_ += 5.0f;
            if (camera_angle_ >= 360.0f) camera_angle_ -= 360.0f;
            auto_rotate_camera_ = false; // Stop auto rotation when manually controlling
            std::cout << "Manual camera angle: " << camera_angle_ << std::endl;
            break;
            
        case SDL_SCANCODE_Q:
            // Move camera up
            camera_height_ += 0.5f;
            camera_height_ = std::min(camera_height_, 15.0f); // Max height
            std::cout << "Camera height: " << camera_height_ << std::endl;
            break;
            
        case SDL_SCANCODE_E:
            // Move camera down
            camera_height_ -= 0.5f;
            camera_height_ = std::max(camera_height_, 0.5f); // Min height
            std::cout << "Camera height: " << camera_height_ << std::endl;
            break;
            
        case SDL_SCANCODE_P:
            // Toggle auto rotation
            auto_rotate_camera_ = !auto_rotate_camera_;
            std::cout << "Auto camera rotation: " << (auto_rotate_camera_ ? "ON" : "OFF") << std::endl;
            break;
            
        case SDL_SCANCODE_H:
            // Move helmet to origin for better visibility
            if (!gltf_entities_.empty() && world_) {
                for (auto entity : gltf_entities_) {
                    if (world_->is_valid_entity(entity)) {
                        auto& transform = world_->get_component<Transform>(entity);
                        transform.position = glm::vec3(0.0f, 0.0f, 0.0f); // Move to origin
                    }
                }
                std::cout << "Moved FlightHelmet to origin (0,0,0)" << std::endl;
            }
            break;
            
        case SDL_SCANCODE_J:
            // Toggle wireframe mode to see if model is there
            wireframe_mode_ = !wireframe_mode_;
            if (renderer_) {
                renderer_->set_wireframe_mode(wireframe_mode_);
            }
            std::cout << "Wireframe mode: " << (wireframe_mode_ ? "ON" : "OFF") << " (same as W key)" << std::endl;
            break;
            
        case SDL_SCANCODE_K:
            // Move helmet to a very visible position in front of camera
            if (!gltf_entities_.empty() && world_) {
                for (auto entity : gltf_entities_) {
                    if (world_->is_valid_entity(entity)) {
                        auto& transform = world_->get_component<Transform>(entity);
                        transform.position = glm::vec3(0.0f, 0.0f, -5.0f); // In front of camera
                    }
                }
                std::cout << "Moved FlightHelmet to (0,0,-5) - should be directly in front of camera" << std::endl;
            }
            break;
            
        default:
            std::cout << "Key pressed: " << SDL_GetScancodeName(key) << std::endl;
            break;
    }
}

void CorePulseApp::on_key_released(SDL_Scancode key) {
    // std::cout << "Key released: " << SDL_GetScancodeName(key) << std::endl;
}

void CorePulseApp::on_mouse_button_pressed(MouseButton button) {
    const char* button_name = "Unknown";
    switch (button) {
        case MouseButton::Left: button_name = "Left"; break;
        case MouseButton::Middle: button_name = "Middle"; break;
        case MouseButton::Right: button_name = "Right"; break;
    }
    std::cout << "Mouse button pressed: " << button_name << std::endl;
}

void CorePulseApp::on_mouse_button_released(MouseButton button) {
    // Optional: handle mouse button release
}

void CorePulseApp::on_mouse_moved(int x, int y, int dx, int dy) {
    // Only log significant mouse movements to avoid spam
    if (abs(dx) > 5 || abs(dy) > 5) {
        // std::cout << "Mouse moved: (" << x << ", " << y << ") delta: (" << dx << ", " << dy << ")\n";
    }
}

void CorePulseApp::on_mouse_wheel(int x, int y) {
    float zoom_speed = 1.0f;
    float min_radius = 2.0f;
    float max_radius = 20.0f;
    
    if (y > 0) {
        // Zoom in (decrease radius)
        camera_radius_ -= zoom_speed;
    } else if (y < 0) {
        // Zoom out (increase radius)
        camera_radius_ += zoom_speed;
    }
    
    // Clamp radius to reasonable bounds
    camera_radius_ = std::max(min_radius, std::min(max_radius, camera_radius_));
    
    std::cout << "Mouse wheel zoom: radius = " << camera_radius_ << std::endl;
}

void CorePulseApp::on_window_resized(int width, int height) {
    if (camera_) {
        float aspect_ratio = static_cast<float>(width) / height;
        camera_->set_perspective(camera_->get_fov(), aspect_ratio, 
                                camera_->get_near_plane(), camera_->get_far_plane());
    }
    std::cout << "Window resized: " << width << "x" << height << std::endl;
}

void CorePulseApp::update_window_title() {
    std::ostringstream title;
    title << "CorePulse - Debug Mode";
    
    title << " | FPS: " << std::fixed << std::setprecision(1) << get_fps()
          << " | Delta: " << std::fixed << std::setprecision(3) << get_delta_time() * 1000.0f << "ms";
    
    if (renderer_) {
        const auto& stats = renderer_->get_stats();
        title << " | Draw calls: " << stats.draw_calls 
              << " | Triangles: " << stats.triangles_rendered;
    }
    
    get_window().set_title(title.str());
}

void CorePulseApp::update_camera_position() {
    if (!camera_) return;
    
    // Orbit camera around the origin with adjustable radius
    float x = camera_radius_ * cos(glm::radians(camera_angle_));
    float z = camera_radius_ * sin(glm::radians(camera_angle_));
    
    // Use manual camera height if not auto-rotating, otherwise use proportional height
    float final_height = auto_rotate_camera_ ? 
        std::max(2.0f, camera_radius_ * 0.3f) : camera_height_;
    
    camera_->set_position(glm::vec3(x, final_height, z));
    camera_->look_at(glm::vec3(0.0f, 0.0f, 0.0f));
}

void CorePulseApp::setup_ecs_systems() {
    if (!world_) return;
    
    // Set up RenderSystem with proper registration
    auto& system_manager = world_->get_system_manager();
    
    // Register RenderSystem manually since it needs constructor parameters
    render_system_ = std::make_shared<RenderSystem>(renderer_, camera_);
    render_system_->set_world(world_.get());
    render_system_->init();
    
    // Calculate system signatures
    Signature render_signature;
    render_signature |= (1 << world_->get_component_type<Transform>());
    render_signature |= (1 << world_->get_component_type<Renderable>());
    
    Signature movement_signature;
    movement_signature |= (1 << world_->get_component_type<Transform>());
    movement_signature |= (1 << world_->get_component_type<Velocity>());
    
    Signature auto_rotate_signature;
    auto_rotate_signature |= (1 << world_->get_component_type<Transform>());
    auto_rotate_signature |= (1 << world_->get_component_type<AutoRotate>());
    
    Signature lifetime_signature;
    lifetime_signature |= (1 << world_->get_component_type<Lifetime>());
    
    // Set up systems with proper signatures (simplified for now)
    movement_system_ = std::make_shared<MovementSystem>();
    movement_system_->set_world(world_.get());
    movement_system_->init();
    
    auto_rotate_system_ = std::make_shared<AutoRotateSystem>();
    auto_rotate_system_->set_world(world_.get());
    auto_rotate_system_->init();
    
    lifetime_system_ = std::make_shared<LifetimeSystem>();
    lifetime_system_->set_world(world_.get());
    lifetime_system_->init();
    
    physics_system_ = std::make_shared<PhysicsSystem>();
    physics_system_->set_world(world_.get());
    physics_system_->init();
    
    audio_system_ = std::make_shared<AudioSystem>(audio_manager_);
    audio_system_->set_world(world_.get());
    audio_system_->init();
    
    // Connect physics system to audio system for collision sounds
    physics_system_->set_audio_system(audio_system_.get());
}

void CorePulseApp::create_demo_entities() {
    if (!world_) return;
    
    // Create physics demo entities - simple sphere falling onto a plane
    
    // Red sphere with physics - will fall and bounce on the plane
    sphere_entity_ = world_->create_entity();
    world_->add_component(sphere_entity_, Transform{glm::vec3(0.0f, 6.0f, 0.0f)});  // Start high above the plane
    world_->add_component(sphere_entity_, Renderable{sphere_mesh_, glm::vec3(1.0f, 0.3f, 0.3f)});  // Bright red
    world_->add_component(sphere_entity_, RigidBody{glm::vec3(0.0f), glm::vec3(0.0f), 1.0f, 0.1f, 0.1f, false, true});  // No initial velocity
    world_->add_component(sphere_entity_, Collider{Collider::Type::Sphere, glm::vec3(1.0f), glm::vec3(0.0f), false});
    
    // Add audio source component for collision sounds
    AudioSourceComponent bounce_audio;
    bounce_audio.clip_name = "bounce";
    bounce_audio.volume = 0.8f;
    bounce_audio.is_3d = true;
    bounce_audio.play_on_collision = true;
    bounce_audio.max_distance = 20.0f;
    world_->add_component(sphere_entity_, bounce_audio);
    
    world_->add_component(sphere_entity_, Tag{"Falling Sphere"});
    demo_entities_.push_back(sphere_entity_);
    
    // Manually register with systems for now
    if (render_system_) render_system_->entities.insert(sphere_entity_);
    if (physics_system_) physics_system_->entities.insert(sphere_entity_);
    if (audio_system_) audio_system_->entities.insert(sphere_entity_);
    
    // Large static blue plane (kinematic, won't be affected by physics)
    Entity plane_entity = world_->create_entity();
    world_->add_component(plane_entity, Transform{glm::vec3(0.0f, 0.5f, 0.0f)});  // Position at y=0.5 to be above ground
    world_->add_component(plane_entity, Renderable{plane_mesh_, glm::vec3(0.3f, 0.3f, 1.0f)});  // Bright blue
    world_->add_component(plane_entity, RigidBody{glm::vec3(0.0f), glm::vec3(0.0f), 1.0f, 0.1f, 0.1f, true, false});  // Kinematic
    world_->add_component(plane_entity, Collider{Collider::Type::Box, glm::vec3(4.0f, 0.2f, 4.0f), glm::vec3(0.0f), false});  // Large flat platform
    world_->add_component(plane_entity, Tag{"Landing Platform"});
    demo_entities_.push_back(plane_entity);
    
    // Manually register with systems for now
    if (render_system_) render_system_->entities.insert(plane_entity);
    if (physics_system_) physics_system_->entities.insert(plane_entity);
    
    // Add ambient audio entity
    Entity ambient_entity = world_->create_entity();
    world_->add_component(ambient_entity, Transform{glm::vec3(0.0f, 0.0f, 0.0f)});
    
    AmbientAudioComponent ambient_audio;
    ambient_audio.clip_name = "ambient_hum";
    ambient_audio.volume = 0.2f;
    ambient_audio.auto_start = true;
    ambient_audio.fade_distance = 30.0f;
    ambient_audio.max_distance = 50.0f;
    world_->add_component(ambient_entity, ambient_audio);
    
    world_->add_component(ambient_entity, Tag{"Ambient Audio"});
    demo_entities_.push_back(ambient_entity);
    
    // Register ambient entity with audio system
    if (audio_system_) audio_system_->entities.insert(ambient_entity);
    
    std::cout << "Created " << demo_entities_.size() << " demo entities (including ambient audio)" << std::endl;
    std::cout << "Watch the red sphere fall and bounce on the blue platform!" << std::endl;
    std::cout << "Listen for velocity-based collision sounds and ambient background audio!" << std::endl;
}

void CorePulseApp::spawn_random_entity() {
    if (!world_) return;
    
    Entity entity = world_->create_entity();
    
    // Random position
    float x = (rand() % 21 - 10) * 0.5f; // -5 to 5
    float y = (rand() % 11) * 0.5f;      // 0 to 5
    float z = (rand() % 21 - 10) * 0.5f; // -5 to 5
    
    // Random mesh and color
    std::shared_ptr<Mesh> mesh;
    glm::vec3 color;
    int mesh_type = rand() % 3;
    
    switch (mesh_type) {
        case 0:
            mesh = cube_mesh_;
            color = glm::vec3((rand() % 256) / 255.0f, 0.5f, 0.5f);
            break;
        case 1:
            mesh = sphere_mesh_;
            color = glm::vec3(0.5f, (rand() % 256) / 255.0f, 0.5f);
            break;
        case 2:
            mesh = plane_mesh_;
            color = glm::vec3(0.5f, 0.5f, (rand() % 256) / 255.0f);
            break;
    }
    
    world_->add_component(entity, Transform{glm::vec3(x, y, z)});
    world_->add_component(entity, Renderable{mesh, color});
    world_->add_component(entity, AutoRotate{glm::vec3(0.0f, 1.0f, 0.0f), (rand() % 91 + 10)}); // 10-100 deg/sec
    world_->add_component(entity, Lifetime{5.0f + (rand() % 51) * 0.1f}); // 5-10 seconds
    
    demo_entities_.push_back(entity);
    std::cout << "Spawned entity at (" << x << ", " << y << ", " << z << ")\n";
}

void CorePulseApp::trigger_sphere_drop() {
    if (!world_ || sphere_entity_ == 0) return;
    
    // Check if the sphere entity is still valid
    if (!world_->is_valid_entity(sphere_entity_)) {
        std::cout << "Sphere entity no longer valid, cannot reset drop" << std::endl;
        return;
    }
    
    // Reset sphere position to high above the platform
    auto& transform = world_->get_component<Transform>(sphere_entity_);
    transform.position = glm::vec3(0.0f, 8.0f, 0.0f); // Start even higher for dramatic effect
    
    // Reset sphere velocity to zero
    auto& rigidbody = world_->get_component<RigidBody>(sphere_entity_);
    rigidbody.velocity = glm::vec3(0.0f);
    rigidbody.angular_velocity = glm::vec3(0.0f);
    
    std::cout << "SPHERE DROP TRIGGERED! Watch the red sphere fall!" << std::endl;
}

void CorePulseApp::test_gltf_loader() {
    GLTFLoader loader;
    
    // Try to load the FlightHelmet model
    std::string gltf_path = "../assets/models/equipment/FlightHelmet/glTF/FlightHelmet.gltf";
    if (loader.load_gltf(gltf_path)) {
        std::cout << "Successfully loaded FlightHelmet.gltf!" << std::endl;
        
        // Extract meshes and materials
        auto meshes = loader.extract_meshes();
        auto materials = loader.extract_materials();
        std::cout << "Extracted " << meshes.size() << " meshes and " << materials.size() << " materials from glTF file" << std::endl;
        
        // Create glTF entities for each mesh in the FlightHelmet model
        if (!meshes.empty() && world_) {
            std::cout << "Creating " << meshes.size() << " FlightHelmet mesh entities..." << std::endl;
            
            // Clear any existing glTF data
            gltf_meshes_.clear();
            gltf_materials_.clear();
            gltf_entities_.clear();
            
            // Create entities for each mesh
            for (size_t i = 0; i < meshes.size(); ++i) {
                Entity gltf_entity = world_->create_entity();
                
                // Position all parts at the same location in front of the camera
                world_->add_component(gltf_entity, Transform{glm::vec3(0.0f, 4.0f, 0.0f)});
                
                // Use actual materials from GLTF with textures
                std::shared_ptr<Material> material;
                if (i < materials.size()) {
                    material = materials[i];
                    std::cout << "Using GLTF material " << i << " '" << material->name << "'" << std::endl;
                } else {
                    // Fallback to default material if not enough materials
                    material = Material::create_default();
                    material->name = "Default Material " + std::to_string(i);
                    std::cout << "Using default material for mesh " << i << std::endl;
                }
                
                // Add component to mark this as a FlightHelmet part
                world_->add_component(gltf_entity, Tag{"FlightHelmet_Part_" + std::to_string(i)});
                
                // Store the mesh, material, and entity
                gltf_meshes_.push_back(meshes[i]);
                gltf_materials_.push_back(material);
                gltf_entities_.push_back(gltf_entity);
                
                demo_entities_.push_back(gltf_entity);
                
                std::cout << "Created FlightHelmet part " << i << " with " 
                         << meshes[i]->get_vertex_count() << " vertices and "
                         << meshes[i]->get_index_count() << " indices" << std::endl;
            }
            
            std::cout << "FlightHelmet model loaded successfully with " << gltf_meshes_.size() << " parts!" << std::endl;
        }
    } else {
        std::cout << "Failed to load FlightHelmet.gltf: " << loader.get_error() << std::endl;
    }
}

} // namespace CorePulse