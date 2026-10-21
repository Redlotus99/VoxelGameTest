#include "world/Chunk.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

Chunk::Chunk() : m_position(0, 0, 0) {
    // Fill with Air and default every cell to a cube shape.
    for (int i = 0; i < SIZE_X * SIZE_Y * SIZE_Z; ++i) {
        blocks[i] = BlockType::Air;
        shapes[i] = BlockShape::Cube;
    }
}

Chunk::Chunk(glm::ivec3 pos) : m_position(pos) {
    // Fill with Air and default every cell to a cube shape.
    for (int i = 0; i < SIZE_X * SIZE_Y * SIZE_Z; ++i) {
        blocks[i] = BlockType::Air;
        shapes[i] = BlockShape::Cube;
    }
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) {
        return BlockType::Air;
    }
    return blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType t) {
    if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) {
        return;
    }

    const int cellIndex = index(x, y, z);
    blocks[cellIndex] = t;
    if (t == BlockType::Air) {
        shapes[cellIndex] = BlockShape::Cube;
    }
}

BlockShape Chunk::getBlockShape(int x, int y, int z) const {
    if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) {
        return BlockShape::Cube;
    }
    return shapes[index(x, y, z)];
}

void Chunk::setBlockShape(int x, int y, int z, BlockShape shape) {
    if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) {
        return;
    }

    const int cellIndex = index(x, y, z);
    if (blocks[cellIndex] != BlockType::Air) {
        shapes[cellIndex] = shape;
    }
}

void Chunk::generateFlat() {
    // Generate flat terrain
    for (int z = 0; z < SIZE_Z; ++z) {
        for (int x = 0; x < SIZE_X; ++x) {
            // Stone layer (y from 0 to 7)
            for (int y = 0; y < 8; ++y) {
                setBlock(x, y, z, BlockType::Stone);
            }

            // Dirt layer (y from 8 to 10)
            for (int y = 8; y < 11; ++y) {
                setBlock(x, y, z, BlockType::Dirt);
            }

            // Grass layer (y == 11)
            setBlock(x, 11, z, BlockType::Grass);

            // Air above grass (y from 12 to SIZE_Y-1)
            for (int y = 12; y < SIZE_Y; ++y) {
                setBlock(x, y, z, BlockType::Air);
            }
        }
    }
}

void Chunk::generateTerrain() {
    // Terrain is derived exclusively from world coordinates so every chunk
    // computes identical heights along shared boundaries.
    const auto rawHeight = [](int worldX, int worldZ) -> int {
        const float noise = glm::perlin(glm::vec2(
            static_cast<float>(worldX), static_cast<float>(worldZ)) * 0.02f);
        return std::max(1, std::min(static_cast<int>(noise * 14.0f + 24.0f), SIZE_Y - 3));
    };
    const auto terrainHeight = [&](int worldX, int worldZ) -> int {
        int height = SIZE_Y;
        for (int dz = -2; dz <= 2; ++dz) {
            for (int dx = -2; dx <= 2; ++dx) {
                height = std::min(height, rawHeight(worldX + dx, worldZ + dz) +
                    std::max(std::abs(dx), std::abs(dz)));
            }
        }
        return std::max(1, std::min(height, SIZE_Y - 3));
    };
    constexpr int haloSizeX = SIZE_X + 2;
    constexpr int haloSizeZ = SIZE_Z + 2;
    std::array<int, haloSizeX * haloSizeZ> heights{};
    const auto cacheIndex = [=](int x, int z) { return (x + 1) + haloSizeX * (z + 1); };
    for (int z = -1; z <= SIZE_Z; ++z) {
        for (int x = -1; x <= SIZE_X; ++x) {
            heights[cacheIndex(x, z)] = terrainHeight(
                m_position.x * SIZE_X + x, m_position.z * SIZE_Z + z);
        }
    }
    for (int z = 0; z < SIZE_Z; ++z) {
        for (int x = 0; x < SIZE_X; ++x) {
            const int surfaceY = heights[cacheIndex(x, z)];
            for (int y = 0; y < SIZE_Y; ++y) {
                if (y > surfaceY) setBlock(x, y, z, BlockType::Air);
                else if (y == surfaceY) {
                    setBlock(x, y, z, BlockType::Grass);
                    setBlockShape(x, y, z, BlockShape::TerrainSurface);
                } else if (y >= surfaceY - 3) setBlock(x, y, z, BlockType::Dirt);
                else setBlock(x, y, z, BlockType::Stone);
            }
        }
    }
}
Mesh Chunk::buildMesh(
    const std::function<BlockType(int wx, int wy, int wz)>& getWorldBlock,
    const std::function<BlockShape(int wx, int wy, int wz)>& getWorldShape) const {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    auto getBlockColor = [](BlockType type) -> glm::vec3 {
        switch (type) {
            case BlockType::Grass: return glm::vec3(0.3f, 0.7f, 0.2f);
            case BlockType::Dirt: return glm::vec3(0.5f, 0.3f, 0.15f);
            case BlockType::Stone: return glm::vec3(0.5f, 0.5f, 0.5f);
            default: return glm::vec3(0.0f, 0.0f, 0.0f);
        }
    };

    // Temporary diagnostic mode: color geometry by shape instead of material.
    // Disable this switch to restore normal grass/dirt/stone coloring.
    constexpr bool kShapeDebugTintEnabled = true;
    auto getShapeDebugColor = [](BlockShape shape) -> glm::vec3 {
        switch (shape) {
            case BlockShape::Cube: return glm::vec3(0.55f, 0.55f, 0.55f); // neutral gray
            // Directional ramps: blue palette.
            case BlockShape::RampNorth: return glm::vec3(0.10f, 0.45f, 1.00f);
            case BlockShape::RampSouth: return glm::vec3(0.05f, 0.20f, 0.75f);
            case BlockShape::RampEast: return glm::vec3(0.00f, 0.85f, 1.00f);
            case BlockShape::RampWest: return glm::vec3(0.05f, 0.55f, 0.70f);
            // Corner ramps: purple palette.
            case BlockShape::CornerRampSouthWest: return glm::vec3(0.85f, 0.20f, 1.00f);
            case BlockShape::CornerRampSouthEast: return glm::vec3(0.60f, 0.05f, 0.85f);
            case BlockShape::CornerRampNorthEast: return glm::vec3(1.00f, 0.30f, 0.90f);
            case BlockShape::CornerRampNorthWest: return glm::vec3(0.45f, 0.10f, 0.65f);
            // Corner wedges: orange palette.
            case BlockShape::CornerWedgeSouthWest: return glm::vec3(1.00f, 0.45f, 0.00f);
            case BlockShape::CornerWedgeSouthEast: return glm::vec3(0.90f, 0.20f, 0.00f);
            case BlockShape::CornerWedgeNorthEast: return glm::vec3(1.00f, 0.75f, 0.05f);
            case BlockShape::CornerWedgeNorthWest: return glm::vec3(0.75f, 0.30f, 0.02f);
            // Complementary corner prisms: amber palette.
            case BlockShape::CornerComplementSouthWest: return glm::vec3(1.00f, 0.55f, 0.10f);
            case BlockShape::CornerComplementSouthEast: return glm::vec3(0.95f, 0.40f, 0.05f);
            case BlockShape::CornerComplementNorthEast: return glm::vec3(1.00f, 0.70f, 0.15f);
            case BlockShape::CornerComplementNorthWest: return glm::vec3(0.80f, 0.35f, 0.05f);
            // Saddles: green palette.
            case BlockShape::SaddleSouthWestNorthEast: return glm::vec3(0.10f, 1.00f, 0.20f);
            case BlockShape::SaddleSouthEastNorthWest: return glm::vec3(0.00f, 0.60f, 0.20f);
            // Natural terrain remains grass green.
            case BlockShape::TerrainSurface: return glm::vec3(0.25f, 0.80f, 0.35f);
            // Corner cuts: red/pink palette.
            case BlockShape::CornerCutSouthWest: return glm::vec3(1.00f, 0.05f, 0.05f);
            case BlockShape::CornerCutTopSouthWest: return glm::vec3(1.00f, 0.05f, 0.55f);
            case BlockShape::CornerCutTopNorthEast: return glm::vec3(0.75f, 0.00f, 0.30f);
            case BlockShape::CornerCutTopNorthWest: return glm::vec3(1.00f, 0.25f, 0.35f);
            case BlockShape::CornerCutTopSouthEast: return glm::vec3(0.85f, 0.10f, 0.65f);
            case BlockShape::Count: break;
        }
        return glm::vec3(1.0f, 0.0f, 1.0f); // bright magenta for an invalid shape
    };

    auto getFaceBrightness = [](int face) -> float {
        switch (face) {
            case 0: return 1.0f;  // Top (+Y)
            case 1: return 0.5f;  // Bottom (-Y)
            case 2: return 0.8f;  // +X
            case 3: return 0.8f;  // -X
            case 4: return 0.65f; // +Z
            case 5: return 0.65f; // -Z
            case 6: return 1.0f;  // Diagonal slope, match ramp top brightness
            default: return 0.9f;
        }
    };

    const float faceCorners[6][4][3] = {
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},  // 0 +Y
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},  // 1 -Y
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},  // 2 +X
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},  // 3 -X
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},  // 4 +Z
        {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}   // 5 -Z
    };

    auto emitTriangle = [&](const glm::vec3& a, const glm::vec3& b,
                            const glm::vec3& c, const glm::vec3& color) {
        const uint32_t base = static_cast<uint32_t>(vertices.size() / 6);
        const glm::vec3 points[3] = {a, b, c};
        for (const glm::vec3& p : points) {
            vertices.push_back(p.x);
            vertices.push_back(p.y);
            vertices.push_back(p.z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
        indices.insert(indices.end(), {base, base + 1, base + 2});
    };

    auto emitQuad = [&](const glm::vec3 corners[4], const glm::vec3& color) {
        emitTriangle(corners[0], corners[1], corners[2], color);
        emitTriangle(corners[0], corners[2], corners[3], color);
    };

    const auto profileCornerIndex = [](float px, float pz) {
        return (pz < 0.5f)
            ? ((px < 0.5f) ? 0 : 1)
            : ((px < 0.5f) ? 3 : 2);
    };

    // Explicit top-corner cuts have the same edge-height profile as a corner
    // ramp: the removed top corner is low and the other three are high.
    const auto edgeTopHeight = [&](BlockShape edgeShape, float localX, float localZ) {
        if (edgeShape == BlockShape::CornerCutTopSouthWest && localX < 0.5f && localZ < 0.5f)
            return 0.0f;
        if (edgeShape == BlockShape::CornerCutTopSouthEast && localX >= 0.5f && localZ < 0.5f)
            return 0.0f;
        if (edgeShape == BlockShape::CornerCutTopNorthEast && localX >= 0.5f && localZ >= 0.5f)
            return 0.0f;
        if (edgeShape == BlockShape::CornerCutTopNorthWest && localX < 0.5f && localZ >= 0.5f)
            return 0.0f;
        return getBlockShapeData(edgeShape).topHeights[
            profileCornerIndex(localX, localZ)];
    };

    // Recompute the same world-space terrain profile used by generation.
    // These helpers are also used when clipping ordinary block faces against
    // a neighboring TerrainSurface, whose edge is not a full cube edge.
    const auto terrainHeightAt = [](int worldX, int worldZ) -> int {
        const auto rawHeight = [](int x, int z) {
            const float noise = glm::perlin(glm::vec2(
                static_cast<float>(x), static_cast<float>(z)) * 0.02f);
            return std::max(1, std::min(
                static_cast<int>(noise * 14.0f + 24.0f), SIZE_Y - 3));
        };
        int height = SIZE_Y;
        for (int dz = -2; dz <= 2; ++dz) {
            for (int dx = -2; dx <= 2; ++dx) {
                height = std::min(height, rawHeight(worldX + dx, worldZ + dz) +
                    std::max(std::abs(dx), std::abs(dz)));
            }
        }
        return std::max(1, std::min(height, SIZE_Y - 3));
    };
    const auto terrainCornerWorldY = [&](int worldX, int worldZ, int cornerX, int cornerZ) {
        int level = SIZE_Y;
        for (int dz = cornerZ - 1; dz <= cornerZ; ++dz) {
            for (int dx = cornerX - 1; dx <= cornerX; ++dx) {
                level = std::min(level, terrainHeightAt(worldX + dx, worldZ + dz));
            }
        }
        return level + 1;
    };

    for (int z = 0; z < SIZE_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
            for (int x = 0; x < SIZE_X; ++x) {
                const BlockType blockType = getBlock(x, y, z);
                if (blockType == BlockType::Air) {
                    continue;
                }

                BlockShape shape = getBlockShape(x, y, z);
                const glm::vec3 baseColor = kShapeDebugTintEnabled
                    ? getShapeDebugColor(shape)
                    : getBlockColor(blockType);

                // Natural terrain uses a shared-corner top surface. A block
                // above it intentionally flattens this cell to a normal cube.
                if (shape == BlockShape::TerrainSurface) {
                    const int worldX = m_position.x * SIZE_X + x;
                    const int worldZ = m_position.z * SIZE_Z + z;
                    if (getWorldBlock(worldX, y + 1, worldZ) != BlockType::Air) {
                        shape = BlockShape::Cube;
                    } else {
                        const auto offsetAt = [&](int wx, int wz, int cx, int cz) {
                            return terrainCornerWorldY(wx, wz, cx, cz) - terrainHeightAt(wx, wz);
                        };
                        const glm::vec3 sw(x, y + offsetAt(worldX, worldZ, 0, 0), z);
                        const glm::vec3 se(x + 1, y + offsetAt(worldX, worldZ, 1, 0), z);
                        const glm::vec3 ne(x + 1, y + offsetAt(worldX, worldZ, 1, 1), z + 1);
                        const glm::vec3 nw(x, y + offsetAt(worldX, worldZ, 0, 1), z + 1);
                        const int d1 = std::abs(static_cast<int>(sw.y - ne.y));
                        const int d2 = std::abs(static_cast<int>(se.y - nw.y));

                        // Terrain cells share one mesh type, so tint each top
                        // by its actual four-corner profile. This brings the
                        // shape diagnostics onto the generated terrain itself.
                        const int swH = static_cast<int>(sw.y) - y;
                        const int seH = static_cast<int>(se.y) - y;
                        const int neH = static_cast<int>(ne.y) - y;
                        const int nwH = static_cast<int>(nw.y) - y;
                        const int lowH = std::min(std::min(swH, seH), std::min(neH, nwH));
                        const int a = swH - lowH;
                        const int b = seH - lowH;
                        const int c = neH - lowH;
                        const int d = nwH - lowH;
                        BlockShape profile = BlockShape::TerrainSurface;
                        if      (a == 0 && b == 0 && c > 0 && d > 0) profile = BlockShape::RampNorth;
                        else if (a > 0 && b > 0 && c == 0 && d == 0) profile = BlockShape::RampSouth;
                        else if (a == 0 && b > 0 && c > 0 && d == 0) profile = BlockShape::RampEast;
                        else if (a > 0 && b == 0 && c == 0 && d > 0) profile = BlockShape::RampWest;
                        else if (a == 0 && b > 0 && c > 0 && d > 0) profile = BlockShape::CornerRampSouthWest;
                        else if (a > 0 && b == 0 && c > 0 && d > 0) profile = BlockShape::CornerRampSouthEast;
                        else if (a > 0 && b > 0 && c == 0 && d > 0) profile = BlockShape::CornerRampNorthEast;
                        else if (a > 0 && b > 0 && c > 0 && d == 0) profile = BlockShape::CornerRampNorthWest;
                        else if (a > 0 && b == 0 && c == 0 && d == 0) profile = BlockShape::CornerWedgeSouthWest;
                        else if (a == 0 && b > 0 && c == 0 && d == 0) profile = BlockShape::CornerWedgeSouthEast;
                        else if (a == 0 && b == 0 && c > 0 && d == 0) profile = BlockShape::CornerWedgeNorthEast;
                        else if (a == 0 && b == 0 && c == 0 && d > 0) profile = BlockShape::CornerWedgeNorthWest;
                        else if (a > 0 && b == 0 && c > 0 && d == 0) profile = BlockShape::SaddleSouthWestNorthEast;
                        else if (a == 0 && b > 0 && c == 0 && d > 0) profile = BlockShape::SaddleSouthEastNorthWest;
                        const glm::vec3 terrainColor = kShapeDebugTintEnabled
                            ? getShapeDebugColor(profile)
                            : getBlockColor(blockType);
                        const glm::vec3 topColor = terrainColor * getFaceBrightness(0);
                        if (d1 <= d2) {
                            emitTriangle(sw, nw, ne, topColor);
                            emitTriangle(sw, ne, se, topColor);
                        } else {
                            emitTriangle(sw, nw, se, topColor);
                            emitTriangle(nw, ne, se, topColor);
                        }
                        continue;
                    }
                }

                // Complementary corner prisms are solid above a diagonal
                // bottom. Their top is full-height; their bottom and side
                // boundaries come from the shared CellShape profile.
                if (shape == BlockShape::CornerComplementSouthWest ||
                    shape == BlockShape::CornerComplementSouthEast ||
                    shape == BlockShape::CornerComplementNorthEast ||
                    shape == BlockShape::CornerComplementNorthWest) {
                    const CellShape cell = getCellShapeData(shape);
                    const auto heightAt = [&](float px, float pz, bool top) {
                        const int ci = profileCornerIndex(px, pz);
                        return top ? cell.top[ci] : cell.bottom[ci];
                    };
                    const auto pointAt = [&](const float corner[3], bool top) {
                        return glm::vec3(x + corner[0],
                            y + heightAt(corner[0], corner[2], top),
                            z + corner[2]);
                    };
                    // The visible faces are emitted independently here; the
                    // shared-edge subtraction pass will replace this simple
                    // neighbor culling once all profile shapes use it.
                    for (int face = 0; face < 6; ++face) {
                        int nx=x, ny=y, nz=z;
                        switch (face) {
                            case 0: ++ny; break; case 1: --ny; break;
                            case 2: ++nx; break; case 3: --nx; break;
                            case 4: ++nz; break; case 5: --nz; break;
                        }
                        BlockType neighbor = getWorldBlock(
                            m_position.x * SIZE_X + nx, ny, m_position.z * SIZE_Z + nz);
                        if (neighbor != BlockType::Air) continue;
                        glm::vec3 p[4];
                        for (int i=0;i<4;++i) p[i]=pointAt(faceCorners[face][i], face != 1);
                        const glm::vec3 color = baseColor * getFaceBrightness(face);
                        if (face == 0) {
                            emitQuad(p, color);
                        } else if (face == 1) {
                            // Split the sloped bottom into two planar triangles.
                            emitTriangle(p[0], p[1], p[2], color);
                            emitTriangle(p[0], p[2], p[3], color);
                        } else {
                            const auto area2 = [](const glm::vec3& a, const glm::vec3& b,
                                                  const glm::vec3& d) {
                                return glm::length(glm::cross(b-a, d-a));
                            };
                            if (area2(p[0],p[1],p[2]) > 0.0001f)
                                emitTriangle(p[0],p[1],p[2],color);
                            if (area2(p[0],p[2],p[3]) > 0.0001f)
                                emitTriangle(p[0],p[2],p[3],color);
                        }
                    }
                    continue;
                }

                // All four top-corner cuts share one mesh template. The removed
                // top corner determines which axis faces become triangles and
                // the orientation of the diagonal cut surface.
                if (shape == BlockShape::CornerCutTopSouthWest ||
                    shape == BlockShape::CornerCutTopNorthEast ||
                    shape == BlockShape::CornerCutTopNorthWest ||
                    shape == BlockShape::CornerCutTopSouthEast) {
                    float removedX = 0.0f;
                    float removedZ = 0.0f;
                    switch (shape) {
                        case BlockShape::CornerCutTopSouthWest:
                            removedX = 0.0f; removedZ = 0.0f; break;
                        case BlockShape::CornerCutTopNorthEast:
                            removedX = 1.0f; removedZ = 1.0f; break;
                        case BlockShape::CornerCutTopNorthWest:
                            removedX = 0.0f; removedZ = 1.0f; break;
                        case BlockShape::CornerCutTopSouthEast:
                            removedX = 1.0f; removedZ = 0.0f; break;
                        default: break;
                    }

                    const glm::vec3 removedTop(
                        x + removedX, y + 1.0f, z + removedZ);

                    for (int face = 0; face < 6; ++face) {
                        int nx = x, ny = y, nz = z;
                        switch (face) {
                            case 0: ++ny; break;
                            case 1: --ny; break;
                            case 2: ++nx; break;
                            case 3: --nx; break;
                            case 4: ++nz; break;
                            case 5: --nz; break;
                        }

                        BlockType neighbor = BlockType::Air;
                        if (nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                            nz >= 0 && nz < SIZE_Z) {
                            neighbor = getBlock(nx, ny, nz);
                        } else {
                            neighbor = getWorldBlock(
                                m_position.x * SIZE_X + nx, ny,
                                m_position.z * SIZE_Z + nz);
                        }

                        const BlockShape neighborShape = neighbor == BlockType::Air
                            ? BlockShape::Cube
                            : (nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                               nz >= 0 && nz < SIZE_Z)
                                ? getBlockShape(nx, ny, nz)
                                : getWorldShape(
                                    m_position.x * SIZE_X + nx, ny,
                                    m_position.z * SIZE_Z + nz);
                        if (neighbor != BlockType::Air &&
                            (neighborShape == BlockShape::Cube ||
                             usesExplicitBlockShapeGeometry(neighborShape))) {
                            continue;
                        }

                        glm::vec3 corners[4];
                        int removedIndex = -1;
                        for (int i = 0; i < 4; ++i) {
                            corners[i] = glm::vec3(
                                x + faceCorners[face][i][0],
                                y + faceCorners[face][i][1],
                                z + faceCorners[face][i][2]);
                            if (glm::distance(corners[i], removedTop) < 0.01f) {
                                removedIndex = i;
                            }
                        }
                        const glm::vec3 color = baseColor * getFaceBrightness(face);

                        if (removedIndex >= 0) {
                            glm::vec3 remaining[3];
                            int next = 0;
                            for (int i = 0; i < 4; ++i) {
                                if (i != removedIndex) {
                                    remaining[next++] = corners[i];
                                }
                            }
                            emitTriangle(remaining[0], remaining[1], remaining[2], color);
                        } else {
                            emitQuad(corners, color);
                        }
                    }

                    const glm::vec3 cutBottom(x + removedX, y, z + removedZ);
                    const glm::vec3 cutTopX(x + (1.0f - removedX),
                                            y + 1.0f, z + removedZ);
                    const glm::vec3 cutTopZ(x + removedX,
                                            y + 1.0f, z + (1.0f - removedZ));
                    glm::vec3 cutA = cutBottom;
                    glm::vec3 cutB = cutTopX;
                    glm::vec3 cutC = cutTopZ;
                    const glm::vec3 cutCenter = (cutA + cutB + cutC) / 3.0f;
                    const glm::vec3 normal = glm::cross(cutB - cutA, cutC - cutA);
                    if (glm::dot(normal, removedTop - cutCenter) < 0.0f) {
                        std::swap(cutB, cutC);
                    }
                    emitTriangle(cutA, cutB, cutC,
                                 baseColor * getFaceBrightness(6));
                    continue;
                }

                // Explicit mesh template for a cube with its bottom southwest
                // corner removed by the plane x + y + z = 1. The three cut
                // boundary faces are triangles; the opposite three remain
                // full quads. Shared faces against occupied cells are omitted.
                if (shape == BlockShape::CornerCutSouthWest) {
                    for (int face = 0; face < 6; ++face) {
                        int nx = x, ny = y, nz = z;
                        switch (face) {
                            case 0: ++ny; break;
                            case 1: --ny; break;
                            case 2: ++nx; break;
                            case 3: --nx; break;
                            case 4: ++nz; break;
                            case 5: --nz; break;
                        }

                        BlockType neighbor = BlockType::Air;
                        if (nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                            nz >= 0 && nz < SIZE_Z) {
                            neighbor = getBlock(nx, ny, nz);
                        } else {
                            neighbor = getWorldBlock(
                                m_position.x * SIZE_X + nx, ny,
                                m_position.z * SIZE_Z + nz);
                        }
                        // A full cube completely covers this cell face, but a
                        // ramp only covers part of it. Keep the cut-shape face
                        // when adjacent to a non-cube so the slope transition
                        // does not leave a visible gap.
                        const BlockShape neighborShape = neighbor == BlockType::Air
                            ? BlockShape::Cube
                            : (nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                               nz >= 0 && nz < SIZE_Z)
                                ? getBlockShape(nx, ny, nz)
                                : getWorldShape(
                                    m_position.x * SIZE_X + nx, ny,
                                    m_position.z * SIZE_Z + nz);
                        if (neighbor != BlockType::Air &&
                            (neighborShape == BlockShape::Cube ||
                             usesExplicitBlockShapeGeometry(neighborShape))) {
                            continue;
                        }

                        glm::vec3 corners[4];
                        for (int i = 0; i < 4; ++i) {
                            corners[i] = glm::vec3(
                                x + faceCorners[face][i][0],
                                y + faceCorners[face][i][1],
                                z + faceCorners[face][i][2]);
                        }
                        const glm::vec3 color = baseColor * getFaceBrightness(face);

                        // Faces touching the removed corner become triangles.
                        if (face == 1) { // bottom (-Y), x + z >= 1
                            emitTriangle(corners[1], corners[2], corners[3], color);
                        } else if (face == 3) { // -X, y + z >= 1
                            emitTriangle(corners[1], corners[2], corners[3], color);
                        } else if (face == 5) { // -Z, x + y >= 1
                            emitTriangle(corners[0], corners[2], corners[3], color);
                        } else {
                            emitQuad(corners, color);
                        }
                    }

                    // Interior diagonal surface faces the removed corner.
                    const glm::vec3 cutA(x + 1.0f, y, z);
                    const glm::vec3 cutB(x, y + 1.0f, z);
                    const glm::vec3 cutC(x, y, z + 1.0f);
                    emitTriangle(cutA, cutC, cutB, baseColor * getFaceBrightness(6));
                    continue;
                }

                const BlockShapeData shapeData = getBlockShapeData(shape);
                for (int face = 0; face < 6; ++face) {
                    int nx = x, ny = y, nz = z;
                    switch (face) {
                        case 0: ++ny; break;
                        case 1: --ny; break;
                        case 2: ++nx; break;
                        case 3: --nx; break;
                        case 4: ++nz; break;
                        case 5: --nz; break;
                    }

                    BlockType neighborBlock = BlockType::Air;
                    BlockShape neighborShape = BlockShape::Cube;
                    const bool neighborIsLocal =
                        nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                        nz >= 0 && nz < SIZE_Z;
                    if (neighborIsLocal) {
                        neighborBlock = getBlock(nx, ny, nz);
                        neighborShape = getBlockShape(nx, ny, nz);
                    } else {
                        const int worldX = m_position.x * SIZE_X + nx;
                        const int worldY = ny;
                        const int worldZ = m_position.z * SIZE_Z + nz;
                        neighborBlock = getWorldBlock(worldX, worldY, worldZ);
                        if (neighborBlock != BlockType::Air) {
                            neighborShape = getWorldShape(worldX, worldY, worldZ);
                        }
                    }

                    // Clip vertical side faces against the neighboring shape's
                    // actual edge profile. A neighboring ramp may cover only part
                    // of this face; omitting the whole face leaves holes, while
                    // emitting the whole face creates protruding overlaps.
                    if (face >= 2 && face <= 5) {
                        const bool neighborAir = neighborBlock == BlockType::Air;
                        const bool xSide = face == 2 || face == 3;
                        const float currentX = face == 2 ? 1.0f : 0.0f;
                        const float currentZ = face == 4 ? 1.0f : 0.0f;
                        const float neighborX = face == 2 ? 0.0f : (face == 3 ? 1.0f : 0.0f);
                        const float neighborZ = face == 4 ? 0.0f : (face == 5 ? 1.0f : 0.0f);
                        const float currentLow = edgeTopHeight(
                            shape, xSide ? currentX : 0.0f, xSide ? 0.0f : currentZ);
                        const float currentHigh = edgeTopHeight(
                            shape, xSide ? currentX : 1.0f, xSide ? 1.0f : currentZ);
                        float neighborLow = neighborAir ? 0.0f : edgeTopHeight(
                            neighborShape, xSide ? neighborX : 0.0f,
                            xSide ? 0.0f : neighborZ);
                        float neighborHigh = neighborAir ? 0.0f : edgeTopHeight(
                            neighborShape, xSide ? neighborX : 1.0f,
                            xSide ? 1.0f : neighborZ);

                        // A natural terrain neighbor has world-space corner
                        // heights, not the unit-height profile of a cube. Use
                        // its true edge so a cube/ramp face cannot protrude
                        // through a 45-degree terrain edge.
                        if (!neighborAir && neighborShape == BlockShape::TerrainSurface) {
                            const int neighborWorldX = m_position.x * SIZE_X + nx;
                            const int neighborWorldZ = m_position.z * SIZE_Z + nz;
                            if (xSide) {
                                neighborLow = static_cast<float>(
                                    terrainCornerWorldY(neighborWorldX, neighborWorldZ,
                                        static_cast<int>(neighborX), 0) - y);
                                neighborHigh = static_cast<float>(
                                    terrainCornerWorldY(neighborWorldX, neighborWorldZ,
                                        static_cast<int>(neighborX), 1) - y);
                            } else {
                                neighborLow = static_cast<float>(
                                    terrainCornerWorldY(neighborWorldX, neighborWorldZ,
                                        0, static_cast<int>(neighborZ)) - y);
                                neighborHigh = static_cast<float>(
                                    terrainCornerWorldY(neighborWorldX, neighborWorldZ,
                                        1, static_cast<int>(neighborZ)) - y);
                            }
                        }

                        struct SidePoint { float t; float y; };
                        const auto neighborHeightAt = [&](float t) {
                            return neighborLow + (neighborHigh - neighborLow) * t;
                        };
                        const auto signedDistance = [&](const SidePoint& p) {
                            return p.y - neighborHeightAt(p.t);
                        };
                        std::vector<SidePoint> polygon = {
                            {0.0f, 0.0f}, {1.0f, 0.0f},
                            {1.0f, currentHigh}, {0.0f, currentLow}
                        };
                        std::vector<SidePoint> clipped;
                        clipped.reserve(6);
                        for (std::size_t i = 0; i < polygon.size(); ++i) {
                            const SidePoint a = polygon[i];
                            const SidePoint b = polygon[(i + 1) % polygon.size()];
                            const float da = signedDistance(a);
                            const float db = signedDistance(b);
                            const bool aOutside = da < -0.0001f;
                            const bool bOutside = db < -0.0001f;
                            if (!aOutside) clipped.push_back(a);
                            if (aOutside != bOutside) {
                                const float denominator = da - db;
                                if (std::abs(denominator) > 0.000001f) {
                                    const float amount = da / denominator;
                                    clipped.push_back({
                                        a.t + (b.t - a.t) * amount,
                                        a.y + (b.y - a.y) * amount
                                    });
                                }
                            }
                        }

                        const glm::vec3 sideColor = baseColor * getFaceBrightness(face);
                        const auto toWorld = [&](const SidePoint& p) {
                            if (face == 2) return glm::vec3(x + 1.0f, y + p.y, z + p.t);
                            if (face == 3) return glm::vec3(x, y + p.y, z + p.t);
                            if (face == 4) return glm::vec3(x + p.t, y + p.y, z + 1.0f);
                            return glm::vec3(x + p.t, y + p.y, z);
                        };
                        if (clipped.size() >= 3) {
                            const glm::vec3 first = toWorld(clipped[0]);
                            for (std::size_t i = 1; i + 1 < clipped.size(); ++i) {
                                const glm::vec3 second = toWorld(clipped[i]);
                                const glm::vec3 third = toWorld(clipped[i + 1]);
                                // The parameterized side polygon winds inward on
                                // +X and -Z; reverse those faces to keep outward
                                // normals consistent with the other cube faces.
                                if (face == 2 || face == 5) {
                                    emitTriangle(first, third, second, sideColor);
                                } else {
                                    emitTriangle(first, second, third, sideColor);
                                }
                            }
                        }
                        continue;
                    }

                    // Top and bottom faces retain their existing shape-specific
                    // handling. Vertical sides are clipped above using the exact
                    // shared-edge height profiles rather than whole-face culling.
                    if (face == 0 && neighborBlock != BlockType::Air &&
                        neighborShape == BlockShape::CornerCutSouthWest) {
                        const glm::vec3 topSouthWest(x, y + 1.0f, z);
                        const glm::vec3 topNorthWest(x, y + 1.0f, z + 1.0f);
                        const glm::vec3 topSouthEast(x + 1.0f, y + 1.0f, z);
                        emitTriangle(topSouthWest, topNorthWest, topSouthEast,
                                     baseColor * getFaceBrightness(face));
                    } else if (face == 0 && neighborBlock != BlockType::Air &&
                               neighborShape == BlockShape::CornerCutTopSouthWest) {
                        glm::vec3 topCorners[4];
                        for (int i = 0; i < 4; ++i) {
                            topCorners[i] = glm::vec3(
                                x + faceCorners[0][i][0],
                                y + faceCorners[0][i][1],
                                z + faceCorners[0][i][2]);
                        }
                        emitQuad(topCorners, baseColor * getFaceBrightness(face));
                    }

                    if (neighborBlock == BlockType::Air) {
                        const float brightness = getFaceBrightness(face);
                        const uint32_t base = static_cast<uint32_t>(vertices.size() / 6);
                        for (int i = 0; i < 4; ++i) {
                            const float cornerX = faceCorners[face][i][0];
                            const float cornerY = faceCorners[face][i][1];
                            const float cornerZ = faceCorners[face][i][2];
                            float shapedY = cornerY;
                            if (cornerY > 0.0f) {
                                const auto profileCornerIndex = [](float px, float pz) {
                                    return (pz < 0.5f)
                                        ? ((px < 0.5f) ? 0 : 1)
                                        : ((px < 0.5f) ? 3 : 2);
                                };
                                shapedY = shapeData.topHeights[
                                    profileCornerIndex(cornerX, cornerZ)];
                            }
                            vertices.push_back(x + cornerX);
                            vertices.push_back(y + shapedY);
                            vertices.push_back(z + cornerZ);
                            const glm::vec3 color = baseColor * brightness;
                            vertices.push_back(color.r);
                            vertices.push_back(color.g);
                            vertices.push_back(color.b);
                        }

                        if (face == 0 &&
                            (shape == BlockShape::CornerRampSouthWest ||
                             shape == BlockShape::CornerRampSouthEast ||
                             shape == BlockShape::CornerRampNorthEast ||
                             shape == BlockShape::CornerRampNorthWest ||
                             shape == BlockShape::CornerWedgeSouthWest ||
                             shape == BlockShape::CornerWedgeSouthEast ||
                             shape == BlockShape::CornerWedgeNorthEast ||
                             shape == BlockShape::CornerWedgeNorthWest)) {
                            // Corner ramps/wedges have one exceptional corner.
                            // Split the top along the diagonal that joins that
                            // corner to its opposite; the default diagonal can
                            // leave a flat triangle beside a sloping triangle,
                            // which reads as a jutting 45-degree edge.
                            const auto& h = shapeData.topHeights;
                            const bool northWestExceptional =
                                (h[3] != h[0] && h[3] != h[1] && h[3] != h[2]);
                            const bool southEastExceptional =
                                (h[1] != h[0] && h[1] != h[2] && h[1] != h[3]);
                            const bool useDiagonal02 = northWestExceptional || southEastExceptional;
                            if (useDiagonal02) {
                                indices.insert(indices.end(), {
                                    base, base + 1, base + 2,
                                    base, base + 2, base + 3
                                });
                            } else {
                                indices.insert(indices.end(), {
                                    base, base + 1, base + 3,
                                    base + 1, base + 2, base + 3
                                });
                            }
                        } else {
                            indices.insert(indices.end(), {
                                base, base + 1, base + 2,
                                base, base + 2, base + 3
                            });
                        }
                    }
                }
            }
        }
    }

    Mesh mesh;
    mesh.upload(vertices, indices);
    return mesh;
}
