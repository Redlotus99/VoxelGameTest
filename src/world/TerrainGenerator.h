#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "world/TerrainHeight.h"

// Terrain generation is independent from chunk storage. Implementations
// provide canonical surface/corner samples; Chunk turns those samples into
// block materials and shared block shapes.
class TerrainGenerator {
public:
    virtual ~TerrainGenerator() = default;

    virtual int surfaceHeight(int worldX, int worldZ, int maxHeight,
                             int wrapSize, std::uint32_t seed) const = 0;
    virtual int cornerHeight(int worldX, int worldZ, int maxHeight,
                             int wrapSize, std::uint32_t seed) const = 0;
};

class GrasslandTerrainGenerator final : public TerrainGenerator {
public:
    int surfaceHeight(int worldX, int worldZ, int maxHeight,
                      int wrapSize, std::uint32_t seed) const override {
        return wrapSize > 0
            ? terrain::surfaceHeightPeriodic(worldX, worldZ, maxHeight, wrapSize, seed)
            : terrain::surfaceHeight(worldX, worldZ, maxHeight);
    }

    int cornerHeight(int worldX, int worldZ, int maxHeight,
                     int wrapSize, std::uint32_t seed) const override {
        return wrapSize > 0
            ? terrain::cornerHeightPeriodic(worldX, worldZ, maxHeight, wrapSize, seed)
            : terrain::cornerHeight(worldX, worldZ, maxHeight);
    }
};

// Rocky highlands use the same periodic base field, but amplify relief
// around the mean elevation. Smoothing and corner selection are repeated on
// the transformed samples so the surface/corner invariants remain identical.
class RockyHighlandsTerrainGenerator final : public TerrainGenerator {
public:
    int surfaceHeight(int worldX, int worldZ, int maxHeight,
                      int wrapSize, std::uint32_t seed) const override {
        return std::min({
            cornerHeight(worldX, worldZ, maxHeight, wrapSize, seed),
            cornerHeight(worldX + 1, worldZ, maxHeight, wrapSize, seed),
            cornerHeight(worldX + 1, worldZ + 1, maxHeight, wrapSize, seed),
            cornerHeight(worldX, worldZ + 1, maxHeight, wrapSize, seed)
        });
    }

    int cornerHeight(int worldX, int worldZ, int maxHeight,
                     int wrapSize, std::uint32_t seed) const override {
        int lowest = maxHeight + 1;
        for (int dz = worldZ - 1; dz <= worldZ; ++dz) {
            for (int dx = worldX - 1; dx <= worldX; ++dx) {
                lowest = std::min(lowest, smoothedHeight(dx, dz, maxHeight, wrapSize, seed));
            }
        }
        return std::min(lowest + 1, maxHeight);
    }

private:
    int rawHighlandHeight(int worldX, int worldZ, int maxHeight,
                          int wrapSize, std::uint32_t seed) const {
        const int base = wrapSize > 0
            ? terrain::rawHeightPeriodic(worldX, worldZ, maxHeight, wrapSize, seed)
            : terrain::rawHeight(worldX, worldZ, maxHeight);
        constexpr float reliefScale = 1.65f;
        const int amplified = 24 + static_cast<int>(std::round((base - 24) * reliefScale));
        return std::max(1, std::min(amplified, maxHeight));
    }

    int smoothedHeight(int worldX, int worldZ, int maxHeight,
                       int wrapSize, std::uint32_t seed) const {
        constexpr int weights[5] = {1, 2, 3, 2, 1};
        int weightedHeight = 0;
        int totalWeight = 0;
        for (int dz = -2; dz <= 2; ++dz) {
            for (int dx = -2; dx <= 2; ++dx) {
                const int weight = weights[dx + 2] * weights[dz + 2];
                weightedHeight += rawHighlandHeight(worldX + dx, worldZ + dz,
                                                    maxHeight, wrapSize, seed) * weight;
                totalWeight += weight;
            }
        }
        return std::max(1, std::min(
            (weightedHeight + totalWeight / 2) / totalWeight, maxHeight));
    }
};

inline std::unique_ptr<TerrainGenerator> createTerrainGenerator(const std::string& biome) {
    if (biome == "rocky_highlands") {
        return std::make_unique<RockyHighlandsTerrainGenerator>();
    }
    // Unknown biome IDs intentionally fall back to grassland so older
    // planet metadata remains loadable.
    return std::make_unique<GrasslandTerrainGenerator>();
}
