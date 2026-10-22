#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

// Canonical deterministic height sampling shared by terrain generation and
// player collision. Keep this function free of chunk/world state so adjacent
// chunks calculate exactly the same heights at their shared boundaries.
namespace terrain {

inline int rawHeight(int worldX, int worldZ, int maxHeight) {
    const float noise = glm::perlin(glm::vec2(
        static_cast<float>(worldX), static_cast<float>(worldZ)) * 0.01f);
    return std::max(1, std::min(
        static_cast<int>(noise * 18.0f + 24.0f), maxHeight));
}

inline int smoothedHeight(int worldX, int worldZ, int maxHeight) {
    constexpr int weights[5] = {1, 2, 3, 2, 1};
    int weightedHeight = 0;
    int totalWeight = 0;

    for (int dz = -2; dz <= 2; ++dz) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int weight = weights[dx + 2] * weights[dz + 2];
            weightedHeight += rawHeight(worldX + dx, worldZ + dz, maxHeight) * weight;
            totalWeight += weight;
        }
    }

    return std::max(1, std::min(
        (weightedHeight + totalWeight / 2) / totalWeight, maxHeight));
}

} // namespace terrain
