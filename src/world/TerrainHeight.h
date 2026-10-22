#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

// Canonical deterministic height sampling shared by terrain generation and
// player collision. Keep this function free of chunk/world state so adjacent
// chunks calculate exactly the same heights at their shared boundaries.
namespace terrain {

inline int rawHeight(int worldX, int worldZ, int maxHeight) {
    const glm::vec2 position(static_cast<float>(worldX), static_cast<float>(worldZ));

    // Build terrain at several scales: broad landforms define the main hills
    // and valleys, medium noise breaks up the silhouettes, and a small amount
    // of fine detail keeps the surface from feeling overly uniform.
    const float broad = glm::perlin(position * 0.0045f) * 20.0f;
    const float medium = glm::perlin(position * 0.012f) * 8.0f;
    const float detail = glm::perlin(position * 0.035f) * 2.0f;
    const float height = 24.0f + broad + medium + detail;

    return std::max(1, std::min(static_cast<int>(std::round(height)), maxHeight));
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
