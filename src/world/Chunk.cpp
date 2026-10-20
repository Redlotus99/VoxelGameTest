#include "world/Chunk.h"
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
    // Keep height sampling deterministic across chunk boundaries so the
    // terrain surface can choose compatible shared ramp shapes.
    const auto sampleHeight = [](int worldX, int worldZ) -> int {
        const float noise = glm::perlin(glm::vec2(worldX, worldZ) * 0.02f);
        int height = 24 + static_cast<int>(noise * 14.0f);
        return std::max(1, std::min(height, SIZE_Y - 2));
    };

    for (int z = 0; z < SIZE_Z; ++z) {
        for (int x = 0; x < SIZE_X; ++x) {
            const int worldX = m_position.x * SIZE_X + x;
            const int worldZ = m_position.z * SIZE_Z + z;
            const int height = sampleHeight(worldX, worldZ);

            // A ramp is used only for a clean one-block drop along one axis.
            // This avoids ambiguous corner transitions until corner shapes
            // are added to the shared vocabulary.
            const int northHeight = sampleHeight(worldX, worldZ + 1);
            const int southHeight = sampleHeight(worldX, worldZ - 1);
            const int eastHeight = sampleHeight(worldX + 1, worldZ);
            const int westHeight = sampleHeight(worldX - 1, worldZ);

            BlockShape surfaceShape = BlockShape::Cube;
            if (northHeight == height && southHeight == height - 1) {
                surfaceShape = BlockShape::RampNorth;
            } else if (southHeight == height && northHeight == height - 1) {
                surfaceShape = BlockShape::RampSouth;
            } else if (eastHeight == height && westHeight == height - 1) {
                surfaceShape = BlockShape::RampEast;
            } else if (westHeight == height && eastHeight == height - 1) {
                surfaceShape = BlockShape::RampWest;
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

Mesh Chunk::buildMesh(const std::function<BlockType(int wx, int wy, int wz)>& getWorldBlock) const {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    // Define color mappings for each block type
    auto getBlockColor = [](BlockType type) -> glm::vec3 {
        switch (type) {
            case BlockType::Grass: return glm::vec3(0.3f, 0.7f, 0.2f);
            case BlockType::Dirt: return glm::vec3(0.5f, 0.3f, 0.15f);
            case BlockType::Stone: return glm::vec3(0.5f, 0.5f, 0.5f);
            default: return glm::vec3(0.0f, 0.0f, 0.0f); // Air
        }
    };
    
    // Define brightness for each face direction
    auto getFaceBrightness = [](int face) -> float {
        switch (face) {
            case 0: return 1.0f;  // Top (+Y)
            case 1: return 0.5f;  // Bottom (-Y)
            case 2: return 0.8f;  // +X
            case 3: return 0.8f;  // -X
            case 4: return 0.65f; // +Z
            case 5: return 0.65f; // -Z
            default: return 1.0f;
        }
    };
    
    // Corner offsets for each face (counter-clockwise when viewed from outside)
    const float faceCorners[6][4][3] = {
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},  // 0 +Y
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},  // 1 -Y
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},  // 2 +X
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},  // 3 -X
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},  // 4 +Z
        {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}   // 5 -Z
    };
    
    // Process each block in the chunk
    for (int z = 0; z < SIZE_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
            for (int x = 0; x < SIZE_X; ++x) {
                BlockType blockType = getBlock(x, y, z);
                
                // Skip air blocks
                if (blockType == BlockType::Air) {
                    continue;
                }
                
                glm::vec3 baseColor = getBlockColor(blockType);
                const BlockShapeData shapeData = getBlockShapeData(getBlockShape(x, y, z));
                
                // Check each of the 6 faces
                for (int face = 0; face < 6; ++face) {
                    int nx = x, ny = y, nz = z;
                    
                    switch (face) {
                        case 0: ny++; break; // Top (+Y)
                        case 1: ny--; break; // Bottom (-Y)
                        case 2: nx++; break; // +X
                        case 3: nx--; break; // -X
                        case 4: nz++; break; // +Z
                        case 5: nz--; break; // -Z
                    }
                    
                    BlockType neighborBlock = BlockType::Air;
                    BlockShape neighborShape = BlockShape::Cube;
                    const bool neighborIsLocal =
                        nx >= 0 && nx < SIZE_X && ny >= 0 && ny < SIZE_Y &&
                        nz >= 0 && nz < SIZE_Z;
                    
                    // Check if neighbor is inside this chunk
                    if (neighborIsLocal) {
                        neighborBlock = getBlock(nx, ny, nz);
                        neighborShape = getBlockShape(nx, ny, nz);
                    } else {
                        // The world callback currently exposes material only;
                        // cross-chunk neighbors therefore retain cube visibility
                        // rules until the world shape lookup is added.
                        int worldX = m_position.x * SIZE_X + nx;
                        int worldY = ny;
                        int worldZ = m_position.z * SIZE_Z + nz;
                        neighborBlock = getWorldBlock(worldX, worldY, worldZ);
                    }

                    const bool isSideFace = face >= 2;
                    const bool clipSideAgainstRamp =
                        isSideFace && neighborBlock != BlockType::Air &&
                        neighborShape != BlockShape::Cube;
                    
                    // A ramp does not cover the whole neighboring side. Keep
                    // the exposed portion of that side and clip its lower edge
                    // to the ramp's top profile instead of hiding the face.
                    if (neighborBlock == BlockType::Air || clipSideAgainstRamp) {
                        // Get brightness for this face
                        float brightness = getFaceBrightness(face);
                        
                        // Compute base index for the 4 vertices of this face
                        uint32_t base = vertices.size() / 6;
                        
                        // Add vertices for this face
                        for (int i = 0; i < 4; ++i) {
                            // Position. Any vertex on the cell's top edge follows
                            // the canonical shape profile, so cubes and ramps share
                            // this mesh path and the same shape data used elsewhere.
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
                                // Map this side's endpoints to the matching
                                // edge on the adjacent ramp. The neighbor's
                                // shape profile is defined in its own cell.
                                float neighborCornerX = cornerX;
                                float neighborCornerZ = cornerZ;
                                if (face == 2) neighborCornerX = 0.0f; // +X -> neighbor west
                                if (face == 3) neighborCornerX = 1.0f; // -X -> neighbor east
                                if (face == 4) neighborCornerZ = 0.0f; // +Z -> neighbor south
                                if (face == 5) neighborCornerZ = 1.0f; // -Z -> neighbor north
                                const int neighborCorner =
                                    profileCornerIndex(neighborCornerX, neighborCornerZ);
                                shapedY = getBlockShapeData(neighborShape).topHeights[neighborCorner];
                            }
                            vertices.push_back(x + cornerX);
                            vertices.push_back(y + shapedY);
                            vertices.push_back(z + cornerZ);
                            
                            // Color (apply brightness)
                            glm::vec3 color = baseColor * brightness;
                            vertices.push_back(color.r);
                            vertices.push_back(color.g);
                            vertices.push_back(color.b);
                        }
                        
                        // Add indices for this face (two triangles per face)
                        indices.push_back(base + 0);
                        indices.push_back(base + 1);
                        indices.push_back(base + 2);
                        indices.push_back(base + 0);
                        indices.push_back(base + 2);
                        indices.push_back(base + 3);
                    }
                }
            }
        }
    }
    
    // Create and return the mesh
    Mesh mesh;
    mesh.upload(vertices, indices);
    return mesh;
}
