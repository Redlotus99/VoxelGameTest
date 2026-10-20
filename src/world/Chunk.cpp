#include "world/Chunk.h"
#include <algorithm>
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
    // Height and surface-shape sampling are deterministic in world space, so
    // a junction can inspect neighbors even when they belong to another chunk.
    const auto sampleHeight = [](int worldX, int worldZ) -> int {
        const float noise = glm::perlin(glm::vec2(worldX, worldZ) * 0.02f);
        int height = 24 + static_cast<int>(noise * 14.0f);
        return std::max(1, std::min(height, SIZE_Y - 2));
    };

    const auto sampleSurfaceShape = [&](int worldX, int worldZ) -> BlockShape {
        const int height = sampleHeight(worldX, worldZ);
        const int northHeight = sampleHeight(worldX, worldZ + 1);
        const int southHeight = sampleHeight(worldX, worldZ - 1);
        const int eastHeight = sampleHeight(worldX + 1, worldZ);
        const int westHeight = sampleHeight(worldX - 1, worldZ);

        // A ramp is used only for a clean one-block drop along one axis.
        if (northHeight == height && southHeight == height - 1) {
            return BlockShape::RampNorth;
        }
        if (southHeight == height && northHeight == height - 1) {
            return BlockShape::RampSouth;
        }
        if (eastHeight == height && westHeight == height - 1) {
            return BlockShape::RampEast;
        }
        if (westHeight == height && eastHeight == height - 1) {
            return BlockShape::RampWest;
        }
        return BlockShape::Cube;
    };

    for (int z = 0; z < SIZE_Z; ++z) {
        for (int x = 0; x < SIZE_X; ++x) {
            const int worldX = m_position.x * SIZE_X + x;
            const int worldZ = m_position.z * SIZE_Z + z;
            const int height = sampleHeight(worldX, worldZ);

            BlockShape surfaceShape = sampleSurfaceShape(worldX, worldZ);

            // A pair of perpendicular ramps can leave a protruding inside
            // corner. Replace that center surface with the matching diagonal
            // cut when both neighboring ramps are at the same terrain height.
            const int westHeight = sampleHeight(worldX - 1, worldZ);
            const int southHeight = sampleHeight(worldX, worldZ - 1);
            const int eastHeight = sampleHeight(worldX + 1, worldZ);
            const int northHeight = sampleHeight(worldX, worldZ + 1);
            const BlockShape westRamp = westHeight == height
                ? sampleSurfaceShape(worldX - 1, worldZ) : BlockShape::Cube;
            const BlockShape southRamp = southHeight == height
                ? sampleSurfaceShape(worldX, worldZ - 1) : BlockShape::Cube;
            const BlockShape eastRamp = eastHeight == height
                ? sampleSurfaceShape(worldX + 1, worldZ) : BlockShape::Cube;
            const BlockShape northRamp = northHeight == height
                ? sampleSurfaceShape(worldX, worldZ + 1) : BlockShape::Cube;
            const BlockShape junctionShape = getTopCornerJunctionShape(
                westRamp, southRamp, eastRamp, northRamp);
            if (junctionShape != BlockShape::Cube) {
                surfaceShape = junctionShape;
            }

            for (int y = 0; y < SIZE_Y; ++y) {
                if (y > height) {
                    setBlock(x, y, z, BlockType::Air);
                } else if (y == height) {
                    setBlock(x, y, z, BlockType::Grass);
                    setBlockShape(x, y, z, surfaceShape);
                } else if (y >= height - 3 && y < height - 1) {
                    setBlock(x, y, z, BlockType::Dirt);
                } else {
                    setBlock(x, y, z, BlockType::Stone);
                }
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

    for (int z = 0; z < SIZE_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
            for (int x = 0; x < SIZE_X; ++x) {
                const BlockType blockType = getBlock(x, y, z);
                if (blockType == BlockType::Air) {
                    continue;
                }

                const BlockShape shape = getBlockShape(x, y, z);
                const glm::vec3 baseColor = getBlockColor(blockType);

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

                    const bool isSideFace = face >= 2;
                    const bool clipSideAgainstRamp =
                        isSideFace && neighborBlock != BlockType::Air &&
                        neighborShape != BlockShape::Cube &&
                        !usesExplicitBlockShapeGeometry(neighborShape);

                    // A bottom-corner cut above this cube leaves only a
                    // triangle exposed. A top-corner cut above touches the
                    // supporting cube at only one point, so its whole top is
                    // still exposed and must remain visible.
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

                    if (neighborBlock == BlockType::Air || clipSideAgainstRamp) {
                        const float brightness = getFaceBrightness(face);
                        const uint32_t base = static_cast<uint32_t>(vertices.size() / 6);
                        for (int i = 0; i < 4; ++i) {
                            const float cornerX = faceCorners[face][i][0];
                            const float cornerY = faceCorners[face][i][1];
                            const float cornerZ = faceCorners[face][i][2];
                            float shapedY = cornerY;
                            const auto profileCornerIndex = [](float px, float pz) {
                                return (pz < 0.5f)
                                    ? ((px < 0.5f) ? 0 : 1)
                                    : ((px < 0.5f) ? 3 : 2);
                            };
                            if (cornerY > 0.0f) {
                                shapedY = shapeData.topHeights[
                                    profileCornerIndex(cornerX, cornerZ)];
                            } else if (clipSideAgainstRamp) {
                                float neighborCornerX = cornerX;
                                float neighborCornerZ = cornerZ;
                                if (face == 2) neighborCornerX = 0.0f;
                                if (face == 3) neighborCornerX = 1.0f;
                                if (face == 4) neighborCornerZ = 0.0f;
                                if (face == 5) neighborCornerZ = 1.0f;
                                const int neighborCorner =
                                    profileCornerIndex(neighborCornerX, neighborCornerZ);
                                shapedY = getBlockShapeData(neighborShape).topHeights[neighborCorner];
                            }
                            vertices.push_back(x + cornerX);
                            vertices.push_back(y + shapedY);
                            vertices.push_back(z + cornerZ);
                            const glm::vec3 color = baseColor * brightness;
                            vertices.push_back(color.r);
                            vertices.push_back(color.g);
                            vertices.push_back(color.b);
                        }

                        indices.insert(indices.end(), {
                            base, base + 1, base + 2,
                            base, base + 2, base + 3
                        });
                    }
                }
            }
        }
    }

    Mesh mesh;
    mesh.upload(vertices, indices);
    return mesh;
}
