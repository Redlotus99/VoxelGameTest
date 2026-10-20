#include "game/Player.h"
#include "world/World.h"
#include <cmath>
#include <algorithm>

bool Player::collides(const World& world, const glm::vec3& p) const {
    int x0 = (int)std::floor(p.x - HALF_WIDTH);
    int x1 = (int)std::floor(p.x + HALF_WIDTH - 0.001f);
    int y0 = (int)std::floor(p.y);
    int y1 = (int)std::floor(p.y + HEIGHT - 0.001f);
    int z0 = (int)std::floor(p.z - HALF_WIDTH);
    int z1 = (int)std::floor(p.z + HALF_WIDTH - 0.001f);

    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                if (world.isSolid(x, y, z)) return true;
    return false;
}

void Player::update(float dt, const World& world, glm::vec3 moveDir, bool jump) {
    velocity.x = moveDir.x * WALK_SPEED;
    velocity.z = moveDir.z * WALK_SPEED;

    if (jump && onGround) velocity.y = JUMP_SPEED;
    velocity.y -= GRAVITY * dt;
    velocity.y = std::max(velocity.y, -50.0f);

    // X axis
    float old = position.x;
    position.x += velocity.x * dt;
    if (collides(world, position)) { position.x = old; velocity.x = 0.0f; }

    // Y axis
    old = position.y;
    position.y += velocity.y * dt;
    if (collides(world, position)) {
        onGround = (velocity.y <= 0.0f);
        position.y = old;
        velocity.y = 0.0f;
    } else {
        onGround = false;
    }

    // Z axis
    old = position.z;
    position.z += velocity.z * dt;
    if (collides(world, position)) { position.z = old; velocity.z = 0.0f; }
}

glm::vec3 Player::getEyePosition() const {
    return position + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
}