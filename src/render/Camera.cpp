#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position) 
    : Position(position), Yaw(YAW), Pitch(PITCH) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
}

void Camera::processKeyboard(bool forward, bool back, bool left, bool right, bool up, bool down, float dt) {
    float velocity = SPEED * dt;
    
    if (forward)
        Position += glm::vec3(0.0f, 0.0f, -1.0f) * velocity;
    if (back)
        Position -= glm::vec3(0.0f, 0.0f, -1.0f) * velocity;
    if (left)
        Position -= glm::vec3(-1.0f, 0.0f, 0.0f) * velocity;
    if (right)
        Position += glm::vec3(-1.0f, 0.0f, 0.0f) * velocity;
    if (up)
        Position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    if (down)
        Position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
}

void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    
    // Normalize the vectors, because their length gets closer to 0 the more you look up or down
    // which results in slower movement.
    glm::vec3 direction = glm::normalize(front);
}
