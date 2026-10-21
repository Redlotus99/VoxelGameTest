#include "game/Player.h"
#include "world/World.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr float kCollisionEpsilon = 0.001f;
constexpr float kGroundSnapDistance = 0.12f;
constexpr float kMaxStepHeight = 0.55f;

float shapeTopAt(BlockShape shape, float x, float z) {
    const auto h = getBlockShapeData(shape).topHeights;
    // Match the diagonal used by the top-face mesh.
    if (z <= x) {
        return h[0] * (1.0f - x) + h[1] * (x - z) + h[2] * z;
    }
    return h[0] * (1.0f - z) + h[2] * x + h[3] * (z - x);
}

bool pointInsideExplicitShape(BlockShape shape, float x, float y, float z) {
    constexpr float eps = 0.0001f;
    switch (shape) {
        case BlockShape::CornerCutSouthWest:
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
            return false;
    }
}

std::array<glm::vec2, 9> footprintSamples(const glm::vec3& p) {
    constexpr float inset = 0.002f;
    const float left = p.x - Player::HALF_WIDTH + inset;
    const float right = p.x + Player::HALF_WIDTH - inset;
    const float nearZ = p.z - Player::HALF_WIDTH + inset;
    const float farZ = p.z + Player::HALF_WIDTH - inset;
    const float midX = p.x;
    const float midZ = p.z;
    return {{
        {left, nearZ}, {midX, nearZ}, {right, nearZ},
        {left, midZ},  {midX, midZ},  {right, midZ},
        {left, farZ},  {midX, farZ},  {right, farZ}
    }};
}

// Find the highest walkable surface beneath the player's footprint. Sampling
// the footprint instead of lifting the whole player in fixed increments makes
// movement up a ramp continuous and avoids the old step-up oscillation.
bool findGroundHeight(const World& world, const glm::vec3& p,
                      float referenceY, float maxRise, float maxDrop,
                      float& groundY) {
    float highest = -1.0e9f;
    bool found = false;
    const auto samples = footprintSamples(p);

    const int minCellY = static_cast<int>(std::floor(referenceY - maxDrop)) - 1;
    const int maxCellY = static_cast<int>(std::floor(referenceY + maxRise));

    for (const auto& sample : samples) {
        const int cellX = static_cast<int>(std::floor(sample.x));
        const int cellZ = static_cast<int>(std::floor(sample.y));
        const float localX = sample.x - cellX;
        const float localZ = sample.y - cellZ;

        for (int cellY = maxCellY; cellY >= minCellY; --cellY) {
            if (!world.isSolid(cellX, cellY, cellZ)) continue;
            const BlockShape shape = world.getShapeAt(cellX, cellY, cellZ);
            if (usesExplicitBlockShapeGeometry(shape)) continue;

            const float surface = static_cast<float>(cellY) +
                                  shapeTopAt(shape, localX, localZ);
            if (surface > referenceY + maxRise + kCollisionEpsilon ||
                surface < referenceY - maxDrop - kCollisionEpsilon) {
                continue;
            }
            highest = std::max(highest, surface);
            found = true;
            // Only the topmost solid surface in this column can be the ground.
            break;
        }
    }

    if (!found) return false;
    groundY = highest;
    return true;
}
}

bool Player::collides(const World& world, const glm::vec3& p) const {
    const int x0 = static_cast<int>(std::floor(p.x - HALF_WIDTH + kCollisionEpsilon));
    const int x1 = static_cast<int>(std::floor(p.x + HALF_WIDTH - kCollisionEpsilon));
    const int y0 = static_cast<int>(std::floor(p.y + kCollisionEpsilon));
    const int y1 = static_cast<int>(std::floor(p.y + HEIGHT - kCollisionEpsilon));
    const int z0 = static_cast<int>(std::floor(p.z - HALF_WIDTH + kCollisionEpsilon));
    const int z1 = static_cast<int>(std::floor(p.z + HALF_WIDTH - kCollisionEpsilon));
    const auto samples = footprintSamples(p);

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
                    const float bottom = p.y - static_cast<float>(y);
                    const float top = p.y + HEIGHT - static_cast<float>(y);

                    if (usesExplicitBlockShapeGeometry(shape)) {
                        const float clampedBottom = std::clamp(bottom, 0.0001f, 0.9999f);
                        const float clampedTop = std::clamp(top, 0.0001f, 0.9999f);
                        if (pointInsideExplicitShape(shape, localX, clampedBottom, localZ) ||
                            pointInsideExplicitShape(shape, localX, clampedTop, localZ)) {
                            return true;
                        }
                    } else {
                        const float surface = shapeTopAt(shape, localX, localZ);
                        // The feet may touch the surface, but must not pass below it.
                        if (top > 0.0f && bottom < surface - kCollisionEpsilon) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void Player::update(float dt, const World& world, glm::vec3 moveDir, bool jump) {
    // Avoid a large movement step causing tunnelling or visible collision jitter.
    dt = std::clamp(dt, 0.0f, 0.05f);

    velocity.x = moveDir.x * WALK_SPEED;
    velocity.z = moveDir.z * WALK_SPEED;

    const bool wasGrounded = onGround;
    if (jump && wasGrounded) {
        velocity.y = JUMP_SPEED;
        onGround = false;
    } else if (!wasGrounded) {
        velocity.y -= GRAVITY * dt;
    } else {
        velocity.y = 0.0f;
    }
    velocity.y = std::max(velocity.y, -50.0f);

    const auto moveHorizontal = [&](float delta, bool xAxis) {
        if (delta == 0.0f) return;
        const glm::vec3 oldPosition = position;
        glm::vec3 candidate = oldPosition;
        if (xAxis) candidate.x += delta;
        else candidate.z += delta;

        // While grounded, follow the actual profile under the footprint.
        // The rise limit prevents this from climbing vertical cube walls.
        if (wasGrounded && !jump && velocity.y <= 0.0f) {
            float groundY = oldPosition.y;
            if (findGroundHeight(world, candidate, oldPosition.y,
                                 kMaxStepHeight, kGroundSnapDistance, groundY) &&
                groundY >= oldPosition.y - kGroundSnapDistance &&
                groundY <= oldPosition.y + kMaxStepHeight) {
                candidate.y = groundY;
                if (!collides(world, candidate)) {
                    position = candidate;
                    velocity.y = 0.0f;
                    onGround = true;
                    return;
                }
            }
        }

        if (!collides(world, candidate)) {
            position = candidate;
            return;
        }
        velocity[xAxis ? 0 : 2] = 0.0f;
    };

    moveHorizontal(velocity.x * dt, true);
    moveHorizontal(velocity.z * dt, false);

    if (onGround && !jump) {
        velocity.y = 0.0f;
        return;
    }

    const float oldY = position.y;
    position.y += velocity.y * dt;
    if (collides(world, position)) {
        if (velocity.y < 0.0f) onGround = true;
        position.y = oldY;
        velocity.y = 0.0f;
    } else {
        onGround = false;
    }

    // If falling onto a ramp or wedge, land on its profile rather than its cell.
    if (velocity.y <= 0.0f) {
        float groundY = position.y;
        if (findGroundHeight(world, position, position.y,
                             kCollisionEpsilon, std::max(0.0f, -velocity.y * dt) + 0.08f,
                             groundY) &&
            position.y <= groundY + 0.08f && position.y >= groundY - 0.2f) {
            position.y = groundY;
            velocity.y = 0.0f;
            onGround = true;
        }
    }
}

glm::vec3 Player::getEyePosition() const {
    return position + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
}
