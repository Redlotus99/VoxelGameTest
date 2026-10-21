#include "world/World.h"
#include "render/Shader.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

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

bool World::isSolid(int wx, int wy, int wz) const {
    return getBlockWorld(wx, wy, wz) != BlockType::Air;
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

    // Diagonal 45-degree chain test, placed near the shape-test spawn so it
    // is visible without a long walk: two red top-corner cuts caddy-cornered
    // with a yellow single-corner wedge between them.
    setBlockWorld(5, platformY + 1, 9, BlockType::Stone);
    setBlockShapeWorld(5, platformY + 1, 9, BlockShape::CornerCutTopSouthWest);
    setBlockWorld(6, platformY + 1, 10, BlockType::Stone);
    setBlockShapeWorld(6, platformY + 1, 10, BlockShape::CornerWedgeNorthEast);
    setBlockWorld(7, platformY + 1, 11, BlockType::Stone);
    setBlockShapeWorld(7, platformY + 1, 11, BlockShape::CornerCutTopNorthEast);

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
