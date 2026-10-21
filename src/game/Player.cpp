#include "game/Player.h"
#include "world/World.h"
#include <cmath>
#include <algorithm>
#include <array>

namespace {
float shapeTopAt(BlockShape shape, float x, float z) {
    const auto h = getBlockShapeData(shape).topHeights;

    // The rendered profile is triangulated along the southwest-to-northeast
    // diagonal, so collision uses the same piecewise-linear surface.
    if (z <= x) {
        return h[0] * (1.0f - x) + h[1] * (x - z) + h[2] * z;
    }
    return h[0] * (1.0f - z) + h[2] * x + h[3] * (z - x);
}

bool pointInsideExplicitShape(BlockShape shape, float x, float y, float z) {
    constexpr float eps = 0.0001f;
    switch (shape) {
        case BlockShape::CornerCutSouthWest:
            // This shape removes the bottom southwest tetrahedron.
            return x + y + z >= 1.0f - eps;
        case BlockShape::CornerCutTopSouthWest:
            return x + z + (1.0f - y) >= 1.0f - eps;
        case BlockShape::CornerCutTopNorthEast:
            return (1.0f - x) + (1.0f - z) + (1.0f - y) >= 1.0f - eps;
        case BlockShape::CornerCutTopNorthWest:
            return x + (1.0f - z) + (1.0f - y) >= 1.0f - eps;
        case BlockShape::CornerCutTopSouthEast:
            return (1.0f - x) + z + (1.0f - y) >= 1.0f - eps;
        default:
            return y <= shapeTopAt(shape, x, z) + eps;
    }
}
}

bool Player::collides(const World& world, const glm::vec3& p) const {
    const int x0 = static_cast<int>(std::floor(p.x - HALF_WIDTH));
    const int x1 = static_cast<int>(std::floor(p.x + HALF_WIDTH - 0.001f));
    const int y0 = static_cast<int>(std::floor(p.y));
    const int y1 = static_cast<int>(std::floor(p.y + HEIGHT - 0.001f));
    const int z0 = static_cast<int>(std::floor(p.z - HALF_WIDTH));
    const int z1 = static_cast<int>(std::floor(p.z + HALF_WIDTH - 0.001f));

    // Sample the footprint at corners and center. This avoids treating the
    // empty portion above a ramp/wedge as solid while retaining a stable
    // player-sized collision footprint.
    const std::array<glm::vec2, 5> samples{{
        {p.x, p.z},
        {p.x - HALF_WIDTH, p.z - HALF_WIDTH},
        {p.x + HALF_WIDTH, p.z - HALF_WIDTH},
        {p.x - HALF_WIDTH, p.z + HALF_WIDTH},
        {p.x + HALF_WIDTH, p.z + HALF_WIDTH}
    }};

    for (int x = x0; x <= x1; ++x) {
        for (int y = y0; y <= y1; ++y) {
            for (int z = z0; z <= z1; ++z) {
                if (!world.isSolid(x, y, z)) continue;
                const BlockShape shape = world.getShapeAt(x, y, z);
                for (const auto& sample : samples) {
                    const int sx = static_cast<int>(std::floor(sample.x));
                    const int sz = static_cast<int>(std::floor(sample.y));
                    if (sx != x || sz != z) continue;
                    const float localX = sample.x - x;
                    const float localZ = sample.y - z;
                    const float bottom = std::max(p.y, static_cast<float>(y)) - y;
                    const float top = std::min(p.y + HEIGHT, static_cast<float>(y + 1)) - y;
                    if (top <= 0.0f || bottom >= 1.0f) continue;
                    if (pointInsideExplicitShape(shape, localX, bottom, localZ) ||
                        pointInsideExplicitShape(shape, localX, std::min(top, 0.9999f), localZ)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Player::update(float dt, const World& world, glm::vec3 moveDir, bool jump) {
    velocity.x = moveDir.x * WALK_SPEED;
    velocity.z = moveDir.z * WALK_SPEED;

    if (jump && onGround) velocity.y = JUMP_SPEED;
    velocity.y -= GRAVITY * dt;
    velocity.y = std::max(velocity.y, -50.0f);

    // Horizontal movement first. If a shaped surface rises beneath the
    // player, attempt a small ground-following lift instead of hitting the
    // shape's old full-cube collision boundary.
    const auto tryHorizontalMove = [&](float delta, bool xAxis) {
        if (delta == 0.0f) return;
        const glm::vec3 oldPosition = position;
        if (xAxis) position.x += delta;
        else position.z += delta;

        if (!collides(world, position)) return;

        const glm::vec3 blockedPosition = position;
        position = oldPosition;
        if (!onGround || jump || velocity.y > 0.0f) {
            velocity[xAxis ? 0 : 2] = 0.0f;
            return;
        }

        // Try small vertical increments. This permits walking over minor
        // shape junctions without allowing the player to climb full walls.
        constexpr float maxStep = 0.55f;
        constexpr float stepSize = 0.05f;
        for (float lift = stepSize; lift <= maxStep + 0.001f; lift += stepSize) {
            position = blockedPosition;
            position.y = oldPosition.y + lift;
            if (!collides(world, position)) {
                velocity.y = 0.0f;
                onGround = true;
                return;
            }
        }
        position = oldPosition;
        velocity[xAxis ? 0 : 2] = 0.0f;
    };

    tryHorizontalMove(velocity.x * dt, true);

    const float oldY = position.y;
    position.y += velocity.y * dt;
    if (collides(world, position)) {
        onGround = velocity.y <= 0.0f;
        position.y = oldY;
        velocity.y = 0.0f;
    } else {
        onGround = false;
    }

    tryHorizontalMove(velocity.z * dt, false);
}

glm::vec3 Player::getEyePosition() const {
    return position + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
}
