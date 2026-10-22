#pragma once

#include <unordered_map>
#include <memory>
#include <cstdint>
#include <string>
#include "world/Chunk.h"
#include "render/Mesh.h"
#include <glm/glm.hpp>

class Shader;

class World {
public:
    World();
    ~World();

    void generate(int radius);
    // Small flat grassland used to validate finite-world wraparound.
    void generatePlanetPrototype();
    bool loadPlanetEdits(const std::string& path, const std::string& planetId);
    bool savePlanetEdits(const std::string& path, const std::string& planetId) const;
    bool isPlanetPrototype() const { return m_planetPrototype; }
    void wrapPosition(glm::vec3& position) const;
    Chunk* getChunk(glm::ivec3 position);
    void draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection);
    void setShapeDebugMode(bool enabled) { m_shapeDebugMode = enabled; }
    bool isSolid(int wx, int wy, int wz) const;
    BlockType getBlockTypeAt(int wx, int wy, int wz) const;
    BlockShape getShapeAt(int wx, int wy, int wz) const;
    // Exact shared-corner terrain height used by both rendering and collision.
    bool getTerrainSurfaceHeight(float worldX, float worldZ, float& height) const;
    void setBlockWorld(int wx, int wy, int wz, BlockType type);
    void setBlockShapeWorld(int wx, int wy, int wz, BlockShape shape);
    void generateCornerCutTestArrangement();
    bool raycast(glm::vec3 origin, glm::vec3 dir, float maxDist, glm::ivec3& hitBlock, glm::ivec3& prevBlock) const;

private:
    int64_t packCoordinates(glm::ivec3 coords) const;
    glm::ivec3 unpackCoordinates(int64_t packed) const;
    BlockType getBlockWorld(int wx, int wy, int wz) const;
    BlockShape getBlockShapeWorld(int wx, int wy, int wz) const;

    std::unordered_map<int64_t, std::unique_ptr<Chunk>> m_chunks;
    std::unordered_map<int64_t, Mesh> m_meshes;
    bool m_shapeDebugMode = false;
    bool m_planetPrototype = false;
    static constexpr int PLANET_CHUNKS = 8;
    static constexpr int PLANET_SIZE = PLANET_CHUNKS * Chunk::SIZE_X;
    int wrapCoordinate(int value) const;
    std::uint32_t packBlockCoordinates(int x, int y, int z) const;
    struct BlockEdit { BlockType type; BlockShape shape; };
    std::unordered_map<std::uint32_t, BlockEdit> m_blockEdits;
};
