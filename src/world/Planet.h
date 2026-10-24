#pragma once

#include <cstdint>
#include <string>

// Lightweight persistent identity/configuration for a generated planet.
// Planet metadata describes the 128x128 surface footprint and a 256-block
// vertical world. Chunk storage remains 64 blocks tall.
struct Planet {
    std::string id = "grassland-01";
    std::string name;
    std::uint32_t seed = 0;
    std::string biome = "grassland";
    int width = 128;
    int depth = 128;
    int height = 256;
    int seaLevel = 24;
    std::uint32_t generationVersion = 1;

    static std::string generateName(const std::string& stableId, std::uint32_t seedValue) {
        static constexpr const char* starts[] = {
            "Vel", "Khar", "Nyth", "Aster", "Or", "Tal", "Zer", "Mira",
            "Cal", "Vora", "Eld", "Rhy", "Sol", "Thal", "Iri", "Drax"
        };
        static constexpr const char* endings[] = {
            "mora", "vox", "ara", "ion", "aris", "ora", "une", "eth",
            "Prime", "os", "eria", "oth", "ara", "eon", "is", "a"
        };

        // Stable, platform-independent FNV-1a-style mixing means the initial
        // name is reproducible for a given ID and seed. The saved name then
        // remains authoritative if this algorithm changes in a future version.
        std::uint32_t hash = 2166136261u ^ seedValue;
        for (unsigned char ch : stableId) {
            hash ^= ch;
            hash *= 16777619u;
        }
        hash ^= seedValue + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        const auto startCount = static_cast<std::uint32_t>(sizeof(starts) / sizeof(starts[0]));
        const auto endingCount = static_cast<std::uint32_t>(sizeof(endings) / sizeof(endings[0]));
        return std::string(starts[hash % startCount]) + endings[(hash >> 8) % endingCount];
    }

    void ensureName() {
        if (name.empty()) name = generateName(id, seed);
    }
};
