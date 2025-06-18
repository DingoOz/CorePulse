#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CorePulse {

enum class CameraType {
    Perspective,
    Orthographic
};

class Camera {
public:
    explicit Camera(CameraType type = CameraType::Perspective);
    ~Camera() = default;
    
    // Camera transformation
    void set_position(const glm::vec3& position) { position_ = position; update_view_matrix(); }
    void set_target(const glm::vec3& target) { target_ = target; update_view_matrix(); }
    void set_up(const glm::vec3& up) { up_ = up; update_view_matrix(); }
    
    // Perspective projection parameters
    void set_perspective(float fov, float aspect_ratio, float near_plane, float far_plane);
    void set_orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
    
    // Movement
    void move_forward(float distance);
    void move_backward(float distance);
    void move_left(float distance);
    void move_right(float distance);
    void move_up(float distance);
    void move_down(float distance);
    
    // Rotation
    void rotate(float yaw_delta, float pitch_delta);
    void look_at(const glm::vec3& target);
    
    // Matrix getters
    const glm::mat4& get_view_matrix() const { return view_matrix_; }
    const glm::mat4& get_projection_matrix() const { return projection_matrix_; }
    glm::mat4 get_view_projection_matrix() const { return projection_matrix_ * view_matrix_; }
    
    // Property getters
    const glm::vec3& get_position() const { return position_; }
    const glm::vec3& get_target() const { return target_; }
    const glm::vec3& get_up() const { return up_; }
    glm::vec3 get_forward() const { return glm::normalize(target_ - position_); }
    glm::vec3 get_right() const { return glm::normalize(glm::cross(get_forward(), up_)); }
    
    float get_fov() const { return fov_; }
    float get_aspect_ratio() const { return aspect_ratio_; }
    float get_near_plane() const { return near_plane_; }
    float get_far_plane() const { return far_plane_; }
    
    // Camera type
    CameraType get_type() const { return type_; }
    void set_type(CameraType type) { type_ = type; update_projection_matrix(); }
    
private:
    CameraType type_;
    
    // View parameters
    glm::vec3 position_{0.0f, 0.0f, 3.0f};
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
    
    // Projection parameters
    float fov_ = 45.0f;
    float aspect_ratio_ = 16.0f / 9.0f;
    float near_plane_ = 0.1f;
    float far_plane_ = 100.0f;
    
    // Orthographic parameters
    float ortho_left_ = -1.0f;
    float ortho_right_ = 1.0f;
    float ortho_bottom_ = -1.0f;
    float ortho_top_ = 1.0f;
    
    // Matrices
    glm::mat4 view_matrix_{1.0f};
    glm::mat4 projection_matrix_{1.0f};
    
    void update_view_matrix();
    void update_projection_matrix();
};

} // namespace CorePulse