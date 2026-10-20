#include "world/Chunk.h"
#include <vector>
#include <glm/glm.hpp>

Chunk::Chunk() : m_position(0, 0, 0) {
    // Fill with Air by default
    for (int i = 0; i < SIZE_X * SIZE_Y * SIZE_Z; ++i) {
        blocks[i] = BlockType::Air;
    }
}

Chunk::Chunk(glm::ivec3 pos) : m_position(pos) {
    // Fill with Air by default
    for (int i = 0; i < SIZE_X * SIZE_Y * SIZE_Z; ++i) {
        blocks[i] = BlockType::Air;
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
    blocks[index(x, y, z)] = t;
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

Mesh Chunk::buildMesh() const {
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
                    
                    // Check if neighbor is air (i.e., face is exposed)
                    if (getBlock(nx, ny, nz) == BlockType::Air) {
                        // Get brightness for this face
                        float brightness = getFaceBrightness(face);
                        
                        // Compute base index for the 4 vertices of this face
                        uint32_t base = vertices.size() / 6;
                        
                        // Add vertices for this face
                        for (int i = 0; i < 4; ++i) {
                            // Position
                            vertices.push_back(x + faceCorners[face][i][0]);
                            vertices.push_back(y + faceCorners[face][i][1]);
                            vertices.push_back(z + faceCorners[face][i][2]);
                            
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
