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
    virtual bool isHighlandSurface(int worldX, int worldZ, int wrapSize,
                                   std::uint32_t seed) const { return false; }
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

// Rocky Highlands planet prototypes contain distinct grassland and highland
// regions. The biome selector and both height fields are sampled canonically,
// then the shared-corner profile is built from the combined height field.
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
        for (int dz = worldZ - 1; dz <= worldZ; ++dz)
            for (int dx = worldX - 1; dx <= worldX; ++dx)
                lowest = std::min(lowest, smoothedHeight(dx, dz, maxHeight, wrapSize, seed));
        return std::min(lowest + 1, maxHeight);
    }

    bool isHighlandSurface(int worldX, int worldZ, int wrapSize,
                           std::uint32_t seed) const override {
        return biomeBlend(worldX, worldZ, wrapSize, seed) > 0.02f;
    }

private:
    float biomeBlend(int x, int z, int wrapSize, std::uint32_t seed) const {
        float selector;
        if (wrapSize > 0) {
            constexpr float tau = 6.2831853071795864769f;
            const float ax = tau * static_cast<float>(terrain::wrapCoordinate(x, wrapSize)) / wrapSize;
            const float az = tau * static_cast<float>(terrain::wrapCoordinate(z, wrapSize)) / wrapSize;
            const float px = terrain::planetSeedPhase(seed, 21);
            const float pz = terrain::planetSeedPhase(seed, 22);
            selector = (std::sin(ax + px) + std::cos(az + pz)) * 0.5f;
        } else {
            const glm::vec2 p(static_cast<float>(x) + 512.0f, static_cast<float>(z) + 512.0f);
            selector = glm::perlin(p * 0.006f);
        }
        constexpr float BORDER_HALF_BAND = 0.01f;
        return glm::smoothstep(-BORDER_HALF_BAND, BORDER_HALF_BAND, selector);
    }

    int rawHeight(int x, int z, int maxHeight, int wrapSize, std::uint32_t seed) const {
        const float blend = biomeBlend(x, z, wrapSize, seed);
        float grass;
        float high;
        if (wrapSize > 0) {
            constexpr float tau = 6.2831853071795864769f;
            const float ax = tau * static_cast<float>(terrain::wrapCoordinate(x, wrapSize)) / wrapSize;
            const float az = tau * static_cast<float>(terrain::wrapCoordinate(z, wrapSize)) / wrapSize;
            const float px = terrain::planetSeedPhase(seed, 23), pz = terrain::planetSeedPhase(seed, 24);
            grass = 20.0f
                  + (std::sin(ax * 3.0f + px) + std::cos(az * 3.0f + pz)) * 1.5f
                  + (std::sin(ax * 12.0f + px * 0.7f) + std::cos(az * 11.0f + pz * 0.7f)) * 0.6f;
            const float ridge = 1.0f - std::abs(std::sin(ax * 2.0f + std::sin(az + pz) * 0.4f + px));
            const float ridge2 = 1.0f - std::abs(std::sin(az * 3.0f + std::sin(ax + px) * 0.4f + pz));
            const float crag = std::sin(ax * 7.0f + px) * std::cos(az * 6.0f + pz) * 1.5f;
            high = 28.0f + (ridge * 0.65f + ridge2 * 0.35f) * 22.0f + crag;
        } else {
            const glm::vec2 p(static_cast<float>(x), static_cast<float>(z));
            grass = 20.0f + glm::perlin(p * 0.015f) * 3.0f
                  + glm::perlin((p + glm::vec2(40.0f, -40.0f)) * 0.06f) * 1.2f;
            const float n0 = glm::perlin((p + glm::vec2(-300.0f, 300.0f)) * 0.02f);
            const float n1 = glm::perlin((p + glm::vec2(-286.9f, 329.9f)) * 0.04f);
            const float n2 = glm::perlin((p + glm::vec2(-273.8f, 359.8f)) * 0.08f);
            const float n3 = glm::perlin((p + glm::vec2(-260.7f, 389.7f)) * 0.16f);
            const float ridge = (1.0f-std::abs(n0))*0.5333f + (1.0f-std::abs(n1))*0.2667f
                              + (1.0f-std::abs(n2))*0.1333f + (1.0f-std::abs(n3))*0.0667f;
            high = 28.0f + ridge * 22.0f + glm::perlin((p + glm::vec2(90.0f,-90.0f))*0.08f)*3.0f;
        }
        return std::max(2, std::min(static_cast<int>(std::round(grass + (high - grass) * blend)), maxHeight));
    }

    int smoothedHeight(int x, int z, int maxHeight, int wrapSize, std::uint32_t seed) const {
        constexpr int weights[5] = {1,2,3,2,1};
        int sum=0, total=0;
        for(int dz=-2;dz<=2;++dz) for(int dx=-2;dx<=2;++dx) {
            const int w=weights[dx+2]*weights[dz+2];
            sum += rawHeight(x+dx,z+dz,maxHeight,wrapSize,seed)*w;
            total += w;
        }
        return std::max(1,std::min((sum+total/2)/total,maxHeight));
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
