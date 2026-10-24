#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <cstdint>
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

inline int wrapCoordinate(int value, int period) {
    const int wrapped = value % period;
    return wrapped < 0 ? wrapped + period : wrapped;
}

// Periodic terrain for finite worlds: every component repeats exactly at the
// world period, including the shared corner profile at the opposite edge.
inline std::uint32_t mixPlanetSeed(std::uint32_t v) { v += 0x9e3779b9u; v = (v ^ (v >> 16)) * 0x85ebca6bu; v = (v ^ (v >> 13)) * 0xc2b2ae35u; return v ^ (v >> 16); }
inline float planetSeedPhase(std::uint32_t seed, std::uint32_t lane) { if (seed == 0) return 0.0f; constexpr float tau = 6.2831853071795864769f; return tau * (static_cast<float>(mixPlanetSeed(seed ^ (lane * 0x45d9f3bu)) & 0x00ffffffu) / 16777216.0f); }
inline int rawHeightPeriodic(int worldX, int worldZ, int maxHeight, int period, std::uint32_t seed = 0) {
    constexpr float tau = 6.2831853071795864769f;
    const float x = tau * static_cast<float>(wrapCoordinate(worldX, period)) / static_cast<float>(period);
    const float z = tau * static_cast<float>(wrapCoordinate(worldZ, period)) / static_cast<float>(period);
    const float px = planetSeedPhase(seed, 1), pz = planetSeedPhase(seed, 2), p2 = planetSeedPhase(seed, 3), p3 = planetSeedPhase(seed, 4);
    const float broad = std::sin(x + px) * std::cos(z * 2.0f + pz) * 10.0f + std::cos(x * 2.0f + p2) * std::sin(z + p3) * 7.0f;
    const float medium = std::sin(x * 4.0f + 0.4f + p3) * std::cos(z * 3.0f + px) * 3.0f + std::cos(x * 3.0f + pz) * std::sin(z * 5.0f + 0.8f + p2) * 2.0f;
    const float detail = std::sin(x * 8.0f + z * 6.0f + p2) * 1.0f;
    const float height = 24.0f + broad + medium + detail;
    return std::max(1, std::min(static_cast<int>(std::round(height)), maxHeight));
}

// Rocky Highlands use a distinct ridged profile rather than merely
// scaling grassland heights. Integer angular harmonics preserve exact wrapping.
inline int rawHighlandsHeightPeriodic(int worldX, int worldZ, int maxHeight, int period, std::uint32_t seed = 0) {
    constexpr float tau = 6.2831853071795864769f;
    const float x = tau * static_cast<float>(wrapCoordinate(worldX, period)) / static_cast<float>(period);
    const float z = tau * static_cast<float>(wrapCoordinate(worldZ, period)) / static_cast<float>(period);
    const float px = planetSeedPhase(seed, 11), pz = planetSeedPhase(seed, 12);
    const float p2 = planetSeedPhase(seed, 13), p3 = planetSeedPhase(seed, 14);

    const float ridgeA = 1.0f - std::abs(std::sin(x * 2.0f + std::sin(z + pz) * 0.65f + px));
    const float ridgeB = 1.0f - std::abs(std::sin(z * 3.0f + std::cos(x * 2.0f + p2) * 0.45f + pz));
    const float broad = ((ridgeA + ridgeB) * 0.5f - 0.5f) * 20.0f;
    const float medium = (std::sin(x * 4.0f + p3) * std::cos(z * 3.0f + px)
                        + std::cos(x * 5.0f + pz) * std::sin(z * 4.0f + p2)) * 1.25f;
    const float detail = std::sin(x * 7.0f + z * 6.0f + p3) * 0.6f;
    const float height = 14.0f + broad + medium + detail;
    return std::max(1, std::min(static_cast<int>(std::round(height)), maxHeight));
}

inline int rawHighlandsHeight(int worldX, int worldZ, int maxHeight) {
    const glm::vec2 p(static_cast<float>(worldX), static_cast<float>(worldZ));
    const float n0 = glm::perlin(p * 0.0028f);
    const float n1 = glm::perlin(p * 0.0065f);
    const float n2 = glm::perlin(p * 0.014f);
    const float ridge0 = 1.0f - std::abs(n0);
    const float ridge1 = 1.0f - std::abs(n1);
    const float height = 15.0f + ridge0 * 10.0f + ridge1 * 3.0f + n2 * 1.2f;
    return std::max(1, std::min(static_cast<int>(std::round(height)), maxHeight));
}

inline int smoothedHeightPeriodic(int worldX, int worldZ, int maxHeight, int period, std::uint32_t seed = 0) {
    constexpr int weights[5] = {1, 2, 3, 2, 1};
    int weightedHeight = 0;
    int totalWeight = 0;
    for (int dz = -2; dz <= 2; ++dz) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int weight = weights[dx + 2] * weights[dz + 2];
            weightedHeight += rawHeightPeriodic(worldX + dx, worldZ + dz, maxHeight, period, seed) * weight;
            totalWeight += weight;
        }
    }
    return std::max(1, std::min((weightedHeight + totalWeight / 2) / totalWeight, maxHeight));
}

inline int cornerHeightPeriodic(int cornerX, int cornerZ, int maxHeight, int period, std::uint32_t seed = 0) {
    int lowest = maxHeight + 1;
    for (int dz = cornerZ - 1; dz <= cornerZ; ++dz) {
        for (int dx = cornerX - 1; dx <= cornerX; ++dx) {
            lowest = std::min(lowest, smoothedHeightPeriodic(dx, dz, maxHeight, period, seed));
        }
    }
    return lowest + 1;
}

inline int surfaceHeightPeriodic(int worldX, int worldZ, int maxHeight, int period, std::uint32_t seed = 0) {
    return std::min({
        cornerHeightPeriodic(worldX, worldZ, maxHeight, period, seed),
        cornerHeightPeriodic(worldX + 1, worldZ, maxHeight, period, seed),
        cornerHeightPeriodic(worldX + 1, worldZ + 1, maxHeight, period, seed),
        cornerHeightPeriodic(worldX, worldZ + 1, maxHeight, period, seed)
    });
}

} // namespace terrain
