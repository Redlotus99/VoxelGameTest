#include "world/World.h"
#include "world/TerrainHeight.h"
#include "render/Shader.h"
#include <algorithm>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <vector>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <utility>
#include <iostream>

World::World() = default;

World::~World() = default;

int World::wrapCoordinate(int value) const {
    if (!m_planetPrototype) return value;
    const int wrapped = value % PLANET_SIZE;
    return wrapped < 0 ? wrapped + PLANET_SIZE : wrapped;
}

std::uint32_t World::packBlockCoordinates(int x, int y, int z) const {
    // Planet edit keys reserve 7 bits each for X, offset-Y, and Z.
    // Offset-Y maps the supported world range [-64, 63] to [0, 127].
    return (static_cast<std::uint32_t>(x) << 14)
         | (static_cast<std::uint32_t>(y + 64) << 7)
         | static_cast<std::uint32_t>(z);
}

bool World::savePlanetEdits(const std::string& path, const std::string& planetId) const {
    if (!m_planetPrototype || planetId.empty()) return false;
    namespace fs = std::filesystem;
    const fs::path target(path), temporary = target.string() + ".tmp", backup = target.string() + ".bak";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file) return false;
        file << "VOXEL_PLANET_EDITS 3 " << PLANET_SIZE << ' ' << (Chunk::SIZE_Y * 2) << ' ' << planetId << '\n';
        for (const auto& entry : m_blockEdits) {
            const std::uint32_t key = entry.first;
            const int x = static_cast<int>((key >> 14) & 0x7f);
            const int y = static_cast<int>((key >> 7) & 0x7f) - 64;
            const int z = static_cast<int>(key & 0x7f);
            file << x << ' ' << y << ' ' << z << ' '
                 << static_cast<unsigned int>(entry.second.type) << ' '
                 << static_cast<unsigned int>(entry.second.shape) << '\n';
        }
        file.flush();
        if (!file.good()) {
            file.close();
            std::error_code ignored; fs::remove(temporary, ignored);
            return false;
        }
    }
    // Stage the new save first and preserve the previous good file for rollback.
    std::error_code ec;
    fs::remove(backup, ec); ec.clear();
    const bool hadTarget = fs::exists(target, ec) && !ec;
    if (hadTarget) {
        fs::rename(target, backup, ec);
        if (ec) { std::error_code ignored; fs::remove(temporary, ignored); return false; }
    }
    fs::rename(temporary, target, ec);
    if (ec) {
        if (hadTarget) { std::error_code restoreError; fs::rename(backup, target, restoreError); }
        std::error_code ignored; fs::remove(temporary, ignored);
        return false;
    }
    fs::remove(backup, ec);
    return true;
}
bool World::loadPlanetEdits(const std::string& path, const std::string& planetId) {
    if (!m_planetPrototype) return false;
    std::ifstream file(path);
    if (!file) return false;
    std::string magic;
    int version = 0, worldSize = 0, worldHeight = 0;
    std::string savedPlanetId;
    if (!(file >> magic >> version >> worldSize >> worldHeight)
        || magic != "VOXEL_PLANET_EDITS"
        || (version < 1 || version > 3)
        || worldSize != PLANET_SIZE
        || worldHeight != (version == 3 ? Chunk::SIZE_Y * 2 : Chunk::SIZE_Y)) return false;
    // Version 1 was the original single-planet save format. Accept it for
    // migration, while version 2 binds the file to its explicit planet ID.
    if (version >= 2 && (!(file >> savedPlanetId) || savedPlanetId != planetId)) return false;
    if (version == 1 && planetId != "grassland-01") return false;

    std::unordered_map<std::uint32_t, BlockEdit> loaded;
    int x = 0, y = 0, z = 0;
    unsigned int type = 0, shape = 0;
    while (file >> x >> y >> z >> type >> shape) {
        if (x < 0 || x >= PLANET_SIZE || z < 0 || z >= PLANET_SIZE
            || (version == 3 ? (y < -Chunk::SIZE_Y || y >= Chunk::SIZE_Y) : (y < 0 || y >= Chunk::SIZE_Y))
            || type > static_cast<unsigned int>(BlockType::Stone)
            || shape >= static_cast<unsigned int>(BlockShape::Count)) return false;
        loaded[packBlockCoordinates(x, y, z)] = {
            static_cast<BlockType>(type), static_cast<BlockShape>(shape)
        };
    }
    if (!file.eof()) return false;

    // Apply only after the entire file validates, avoiding partial restoration
    // if a save is malformed or belongs to a different world size.
    m_blockEdits.clear();
    for (const auto& entry : loaded) {
        const std::uint32_t key = entry.first;
        const int bx = static_cast<int>((key >> 14) & 0x7f);
        const int by = static_cast<int>((key >> 7) & 0x7f) - 64;
        const int bz = static_cast<int>(key & 0x7f);
        setBlockWorld(bx, by, bz, entry.second.type);
        if (entry.second.type != BlockType::Air) {
            setBlockShapeWorld(bx, by, bz, entry.second.shape);
        }
    }
    return true;
}

void World::wrapPosition(glm::vec3& position) const {
    if (!m_planetPrototype) return;
    const float size = static_cast<float>(PLANET_SIZE);
    position.x = std::fmod(position.x, size);
    position.z = std::fmod(position.z, size);
    if (position.x < 0.0f) position.x += size;
    if (position.z < 0.0f) position.z += size;
}

void World::generatePlanetPrototype(Planet planet, int undergroundDepth) {
    // Keep the current save-key encoding and chunk alignment: supported depths
    // are whole 16-block layers from 16 through 64 blocks.
    undergroundDepth = std::clamp(undergroundDepth, Chunk::SIZE_Y, Chunk::SIZE_Y * 4);
    undergroundDepth = ((undergroundDepth + Chunk::SIZE_Y / 2) / Chunk::SIZE_Y) * Chunk::SIZE_Y;
    m_undergroundDepth = undergroundDepth;
    std::cout << "World.cpp depth after assignment=" << m_undergroundDepth << "\\n";
    m_chunks.clear();
    m_meshes.clear();
    m_planetPrototype = true;
    planet.ensureName();
    m_planet = std::move(planet);
    std::cout << "World.cpp depth after planet assignment=" << m_undergroundDepth << "\\n";
    for (int x = 0; x < PLANET_CHUNKS; ++x) {
        for (int z = 0; z < PLANET_CHUNKS; ++z) {
            const glm::ivec3 surfacePos(x, 0, z);
            auto surface = std::make_unique<Chunk>(surfacePos);
            surface->generateTerrain(PLANET_SIZE, m_planet.seed);
            m_chunks.emplace(packCoordinates(surfacePos), std::move(surface));

            // Generate configurable solid-stone layers below the unchanged
            // surface. Each layer is a complete 16-block chunk for clean seams.
            for (int chunkY = -1; chunkY >= -(m_undergroundDepth / Chunk::SIZE_Y); --chunkY) {
                const glm::ivec3 lowerPos(x, chunkY, z);
                auto lower = std::make_unique<Chunk>(lowerPos);
                for (int localZ = 0; localZ < Chunk::SIZE_Z; ++localZ) {
                    for (int localX = 0; localX < Chunk::SIZE_X; ++localX) {
                        for (int localY = 0; localY < Chunk::SIZE_Y; ++localY) {
                            lower->setBlock(localX, localY, localZ, BlockType::Stone);
                        }
                    }
                }
                m_chunks.emplace(packCoordinates(lowerPos), std::move(lower));
            }
        }
    }
}

void World::generate(int radius) {
    m_planetPrototype = false;
    for (int x = -radius; x <= radius; ++x) {
        for (int z = -radius; z <= radius; ++z) {
            // Only generate at y = 0
            glm::ivec3 chunkPos(x, 0, z);
            int64_t packed = packCoordinates(chunkPos);
            
            if (m_chunks.find(packed) == m_chunks.end()) {
                auto chunk = std::make_unique<Chunk>(chunkPos);
                chunk->generateTerrain();
                m_chunks[packed] = std::move(chunk);
            }
        }
    }

}

Chunk* World::getChunk(glm::ivec3 position) {
    int64_t packed = packCoordinates(position);
    auto it = m_chunks.find(packed);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

BlockType World::getBlockWorld(int wx, int wy, int wz) const {
    wx = wrapCoordinate(wx);
    wz = wrapCoordinate(wz);
    if (m_planetPrototype) {
        if (wy < -m_undergroundDepth || wy >= Chunk::SIZE_Y) return BlockType::Air;
    } else if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return BlockType::Air;
    }
    // Calculate chunk coordinates
    int chunkX = wx / 16;
    int chunkZ = wz / 16;
    
    // Handle negative coordinates correctly for floor division
    if (wx < 0 && wx % 16 != 0) {
        chunkX--;
    }
    if (wz < 0 && wz % 16 != 0) {
        chunkZ--;
    }
    
    const int chunkY = m_planetPrototype
        ? static_cast<int>(std::floor(static_cast<double>(wy) / Chunk::SIZE_Y)) : 0;
    glm::ivec3 chunkPos(chunkX, chunkY, chunkZ);
    int64_t packed = packCoordinates(chunkPos);
    
    auto it = m_chunks.find(packed);
    if (it == m_chunks.end()) {
        return BlockType::Air;
    }
    
    Chunk* chunk = it->second.get();
    
    // Convert world coordinates to local coordinates
    int localX = wx - (chunkX * 16);
    int localZ = wz - (chunkZ * 16);
    
    const int localY = wy - chunkY * Chunk::SIZE_Y;
    return chunk->getBlock(localX, localY, localZ);
}

BlockShape World::getBlockShapeWorld(int wx, int wy, int wz) const {
    wx = wrapCoordinate(wx);
    wz = wrapCoordinate(wz);
    if (m_planetPrototype) {
        if (wy < -m_undergroundDepth || wy >= Chunk::SIZE_Y) return BlockShape::Cube;
    } else if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return BlockShape::Cube;
    }

    const int chunkX = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    const int chunkZ = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    const int chunkY = m_planetPrototype
        ? static_cast<int>(std::floor(static_cast<double>(wy) / Chunk::SIZE_Y)) : 0;
    const glm::ivec3 chunkPos(chunkX, chunkY, chunkZ);
    const auto it = m_chunks.find(packCoordinates(chunkPos));
    if (it == m_chunks.end()) {
        return BlockShape::Cube;
    }

    const int localX = wx - chunkX * Chunk::SIZE_X;
    const int localY = wy - chunkY * Chunk::SIZE_Y;
    const int localZ = wz - chunkZ * Chunk::SIZE_Z;
    return it->second->getBlockShape(localX, localY, localZ);
}

BlockShape World::getShapeAt(int wx, int wy, int wz) const {
    return getBlockShapeWorld(wx, wy, wz);
}

bool World::getTerrainSurfaceHeight(float worldX, float worldZ, float& height) const {
    const int cellX = static_cast<int>(std::floor(worldX));
    const int cellZ = static_cast<int>(std::floor(worldZ));
    const float x = worldX - static_cast<float>(cellX);
    const float z = worldZ - static_cast<float>(cellZ);

    const auto surfaceHeight = [this](int wx, int wz) {
        return m_planetPrototype
            ? terrain::surfaceHeightPeriodic(wx, wz, Chunk::SIZE_Y - 3, PLANET_SIZE, m_planet.seed)
            : terrain::surfaceHeight(wx, wz, Chunk::SIZE_Y - 3);
    };
    const auto cornerHeight = [this](int cx, int cz) {
        return static_cast<float>(m_planetPrototype
            ? terrain::cornerHeightPeriodic(cx, cz, Chunk::SIZE_Y - 3, PLANET_SIZE, m_planet.seed)
            : terrain::cornerHeight(cx, cz, Chunk::SIZE_Y - 3));
    };

    const float sw = cornerHeight(cellX, cellZ);
    const float se = cornerHeight(cellX + 1, cellZ);
    const float ne = cornerHeight(cellX + 1, cellZ + 1);
    const float nw = cornerHeight(cellX, cellZ + 1);

    // Match Chunk::buildMesh's diagonal choice for exceptional corners/saddles.
    const int swH = static_cast<int>(sw) - surfaceHeight(cellX, cellZ);
    const int seH = static_cast<int>(se) - surfaceHeight(cellX, cellZ);
    const int neH = static_cast<int>(ne) - surfaceHeight(cellX, cellZ);
    const int nwH = static_cast<int>(nw) - surfaceHeight(cellX, cellZ);
    const int d1 = std::abs(neH - swH);
    const int d2 = std::abs(nwH - seH);
    bool useSwNeDiagonal = d1 <= d2;
    if (swH != seH && swH != neH && swH != nwH) useSwNeDiagonal = false;
    else if (seH != swH && seH != neH && seH != nwH) useSwNeDiagonal = true;
    else if (neH != swH && neH != seH && neH != nwH) useSwNeDiagonal = false;
    else if (nwH != swH && nwH != seH && nwH != neH) useSwNeDiagonal = true;
    else if (swH == neH && seH == nwH && swH != seH) useSwNeDiagonal = true;
    else if (swH == neH && seH == nwH && swH == seH) useSwNeDiagonal = true;

    if (useSwNeDiagonal) {
        height = z >= x
            ? sw * (1.0f - z) + nw * (z - x) + ne * x
            : sw * (1.0f - x) + ne * z + se * (x - z);
    } else {
        height = x + z <= 1.0f
            ? sw * (1.0f - x - z) + nw * z + se * x
            : nw * (1.0f - x) + ne * (x + z - 1.0f) + se * (1.0f - z);
    }
    return true;
}

bool World::isSolid(int wx, int wy, int wz) const {
    return getBlockWorld(wx, wy, wz) != BlockType::Air;
}

BlockType World::getBlockTypeAt(int wx, int wy, int wz) const {
    return getBlockWorld(wx, wy, wz);
}

void World::setBlockWorld(int wx, int wy, int wz, BlockType type) {
    wx = wrapCoordinate(wx);
    wz = wrapCoordinate(wz);
    if (m_planetPrototype) {
        if (wy < -m_undergroundDepth || wy >= Chunk::SIZE_Y) return;
    } else if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return;
    }
    
    // Calculate chunk coordinates using floor division
    int cx = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    int cz = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    
    // Calculate local coordinates
    int lx = wx - cx * Chunk::SIZE_X;
    int lz = wz - cz * Chunk::SIZE_Z;
    const int cy = m_planetPrototype
        ? static_cast<int>(std::floor(static_cast<double>(wy) / Chunk::SIZE_Y)) : 0;
    const int ly = wy - cy * Chunk::SIZE_Y;

    // Get the chunk at this position
    glm::ivec3 chunkPos(cx, cy, cz);
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        return;
    }
    
    // Set the block in the chunk
    chunk->setBlock(lx, ly, lz, type);
    if (m_planetPrototype) m_blockEdits[packBlockCoordinates(wx, wy, wz)] = {type, chunk->getBlockShape(lx, ly, lz)};

    // A block edit only affects this chunk's mesh and, at a chunk edge,
    // the neighboring mesh that shares the edited boundary. In planet mode,
    // wrap neighboring chunk coordinates so edits at an outer edge invalidate
    // the matching chunk on the opposite side instead of clearing every mesh.
    const auto invalidateChunk = [this, cy](int chunkX, int chunkY, int chunkZ) {
        if (m_planetPrototype) {
            chunkX = (chunkX % PLANET_CHUNKS + PLANET_CHUNKS) % PLANET_CHUNKS;
            chunkZ = (chunkZ % PLANET_CHUNKS + PLANET_CHUNKS) % PLANET_CHUNKS;
        }
        m_meshes.erase(packCoordinates(glm::ivec3(chunkX, chunkY, chunkZ)));
    };

    invalidateChunk(cx, cy, cz);
    if (lx == 0) invalidateChunk(cx - 1, cy, cz);
    if (lx == Chunk::SIZE_X - 1) invalidateChunk(cx + 1, cy, cz);
    if (lz == 0) invalidateChunk(cx, cy, cz - 1);
    if (lz == Chunk::SIZE_Z - 1) invalidateChunk(cx, cy, cz + 1);
    if (m_planetPrototype && ly == 0) invalidateChunk(cx, cy - 1, cz);
    if (m_planetPrototype && ly == Chunk::SIZE_Y - 1) invalidateChunk(cx, cy + 1, cz);
}


void World::setBlockShapeWorld(int wx, int wy, int wz, BlockShape shape) {
    wx = wrapCoordinate(wx);
    wz = wrapCoordinate(wz);
    if (m_planetPrototype) {
        if (wy < -m_undergroundDepth || wy >= Chunk::SIZE_Y) return;
    } else if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return;
    }

    const int cx = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    const int cz = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    const int lx = wx - cx * Chunk::SIZE_X;
    const int lz = wz - cz * Chunk::SIZE_Z;
    const int cy = m_planetPrototype
        ? static_cast<int>(std::floor(static_cast<double>(wy) / Chunk::SIZE_Y)) : 0;
    const int ly = wy - cy * Chunk::SIZE_Y;
    const glm::ivec3 chunkPos(cx, cy, cz);
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr || chunk->getBlock(lx, ly, lz) == BlockType::Air) {
        return;
    }

    chunk->setBlockShape(lx, ly, lz, shape);
    if (m_planetPrototype) m_blockEdits[packBlockCoordinates(wx, wy, wz)] = {chunk->getBlock(lx, ly, lz), shape};
    m_meshes.erase(packCoordinates(chunkPos));
    if (m_planetPrototype && ly == 0) m_meshes.erase(packCoordinates(glm::ivec3(cx, cy - 1, cz)));
    if (m_planetPrototype && ly == Chunk::SIZE_Y - 1) m_meshes.erase(packCoordinates(glm::ivec3(cx, cy + 1, cz)));
}

void World::generateCornerCutTestArrangement() {
    // Deliberately opt-in, elevated test platform so the normal terrain stays
    // unchanged unless the executable is launched with --shape-test.
    constexpr int platformY = 40;
    for (int z = 4; z <= 16; ++z) {
        for (int x = 5; x <= 16; ++x) {
            setBlockWorld(x, platformY, z, BlockType::Stone);
        }
    }

    // Bottom-corner cut examples, retained as isolated reference shapes.
    setBlockWorld(7, platformY + 1, 7, BlockType::Stone);
    setBlockShapeWorld(7, platformY + 1, 7, BlockShape::CornerCutSouthWest);
    setBlockWorld(8, platformY + 1, 7, BlockType::Stone);
    setBlockWorld(7, platformY + 1, 8, BlockType::Stone);

    setBlockWorld(10, platformY + 1, 7, BlockType::Stone);
    setBlockShapeWorld(10, platformY + 1, 7, BlockShape::CornerCutSouthWest);

    // Main L-shaped test: rotate the west ramp one quarter-turn from east
    // to north, and the south ramp one quarter-turn from north to east.
    // Rotate the center diagonal wedge 180 degrees from the previous setup,
    // changing the removed top corner from northeast to southwest.
    setBlockWorld(7, platformY + 1, 10, BlockType::Stone);
    setBlockShapeWorld(7, platformY + 1, 10, BlockShape::RampNorth);
    setBlockWorld(8, platformY + 1, 9, BlockType::Stone);
    setBlockShapeWorld(8, platformY + 1, 9, BlockShape::RampEast);
    setBlockWorld(8, platformY + 1, 10, BlockType::Stone);
    setBlockShapeWorld(8, platformY + 1, 10, BlockShape::CornerCutTopSouthWest);

    // A second isolated top-corner cut makes its diagonal easier to inspect.
    setBlockWorld(11, platformY + 1, 11, BlockType::Stone);
    setBlockShapeWorld(11, platformY + 1, 11, BlockShape::CornerCutTopSouthWest);

    // Isolated complementary wedge reference: remove the top northeast corner.
    setBlockWorld(12, platformY + 1, 7, BlockType::Stone);
    setBlockShapeWorld(12, platformY + 1, 7, BlockShape::CornerCutTopNorthEast);

    // Isolated single-corner wedge: one raised northeast corner slopes down
    // toward the other three corners. Kept separate to inspect its silhouette.
    setBlockWorld(15, platformY + 1, 7, BlockType::Stone);
    setBlockShapeWorld(15, platformY + 1, 7, BlockShape::CornerWedgeNorthEast);

    // Complementary prism: the inverse half of a cube, solid above a
    // diagonal bottom. Kept beside the wedge for silhouette comparison.
    setBlockWorld(13, platformY + 1, 7, BlockType::Stone);
    setBlockShapeWorld(13, platformY + 1, 7, BlockShape::CornerComplementSouthWest);

    // Rotated junction references. Each center cut should match the two
    // perpendicular ramps around it, proving all four orientations render.
    setBlockWorld(13, platformY + 1, 10, BlockType::Stone);
    setBlockShapeWorld(13, platformY + 1, 10, BlockShape::RampSouth);
    setBlockWorld(14, platformY + 1, 11, BlockType::Stone);
    setBlockShapeWorld(14, platformY + 1, 11, BlockShape::RampEast);
    setBlockWorld(14, platformY + 1, 10, BlockType::Stone);
    setBlockShapeWorld(14, platformY + 1, 10, BlockShape::CornerCutTopNorthWest);

    setBlockWorld(11, platformY + 1, 14, BlockType::Stone);
    setBlockShapeWorld(11, platformY + 1, 14, BlockShape::RampSouth);
    setBlockWorld(10, platformY + 1, 15, BlockType::Stone);
    setBlockShapeWorld(10, platformY + 1, 15, BlockShape::RampWest);
    setBlockWorld(10, platformY + 1, 14, BlockType::Stone);
    setBlockShapeWorld(10, platformY + 1, 14, BlockShape::CornerCutTopNorthEast);

    setBlockWorld(14, platformY + 1, 13, BlockType::Stone);
    setBlockShapeWorld(14, platformY + 1, 13, BlockShape::RampWest);
    setBlockWorld(15, platformY + 1, 14, BlockType::Stone);
    setBlockShapeWorld(15, platformY + 1, 14, BlockShape::RampNorth);
    setBlockWorld(14, platformY + 1, 14, BlockType::Stone);
    setBlockShapeWorld(14, platformY + 1, 14, BlockShape::CornerCutTopSouthEast);

    // Straight red -> yellow -> red connector test, directly in front of spawn.
    // The yellow complementary tetrahedron is rotated 180 degrees in its cell.
    constexpr int diagonalTestY = platformY + 4;

    setBlockWorld(5, diagonalTestY, 6, BlockType::Stone);
    setBlockShapeWorld(5, diagonalTestY, 6, BlockShape::CornerCutTopNorthEast);

    setBlockWorld(6, diagonalTestY, 6, BlockType::Stone);
    setBlockShapeWorld(6, diagonalTestY, 6, BlockShape::OpenBottomCornerComplementNorthEast);

    // Move the third red one cell left and one cell back from its previous
    // position so its diagonal cut can meet the yellow at the intended angle.
    // The first red and yellow reference blocks remain fixed.
    setBlockWorld(6, diagonalTestY, 5, BlockType::Stone);
    setBlockShapeWorld(6, diagonalTestY, 5, BlockShape::CornerCutTopNorthEast);

    // Same-height cube/ramp seams: the cube's top must meet the ramp's
    // high edge. RampWest is high on its west edge, so it joins the cube
    // immediately to its west without raising the ramp by an extra block.
    setBlockWorld(5, platformY + 1, 12, BlockType::Stone);
    setBlockWorld(6, platformY + 1, 12, BlockType::Stone);
    setBlockShapeWorld(6, platformY + 1, 12, BlockShape::RampWest);

    setBlockWorld(5, platformY + 1, 16, BlockType::Stone);
    setBlockWorld(6, platformY + 1, 16, BlockType::Stone);
    setBlockShapeWorld(6, platformY + 1, 16, BlockShape::RampWest);

    // Mirror the seam: the cube is east of the ramp's high east edge.
    setBlockWorld(11, platformY + 1, 16, BlockType::Stone);
    setBlockWorld(10, platformY + 1, 16, BlockType::Stone);
    setBlockShapeWorld(10, platformY + 1, 16, BlockShape::RampEast);
}

bool World::raycast(glm::vec3 origin, glm::vec3 dir, float maxDist, glm::ivec3& hitBlock, glm::ivec3& prevBlock) const {
    // Normalize direction
    dir = glm::normalize(dir);
    
    // Start cell
    glm::ivec3 currentCell = glm::floor(origin);
    
    // Step direction
    glm::ivec3 step;
    step.x = (dir.x > 0) ? 1 : (dir.x < 0) ? -1 : 0;
    step.y = (dir.y > 0) ? 1 : (dir.y < 0) ? -1 : 0;
    step.z = (dir.z > 0) ? 1 : (dir.z < 0) ? -1 : 0;
    
    // tDelta
    float tDeltaX, tDeltaY, tDeltaZ;
    
    if (dir.x != 0.0f) {
        tDeltaX = 1.0f / std::abs(dir.x);
    } else {
        tDeltaX = 1e30f; // Very large number
    }
    
    if (dir.y != 0.0f) {
        tDeltaY = 1.0f / std::abs(dir.y);
    } else {
        tDeltaY = 1e30f; // Very large number
    }
    
    if (dir.z != 0.0f) {
        tDeltaZ = 1.0f / std::abs(dir.z);
    } else {
        tDeltaZ = 1e30f; // Very large number
    }
    
    // tMax
    float tMaxX, tMaxY, tMaxZ;
    
    if (dir.x > 0) {
        tMaxX = (currentCell.x + 1.0f - origin.x) / dir.x;
    } else if (dir.x < 0) {
        tMaxX = (currentCell.x - origin.x) / dir.x;
    } else {
        tMaxX = 1e30f;
    }
    
    if (dir.y > 0) {
        tMaxY = (currentCell.y + 1.0f - origin.y) / dir.y;
    } else if (dir.y < 0) {
        tMaxY = (currentCell.y - origin.y) / dir.y;
    } else {
        tMaxY = 1e30f;
    }
    
    if (dir.z > 0) {
        tMaxZ = (currentCell.z + 1.0f - origin.z) / dir.z;
    } else if (dir.z < 0) {
        tMaxZ = (currentCell.z - origin.z) / dir.z;
    } else {
        tMaxZ = 1e30f;
    }
    
    // Loop until we exceed maxDist or hit a solid block
    float t = 0.0f;
    while (t < maxDist) {
        // Check if current cell is solid
        if (isSolid(currentCell.x, currentCell.y, currentCell.z)) {
            hitBlock = currentCell;
            return true;
        }
        
        // Record previous block
        prevBlock = currentCell;
        
        // Advance along the axis with smallest tMax
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            currentCell.x += step.x;
            t = tMaxX;
            tMaxX += tDeltaX;
        } else if (tMaxY < tMaxZ) {
            currentCell.y += step.y;
            t = tMaxY;
            tMaxY += tDeltaY;
        } else {
            currentCell.z += step.z;
            t = tMaxZ;
            tMaxZ += tDeltaZ;
        }
        
        // Check if we've exceeded maxDist
        if (t >= maxDist) {
            return false;
        }
    }
    
    return false;
}

void World::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    // Set the view and projection matrices
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    
    // Draw each chunk
    for (auto& pair : m_chunks) {
        int64_t packed = pair.first;
        Chunk* chunk = pair.second.get();
        
        // Get or build mesh for this chunk
        auto meshIt = m_meshes.find(packed);
        if (meshIt == m_meshes.end()) {
            // Build mesh with world lookups for both material and shape.
            auto getWorldBlock = [this](int wx, int wy, int wz) -> BlockType {
                return getBlockWorld(wx, wy, wz);
            };
            auto getWorldShape = [this](int wx, int wy, int wz) -> BlockShape {
                return getBlockShapeWorld(wx, wy, wz);
            };
            
            Mesh mesh = chunk->buildMesh(
                getWorldBlock, getWorldShape, m_shapeDebugMode,
                m_planetPrototype ? PLANET_SIZE : 0,
                m_planetPrototype ? m_planet.seed : 0);
            meshIt = m_meshes.emplace(packed, std::move(mesh)).first;
        }
        
        const glm::ivec3 chunkPosition = chunk->getPosition();
        const glm::vec3 basePos(
            static_cast<float>(chunkPosition.x * Chunk::SIZE_X),
            static_cast<float>(chunkPosition.y * Chunk::SIZE_Y),
            static_cast<float>(chunkPosition.z * Chunk::SIZE_Z));
        const int copyMin = m_planetPrototype ? -1 : 0;
        const int copyMax = m_planetPrototype ? 1 : 0;
        for (int copyX = copyMin; copyX <= copyMax; ++copyX) {
            for (int copyZ = copyMin; copyZ <= copyMax; ++copyZ) {
                const glm::vec3 drawPos = basePos + glm::vec3(
                    static_cast<float>(copyX * PLANET_SIZE), 0.0f,
                    static_cast<float>(copyZ * PLANET_SIZE));
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), drawPos);
                shader.setMat4("model", model);
                meshIt->second.draw();
            }
        }
    }
}

int64_t World::packCoordinates(glm::ivec3 coords) const {
    // Pack coordinates into a single 64-bit integer
    // Using 21 bits for each coordinate (allowing for range -1048576 to 1048575)
    return (static_cast<int64_t>(coords.x & 0x1FFFFF) << 42) |
           (static_cast<int64_t>(coords.y & 0x1FFFFF) << 21) |
           (static_cast<int64_t>(coords.z & 0x1FFFFF));
}

glm::ivec3 World::unpackCoordinates(int64_t packed) const {
    // Unpack the coordinates from a single 64-bit integer
    int x = static_cast<int>((packed >> 42) & 0x1FFFFF);
    int y = static_cast<int>((packed >> 21) & 0x1FFFFF);
    int z = static_cast<int>(packed & 0x1FFFFF);
    
    // Handle negative coordinates (two's complement)
    if (x >= 0x100000) x -= 0x200000;
    if (y >= 0x100000) y -= 0x200000;
    if (z >= 0x100000) z -= 0x200000;
    
    return glm::ivec3(x, y, z);
}
