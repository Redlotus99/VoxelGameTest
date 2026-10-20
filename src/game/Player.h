#pragma once

#include <glm/glm.hpp>

class World;

class Player {
public:
    static constexpr float HALF_WIDTH = 0.3f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float EYE_HEIGHT = 1.62f;
    static constexpr float GRAVITY = 25.0f;
    static constexpr float JUMP_SPEED = 8.0f;
    static constexpr float WALK_SPEED = 5.0f;

    glm::vec3 position{8.5f, 60.0f, 8.5f};  // feet position
    glm::vec3 velocity{0.0f};
    bool onGround = false;

    Player() = default;

    void update(float dt, const World& world, glm::vec3 moveDir, bool jump);
    glm::vec3 getEyePosition() const;

private:
    bool collides(const World& world, const glm::vec3& p) const;
};