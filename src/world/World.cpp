#include "world/World.h"
#include "render/Shader.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <vector>

World::World() = default;

World::~World() = default;

void World::generate(int radius) {
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

    // Detect connector candidates after all chunks have generated. Candidates
    // are resolved in two phases so a later junction cannot overwrite part of
    // an earlier red-yellow-red pattern. Three-cell patterns win over partial
    // two-cell patterns; repeated non-overlapping patterns can form long runs.
    const int minWorld = -radius * Chunk::SIZE_X;
    const int maxWorld = (radius + 1) * Chunk::SIZE_X - 1;
    const auto surfaceYAt = [this](int wx, int wz) {
        for (int y = Chunk::SIZE_Y - 2; y >= 1; --y) {
            if (getBlockWorld(wx, y, wz) == BlockType::Grass &&
                getBlockWorld(wx, y + 1, wz) == BlockType::Air) {
                return y;
            }
        }
        return -1;
    };

    struct ConnectorCell {
        int x;
        int y;
        int z;
        BlockShape shape;
    };
    struct ConnectorCandidate {
        ConnectorCell cells[3];
        int count;
        int orientation;
    };
    std::vector<ConnectorCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>((maxWorld - minWorld) * (maxWorld - minWorld) / 8));

    for (int wz = minWorld + 1; wz < maxWorld; ++wz) {
        for (int wx = minWorld + 1; wx < maxWorld; ++wx) {
            const int centerY = surfaceYAt(wx, wz);
            if (centerY < 1) continue;

            const BlockShape centerShape = getBlockShapeWorld(wx, centerY, wz);
            if (usesExplicitBlockShapeGeometry(centerShape)) continue;

            const int westY = surfaceYAt(wx - 1, wz);
            const int southY = surfaceYAt(wx, wz - 1);
            const int eastY = surfaceYAt(wx + 1, wz);
            const int northY = surfaceYAt(wx, wz + 1);

            const auto profileAt = [this](int x, int y, int z) {
                return getBlockShapeData(getBlockShapeWorld(x, y, z)).topHeights;
            };
            const auto west = profileAt(wx - 1, westY, wz);
            const auto south = profileAt(wx, southY, wz - 1);
            const auto east = profileAt(wx + 1, eastY, wz);
            const auto north = profileAt(wx, northY, wz + 1);

            // Each orientation is expressed as two independently testable
            // slope arms. If both arms meet at this cell, use all three pieces;
            // if only one arm is valid, use the matching two-piece partial.
            bool armA = false, armB = false;
            BlockShape redShape = BlockShape::CornerCutTopNorthEast;
            BlockShape yellowShape = BlockShape::OpenBottomCornerComplementNorthEast;
            int redAx = wx - 1, redAz = wz, redBx = wx, redBz = wz - 1;
            int orientation = 0;

            if (westY == centerY && southY == centerY &&
                west[2] > west[1] && south[2] > south[3]) {
                armA = armB = true;
                orientation = 0;
            } else if (westY == centerY && northY == centerY &&
                       west[1] > west[2] && north[1] > north[0]) {
                armA = armB = true;
                redShape = BlockShape::CornerCutTopSouthEast;
                yellowShape = BlockShape::OpenBottomCornerComplementSouthEast;
                redAx = wx - 1; redAz = wz; redBx = wx; redBz = wz + 1;
                orientation = 1;
            } else if (eastY == centerY && northY == centerY &&
                       east[0] > east[3] && north[0] > north[1]) {
                armA = armB = true;
                redShape = BlockShape::CornerCutTopSouthWest;
                yellowShape = BlockShape::OpenBottomCornerComplementSouthWest;
                redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz + 1;
                orientation = 2;
            } else if (eastY == centerY && southY == centerY &&
                       east[3] > east[0] && south[3] > south[2]) {
                armA = armB = true;
                redShape = BlockShape::CornerCutTopNorthWest;
                yellowShape = BlockShape::OpenBottomCornerComplementNorthWest;
                redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz - 1;
                orientation = 3;
            } else {
                // Partial patterns are considered only when a single arm
                // independently matches one of the four known orientations.
                if (westY == centerY && west[2] > west[1]) {
                    armA = true; orientation = 0;
                } else if (southY == centerY && south[2] > south[3]) {
                    armB = true; orientation = 0;
                } else if (westY == centerY && west[1] > west[2]) {
                    armA = true; orientation = 1;
                    redShape = BlockShape::CornerCutTopSouthEast;
                    yellowShape = BlockShape::OpenBottomCornerComplementSouthEast;
                    redAx = wx - 1; redAz = wz; redBx = wx; redBz = wz + 1;
                } else if (northY == centerY && north[1] > north[0]) {
                    armB = true; orientation = 1;
                    redShape = BlockShape::CornerCutTopSouthEast;
                    yellowShape = BlockShape::OpenBottomCornerComplementSouthEast;
                    redAx = wx - 1; redAz = wz; redBx = wx; redBz = wz + 1;
                } else if (eastY == centerY && east[0] > east[3]) {
                    armA = true; orientation = 2;
                    redShape = BlockShape::CornerCutTopSouthWest;
                    yellowShape = BlockShape::OpenBottomCornerComplementSouthWest;
                    redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz + 1;
                } else if (northY == centerY && north[0] > north[1]) {
                    armB = true; orientation = 2;
                    redShape = BlockShape::CornerCutTopSouthWest;
                    yellowShape = BlockShape::OpenBottomCornerComplementSouthWest;
                    redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz + 1;
                } else if (eastY == centerY && east[3] > east[0]) {
                    armA = true; orientation = 3;
                    redShape = BlockShape::CornerCutTopNorthWest;
                    yellowShape = BlockShape::OpenBottomCornerComplementNorthWest;
                    redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz - 1;
                } else if (southY == centerY && south[3] > south[2]) {
                    armB = true; orientation = 3;
                    redShape = BlockShape::CornerCutTopNorthWest;
                    yellowShape = BlockShape::OpenBottomCornerComplementNorthWest;
                    redAx = wx + 1; redAz = wz; redBx = wx; redBz = wz - 1;
                }
            }

            if (!armA && !armB) continue;
            ConnectorCandidate candidate{};
            candidate.orientation = orientation;
            candidate.cells[0] = {wx, centerY, wz, yellowShape};
            candidate.count = 1;
            if (armA) candidate.cells[candidate.count++] = {redAx, centerY, redAz, redShape};
            if (armB) candidate.cells[candidate.count++] = {redBx, centerY, redBz, redShape};
            candidates.push_back(candidate);
        }
    }

    // Longer complete patterns get first claim on their cells. This prevents
    // overlapping junctions from turning a yellow center red (or vice versa).
    std::stable_sort(candidates.begin(), candidates.end(),
        [](const ConnectorCandidate& a, const ConnectorCandidate& b) {
            if (a.count != b.count) return a.count > b.count;
            return a.orientation < b.orientation;
        });
    struct ReservedCell { int x; int y; int z; };
    std::vector<ReservedCell> reserved;
    reserved.reserve(candidates.size() * 2);
    const auto isReserved = [&reserved](int x, int y, int z) {
        return std::any_of(reserved.begin(), reserved.end(),
            [=](const ReservedCell& cell) {
                return cell.x == x && cell.y == y && cell.z == z;
            });
    };
    for (const ConnectorCandidate& candidate : candidates) {
        bool conflict = false;
        for (int i = 0; i < candidate.count; ++i) {
            const ConnectorCell& cell = candidate.cells[i];
            if (isReserved(cell.x, cell.y, cell.z) ||
                getBlockWorld(cell.x, cell.y, cell.z) != BlockType::Grass ||
                getBlockWorld(cell.x, cell.y + 1, cell.z) != BlockType::Air) {
                conflict = true;
                break;
            }
        }
        if (conflict) continue;
        for (int i = 0; i < candidate.count; ++i) {
            const ConnectorCell& cell = candidate.cells[i];
            setBlockShapeWorld(cell.x, cell.y, cell.z, cell.shape);
            reserved.push_back({cell.x, cell.y, cell.z});
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
    
    // Only consider chunks at y = 0 for terrain
    glm::ivec3 chunkPos(chunkX, 0, chunkZ);
    int64_t packed = packCoordinates(chunkPos);
    
    auto it = m_chunks.find(packed);
    if (it == m_chunks.end()) {
        return BlockType::Air;
    }
    
    Chunk* chunk = it->second.get();
    
    // Convert world coordinates to local coordinates
    int localX = wx - (chunkX * 16);
    int localZ = wz - (chunkZ * 16);
    
    // Check bounds for y coordinate
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return BlockType::Air;
    }
    
    // Return block at local coordinates
    return chunk->getBlock(localX, wy, localZ);
}

BlockShape World::getBlockShapeWorld(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return BlockShape::Cube;
    }

    const int chunkX = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    const int chunkZ = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    const glm::ivec3 chunkPos(chunkX, 0, chunkZ);
    const auto it = m_chunks.find(packCoordinates(chunkPos));
    if (it == m_chunks.end()) {
        return BlockShape::Cube;
    }

    const int localX = wx - chunkX * Chunk::SIZE_X;
    const int localZ = wz - chunkZ * Chunk::SIZE_Z;
    return it->second->getBlockShape(localX, wy, localZ);
}

BlockShape World::getShapeAt(int wx, int wy, int wz) const {
    return getBlockShapeWorld(wx, wy, wz);
}

bool World::getTerrainSurfaceHeight(float worldX, float worldZ, float& height) const {
    const int cellX = static_cast<int>(std::floor(worldX));
    const int cellZ = static_cast<int>(std::floor(worldZ));
    const float x = worldX - static_cast<float>(cellX);
    const float z = worldZ - static_cast<float>(cellZ);

    const auto smoothedHeight = [](int wx, int wz) {
        const auto rawHeight = [](int x, int z) {
            const float noise = glm::perlin(glm::vec2(
                static_cast<float>(x), static_cast<float>(z)) * 0.01f);
            return std::max(1, std::min(
                static_cast<int>(noise * 18.0f + 24.0f), Chunk::SIZE_Y - 3));
        };
        constexpr int weights[5] = {1, 2, 3, 2, 1};
        int sum = 0;
        int total = 0;
        for (int dz = -2; dz <= 2; ++dz) {
            for (int dx = -2; dx <= 2; ++dx) {
                const int weight = weights[dx + 2] * weights[dz + 2];
                sum += rawHeight(wx + dx, wz + dz) * weight;
                total += weight;
            }
        }
        return std::max(1, std::min(
            (sum + total / 2) / total, Chunk::SIZE_Y - 3));
    };
    const auto cornerHeight = [&](int cx, int cz) {
        int lowest = Chunk::SIZE_Y;
        for (int dz = cz - 1; dz <= cz; ++dz) {
            for (int dx = cx - 1; dx <= cx; ++dx) {
                lowest = std::min(lowest, smoothedHeight(dx, dz));
            }
        }
        return static_cast<float>(lowest + 1);
    };

    const float sw = cornerHeight(cellX, cellZ);
    const float se = cornerHeight(cellX + 1, cellZ);
    const float ne = cornerHeight(cellX + 1, cellZ + 1);
    const float nw = cornerHeight(cellX, cellZ + 1);

    // Match Chunk::buildMesh's diagonal choice for exceptional corners/saddles.
    const int swH = static_cast<int>(sw) - smoothedHeight(cellX, cellZ);
    const int seH = static_cast<int>(se) - smoothedHeight(cellX, cellZ);
    const int neH = static_cast<int>(ne) - smoothedHeight(cellX, cellZ);
    const int nwH = static_cast<int>(nw) - smoothedHeight(cellX, cellZ);
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
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return;
    }
    
    // Calculate chunk coordinates using floor division
    int cx = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    int cz = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    
    // Calculate local coordinates
    int lx = wx - cx * Chunk::SIZE_X;
    int lz = wz - cz * Chunk::SIZE_Z;
    
    // Get the chunk at this position
    glm::ivec3 chunkPos(cx, 0, cz);
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        return;
    }
    
    // Set the block in the chunk
    chunk->setBlock(lx, wy, lz, type);
    
    // Erase the mesh for this chunk so it gets rebuilt
    int64_t packed = packCoordinates(chunkPos);
    m_meshes.erase(packed);
    
    // Also erase meshes of adjacent chunks if we're at chunk boundaries
    if (lx == 0) {
        glm::ivec3 leftChunkPos(cx - 1, 0, cz);
        int64_t leftPacked = packCoordinates(leftChunkPos);
        m_meshes.erase(leftPacked);
    }
    
    if (lx == Chunk::SIZE_X - 1) {
        glm::ivec3 rightChunkPos(cx + 1, 0, cz);
        int64_t rightPacked = packCoordinates(rightChunkPos);
        m_meshes.erase(rightPacked);
    }
    
    if (lz == 0) {
        glm::ivec3 frontChunkPos(cx, 0, cz - 1);
        int64_t frontPacked = packCoordinates(frontChunkPos);
        m_meshes.erase(frontPacked);
    }
    
    if (lz == Chunk::SIZE_Z - 1) {
        glm::ivec3 backChunkPos(cx, 0, cz + 1);
        int64_t backPacked = packCoordinates(backChunkPos);
        m_meshes.erase(backPacked);
    }
}


void World::setBlockShapeWorld(int wx, int wy, int wz, BlockShape shape) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return;
    }

    const int cx = static_cast<int>(std::floor(static_cast<float>(wx) / Chunk::SIZE_X));
    const int cz = static_cast<int>(std::floor(static_cast<float>(wz) / Chunk::SIZE_Z));
    const int lx = wx - cx * Chunk::SIZE_X;
    const int lz = wz - cz * Chunk::SIZE_Z;
    const glm::ivec3 chunkPos(cx, 0, cz);
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr || chunk->getBlock(lx, wy, lz) == BlockType::Air) {
        return;
    }

    chunk->setBlockShape(lx, wy, lz, shape);
    m_meshes.erase(packCoordinates(chunkPos));
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
            
            Mesh mesh = chunk->buildMesh(getWorldBlock, getWorldShape);
            meshIt = m_meshes.emplace(packed, std::move(mesh)).first;
        }
        
        // Calculate model matrix based on chunk position
        glm::vec3 chunkPos = chunk->getPosition() * 16; // Assuming each chunk is 16x16x16 blocks
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkPos);
        
        // Set the model matrix
        shader.setMat4("model", model);
        
        // Draw the mesh
        meshIt->second.draw();
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
