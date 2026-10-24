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

// Unknown biome IDs intentionally fall back to grassland until dedicated
// generators are implemented. This keeps older planet metadata loadable.
inline std::unique_ptr<TerrainGenerator> createTerrainGenerator(const std::string& biome) {
    (void)biome;
    return std::make_unique<GrasslandTerrainGenerator>();
}
