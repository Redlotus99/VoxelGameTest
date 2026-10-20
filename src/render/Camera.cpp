#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera(glm::vec3 position) 
    : Position(position), Yaw(YAW), Pitch(PITCH) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(FOV), aspect, NEAR_PLANE, FAR_PLANE);
}

void Camera::processMouse(float dx, float dy) {
    Yaw += dx * SENSITIVITY;
    Pitch -= dy * SENSITIVITY; // Note: inverted because of SDL mouse coordinates
    
    // Clamp pitch to avoid gimbal lock
    if (Pitch > PITCH_MAX) Pitch = PITCH_MAX;
    if (Pitch < PITCH_MIN) Pitch = PITCH_MIN;
    
    updateCameraVectors();
    
    // Temporary debug output
    if (dx != 0 || dy != 0) {
        std::cout << "Mouse delta: dx=" << dx << ", dy=" << dy << std::endl;
    }
}

void Camera::processKeyboard(bool forward, bool back, bool left, bool right, bool up, bool down, float dt) {
    float velocity = SPEED * dt;
    
    if (forward)
        Position += Front * velocity;
    if (back)
        Position -= Front * velocity;
    if (left)
        Position -= Right * velocity;
    if (right)
        Position += Right * velocity;
    if (up)
        Position += Up * velocity;
    if (down)
        Position -= Up * velocity;
}

void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    
    Front = glm::normalize(front);
    
    // Also update the Right and Up vector
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));
}
