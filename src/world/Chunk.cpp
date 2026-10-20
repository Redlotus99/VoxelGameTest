#include "world/Chunk.h"
#include <vector>

Chunk::Chunk() {
    // Fill with Air by default
    for (int i = 0; i < 4096; ++i) {
        blocks[i] = BlockType::Air;
    }
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
        return BlockType::Air;
    }
    return blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType t) {
    if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
        return;
    }
    blocks[index(x, y, z)] = t;
}

void Chunk::generateFlat() {
    // Generate flat terrain
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            // Stone layer (y < 8)
            for (int y = 0; y < 8; ++y) {
                setBlock(x, y, z, BlockType::Stone);
            }
            
            // Dirt layer (y >= 8 and y < 10)
            for (int y = 8; y < 10; ++y) {
                setBlock(x, y, z, BlockType::Dirt);
            }
            
            // Grass layer (y == 10)
            setBlock(x, 10, z, BlockType::Grass);
            
            // Air above grass
            for (int y = 11; y < 16; ++y) {
                setBlock(x, y, z, BlockType::Air);
            }
        }
    }
}

Mesh Chunk::buildMesh() const {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    uint32_t indexOffset = 0;
    
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
    
    // Face directions for normals and vertex ordering
    const glm::vec3 faceNormals[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f),   // Top (+Y)
        glm::vec3(0.0f, -1.0f, 0.0f),  // Bottom (-Y)
        glm::vec3(1.0f, 0.0f, 0.0f),   // +X
        glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
        glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
        glm::vec3(0.0f, 0.0f, -1.0f)   // -Z
    };
    
    // Vertex positions for a unit cube (2 triangles per face)
    const float cubeVertices[8][3] = {
        {0.0f, 0.0f, 0.0f},  // 0
        {1.0f, 0.0f, 0.0f},  // 1
        {1.0f, 1.0f, 0.0f},  // 2
        {0.0f, 1.0f, 0.0f},  // 3
        {0.0f, 0.0f, 1.0f},  // 4
        {1.0f, 0.0f, 1.0f},  // 5
        {1.0f, 1.0f, 1.0f},  // 6
        {0.0f, 1.0f, 1.0f}   // 7
    };
    
    // Face indices (two triangles per face)
    const int faceIndices[6][6] = {
        {0, 1, 2, 2, 3, 0},  // Top (+Y) - vertices 0,1,2,3
        {4, 7, 6, 6, 5, 4},  // Bottom (-Y) - vertices 4,7,6,5
        {1, 5, 6, 6, 2, 1},  // +X - vertices 1,5,6,2
        {0, 3, 7, 7, 4, 0},  // -X - vertices 0,3,7,4
        {0, 4, 5, 5, 1, 0},  // +Z - vertices 0,4,5,1
        {3, 2, 6, 6, 7, 3}   // -Z - vertices 3,2,6,7
    };
    
    // Process each block in the chunk
    for (int z = 0; z < 16; ++z) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
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
                        
                        // Add vertices for this face
                        for (int i = 0; i < 6; ++i) {
                            int vertexIndex = faceIndices[face][i];
                            
                            // Position
                            vertices.push_back(cubeVertices[vertexIndex][0] + x);
                            vertices.push_back(cubeVertices[vertexIndex][1] + y);
                            vertices.push_back(cubeVertices[vertexIndex][2] + z);
                            
                            // Color (apply brightness)
                            glm::vec3 color = baseColor * brightness;
                            vertices.push_back(color.r);
                            vertices.push_back(color.g);
                            vertices.push_back(color.b);
                        }
                        
                        // Add indices for this face
                        uint32_t baseIndex = indexOffset;
                        indices.push_back(baseIndex + 0);
                        indices.push_back(baseIndex + 1);
                        indices.push_back(baseIndex + 2);
                        indices.push_back(baseIndex + 2);
                        indices.push_back(baseIndex + 3);
                        indices.push_back(baseIndex + 0);
                        
                        indexOffset += 4;
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
