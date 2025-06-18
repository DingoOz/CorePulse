#include "Camera.h"
#include <algorithm>

namespace CorePulse {

Camera::Camera(CameraType type) : type_(type) {
    update_view_matrix();
    update_projection_matrix();
}

void Camera::set_perspective(float fov, float aspect_ratio, float near_plane, float far_plane) {
    fov_ = fov;
    aspect_ratio_ = aspect_ratio;
    near_plane_ = near_plane;
    far_plane_ = far_plane;
    update_projection_matrix();
}

void Camera::set_orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane) {
    ortho_left_ = left;
    ortho_right_ = right;
    ortho_bottom_ = bottom;
    ortho_top_ = top;
    near_plane_ = near_plane;
    far_plane_ = far_plane;
    update_projection_matrix();
}

void Camera::move_forward(float distance) {
    glm::vec3 forward = get_forward();
    position_ += forward * distance;
    target_ += forward * distance;
    update_view_matrix();
}

void Camera::move_backward(float distance) {
    move_forward(-distance);
}

void Camera::move_left(float distance) {
    glm::vec3 right = get_right();
    position_ -= right * distance;
    target_ -= right * distance;
    update_view_matrix();
}

void Camera::move_right(float distance) {
    move_left(-distance);
}

void Camera::move_up(float distance) {
    position_ += up_ * distance;
    target_ += up_ * distance;
    update_view_matrix();
}

void Camera::move_down(float distance) {
    move_up(-distance);
}

void Camera::rotate(float yaw_delta, float pitch_delta) {
    glm::vec3 forward = get_forward();
    glm::vec3 right = get_right();
    
    // Calculate current pitch and yaw
    float current_pitch = asin(-forward.y);
    float current_yaw = atan2(forward.x, forward.z);
    
    // Apply rotation deltas
    current_yaw += glm::radians(yaw_delta);
    current_pitch += glm::radians(pitch_delta);
    
    // Clamp pitch to avoid gimbal lock
    current_pitch = std::clamp(current_pitch, glm::radians(-89.0f), glm::radians(89.0f));
    
    // Calculate new forward vector
    glm::vec3 new_forward;
    new_forward.x = sin(current_yaw) * cos(current_pitch);
    new_forward.y = -sin(current_pitch);
    new_forward.z = cos(current_yaw) * cos(current_pitch);
    new_forward = glm::normalize(new_forward);
    
    // Update target based on new forward direction
    target_ = position_ + new_forward;
    update_view_matrix();
}

void Camera::look_at(const glm::vec3& target) {
    target_ = target;
    update_view_matrix();
}

void Camera::update_view_matrix() {
    view_matrix_ = glm::lookAt(position_, target_, up_);
}

void Camera::update_projection_matrix() {
    if (type_ == CameraType::Perspective) {
        projection_matrix_ = glm::perspective(glm::radians(fov_), aspect_ratio_, near_plane_, far_plane_);
    } else {
        projection_matrix_ = glm::ortho(ortho_left_, ortho_right_, ortho_bottom_, ortho_top_, near_plane_, far_plane_);
    }
}

} // namespace CorePulse