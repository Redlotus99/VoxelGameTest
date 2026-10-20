#pragma once
#include <glm/glm.hpp>
#include "render/Camera.h"

class Window;

class Player {
public:
    Player(Window& window, Camera& camera);
    
    void update(float deltaTime);
    glm::vec3 getPosition() const;
    
private:
    Window& m_window;
    Camera& m_camera;
    glm::vec3 m_position;
};
