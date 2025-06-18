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
    
    // Initialize camera
    camera_ = std::make_shared<Camera>();
    camera_->set_position(glm::vec3(0.0f, 2.0f, 8.0f));
    camera_->set_target(glm::vec3(0.0f, 0.0f, 0.0f));
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
    std::cout << "  SPACE - Test\n";
    std::cout << "  Mouse wheel - Zoom camera\n";
    std::cout << "  Arrow keys - Move camera\n";
    
    return true;
}

void CorePulseApp::on_update(float delta_time) {
    // Update camera rotation
    camera_angle_ += 45.0f * delta_time; // Rotation for testing
    if (camera_angle_ > 360.0f) {
        camera_angle_ -= 360.0f;
    }
    
    update_camera_position();
    
    // Update ECS World
    if (world_) {
        world_->update(delta_time);
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
    
    // Clean up resources
    cube_mesh_.reset();
    sphere_mesh_.reset();
    plane_mesh_.reset();
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
            spawn_random_entity();
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
            if (camera_) {
                camera_->move_forward(0.5f);
            }
            break;
            
        case SDL_SCANCODE_DOWN:
            if (camera_) {
                camera_->move_backward(0.5f);
            }
            break;
            
        case SDL_SCANCODE_LEFT:
            if (camera_) {
                camera_->move_left(0.5f);
            }
            break;
            
        case SDL_SCANCODE_RIGHT:
            if (camera_) {
                camera_->move_right(0.5f);
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
    if (camera_) {
        float zoom_speed = 0.5f;
        if (y > 0) {
            camera_->move_forward(zoom_speed);
        } else if (y < 0) {
            camera_->move_backward(zoom_speed);
        }
    }
    std::cout << "Mouse wheel: (" << x << ", " << y << ")\n";
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
    
    // Orbit camera around the origin
    float radius = 8.0f;
    float x = radius * cos(glm::radians(camera_angle_));
    float z = radius * sin(glm::radians(camera_angle_));
    
    camera_->set_position(glm::vec3(x, 2.0f, z));
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
}

void CorePulseApp::create_demo_entities() {
    if (!world_) return;
    
    // Create a few initial demo entities
    
    // Spinning red cube at origin
    Entity cube_entity = world_->create_entity();
    world_->add_component(cube_entity, Transform{glm::vec3(0.0f, 0.0f, 0.0f)});
    world_->add_component(cube_entity, Renderable{cube_mesh_, glm::vec3(1.0f, 0.5f, 0.5f)});
    world_->add_component(cube_entity, AutoRotate{glm::vec3(0.0f, 1.0f, 0.0f), 45.0f});
    world_->add_component(cube_entity, Tag{"Red Cube"});
    demo_entities_.push_back(cube_entity);
    
    // Manually register with systems for now
    if (render_system_) render_system_->entities.insert(cube_entity);
    if (auto_rotate_system_) auto_rotate_system_->entities.insert(cube_entity);
    
    // Moving green sphere
    Entity sphere_entity = world_->create_entity();
    world_->add_component(sphere_entity, Transform{glm::vec3(3.0f, 0.0f, 0.0f)});
    world_->add_component(sphere_entity, Renderable{sphere_mesh_, glm::vec3(0.5f, 1.0f, 0.5f)});
    world_->add_component(sphere_entity, Velocity{glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 90.0f)});
    world_->add_component(sphere_entity, Tag{"Green Sphere"});
    demo_entities_.push_back(sphere_entity);
    
    // Manually register with systems for now
    if (render_system_) render_system_->entities.insert(sphere_entity);
    if (movement_system_) movement_system_->entities.insert(sphere_entity);
    
    // Static blue plane
    Entity plane_entity = world_->create_entity();
    world_->add_component(plane_entity, Transform{glm::vec3(-3.0f, 0.0f, 0.0f)});
    world_->add_component(plane_entity, Renderable{plane_mesh_, glm::vec3(0.5f, 0.5f, 1.0f)});
    world_->add_component(plane_entity, Tag{"Blue Plane"});
    demo_entities_.push_back(plane_entity);
    
    // Manually register with systems for now
    if (render_system_) render_system_->entities.insert(plane_entity);
    
    std::cout << "Created " << demo_entities_.size() << " demo entities" << std::endl;
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

} // namespace CorePulse