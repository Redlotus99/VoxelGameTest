#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
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
    // Mesh construction and collision query the same nearby samples many
    // times. Cache the deterministic result so those queries don't rerun 25
    // noise samples for every corner of every terrain cell.
    using CacheKey = std::tuple<int, int, int>;
    static std::map<CacheKey, int> cache;
    const CacheKey key{worldX, worldZ, maxHeight};
    const auto cached = cache.find(key);
    if (cached != cache.end()) {
        return cached->second;
    }

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

    const int result = std::max(1, std::min(
        (weightedHeight + totalWeight / 2) / totalWeight, maxHeight));
    cache.emplace(key, result);
    return result;
}


// Shared world-grid corner level. A corner belongs to four cells, so it is
// derived only from those cells' canonical smoothed samples.
inline int cornerHeight(int cornerX, int cornerZ, int maxHeight) {
    int lowest = maxHeight + 1;
    for (int dz = cornerZ - 1; dz <= cornerZ; ++dz) {
        for (int dx = cornerX - 1; dx <= cornerX; ++dx) {
            lowest = std::min(lowest, smoothedHeight(dx, dz, maxHeight));
        }
    }
    return lowest + 1;
}

// Place each grass surface cell at the lowest of its four shared corner
// levels. This guarantees no corner falls below its cell base.
inline int surfaceHeight(int worldX, int worldZ, int maxHeight) {
    return std::min({
        cornerHeight(worldX,     worldZ,     maxHeight),
        cornerHeight(worldX + 1, worldZ,     maxHeight),
        cornerHeight(worldX + 1, worldZ + 1, maxHeight),
        cornerHeight(worldX,     worldZ + 1, maxHeight)
    });
}

} // namespace terrain
