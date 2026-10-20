#include "Player.h"
#include "core/Window.h"
#include <glm/gtc/matrix_transform.hpp>

Player::Player(Window& window, Camera& camera) 
    : m_window(window), m_camera(camera), m_position(0.0f, 50.0f, 0.0f) {
    // Initialize player state
    m_camera.Position = m_position;
}

void Player::update(float deltaTime) {
    // Get mouse delta
    SDL_Point mouseDelta = m_window.getMouseDelta();
    
    // Process mouse input for camera rotation
    if (mouseDelta.x != 0 || mouseDelta.y != 0) {
        m_camera.processMouse(mouseDelta.x, mouseDelta.y);
    }
    
    // Handle keyboard input for movement
    bool forward = m_window.isKeyDown(SDL_SCANCODE_W);
    bool back = m_window.isKeyDown(SDL_SCANCODE_S);
    bool left = m_window.isKeyDown(SDL_SCANCODE_A);
    bool right = m_window.isKeyDown(SDL_SCANCODE_D);
    bool up = m_window.isKeyDown(SDL_SCANCODE_SPACE);
    bool down = m_window.isKeyDown(SDL_SCANCODE_LSHIFT);
    
    // Update camera position based on input
    m_camera.processKeyboard(forward, back, left, right, up, down, deltaTime);
    
    // Update player position to match camera position
    m_position = m_camera.Position;
}

glm::vec3 Player::getPosition() const {
    return m_position;
}
