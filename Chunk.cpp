#include "Chunk.h"
#include <vector>
#include <cstdint>

// Assuming these constants are defined somewhere in the codebase
// For now, I'll define them here for completeness
const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;
const int CHUNK_DEPTH = 16;

// Block types - assuming these are defined elsewhere
enum class BlockType {
    Air,
    Grass,
    Dirt,
    Stone
};

// Helper function to get block color based on type
glm::vec3 getBlockColor(BlockType type) {
    switch (type) {
        case BlockType::Grass: return glm::vec3(0.3f, 0.7f, 0.2f);
        case BlockType::Dirt: return glm::vec3(0.5f, 0.3f, 0.15f);
        case BlockType::Stone: return glm::vec3(0.5f, 0.5f, 0.5f);
        default: return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

// Helper function to get brightness factor for a face
float getFaceBrightness(int face) {
    // face: 0=bottom, 1=top, 2=+X, 3=-X, 4=+Z, 5=-Z
    switch (face) {
        case 0: return 0.5f; // bottom
        case 1: return 1.0f; // top
        case 2: return 0.8f; // +X
        case 3: return 0.8f; // -X
        case 4: return 0.65f; // +Z
        case 5: return 0.65f; // -Z
        default: return 1.0f;
    }
}

Mesh Chunk::buildMesh() const {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    // Iterate through all blocks in the chunk
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_DEPTH; z++) {
                BlockType block = getBlock(x, y, z);
                
                // Only process non-air blocks
                if (block != BlockType::Air) {
                    glm::vec3 color = getBlockColor(block);
                    
                    // Check each of the 6 neighbors
                    for (int face = 0; face < 6; face++) {
                        int nx = x, ny = y, nz = z;
                        
                        switch (face) {
                            case 0: ny--; break; // bottom
                            case 1: ny++; break; // top
                            case 2: nx++; break; // +X
                            case 3: nx--; break; // -X
                            case 4: nz++; break; // +Z
                            case 5: nz--; break; // -Z
                        }
                        
                        // Get neighbor block (Air if out of bounds)
                        BlockType neighbor = getBlock(nx, ny, nz);
                        
                        // Emit quad if neighbor is Air
                        if (neighbor == BlockType::Air) {
                            float brightness = getFaceBrightness(face);
                            
                            // Define the 4 vertices for this face
                            // Each vertex has position + color (6 floats)
                            glm::vec3 v0, v1, v2, v3;
                            
                            switch (face) {
                                case 0: // bottom face
                                    v0 = glm::vec3(x, y, z);
                                    v1 = glm::vec3(x+1, y, z);
                                    v2 = glm::vec3(x+1, y, z+1);
                                    v3 = glm::vec3(x, y, z+1);
                                    break;
                                case 1: // top face
                                    v0 = glm::vec3(x, y+1, z+1);
                                    v1 = glm::vec3(x+1, y+1, z+1);
                                    v2 = glm::vec3(x+1, y+1, z);
                                    v3 = glm::vec3(x, y+1, z);
                                    break;
                                case 2: // +X face
                                    v0 = glm::vec3(x+1, y, z);
                                    v1 = glm::vec3(x+1, y+1, z);
                                    v2 = glm::vec3(x+1, y+1, z+1);
                                    v3 = glm::vec3(x+1, y, z+1);
                                    break;
                                case 3: // -X face
                                    v0 = glm::vec3(x, y, z+1);
                                    v1 = glm::vec3(x, y+1, z+1);
                                    v2 = glm::vec3(x, y+1, z);
                                    v3 = glm::vec3(x, y, z);
                                    break;
                                case 4: // +Z face
                                    v0 = glm::vec3(x, y, z+1);
                                    v1 = glm::vec3(x+1, y, z+1);
                                    v2 = glm::vec3(x+1, y+1, z+1);
                                    v3 = glm::vec3(x, y+1, z+1);
                                    break;
                                case 5: // -Z face
                                    v0 = glm::vec3(x, y, z);
                                    v1 = glm::vec3(x+1, y, z);
                                    v2 = glm::vec3(x+1, y+1, z);
                                    v3 = glm::vec3(x, y+1, z);
                                    break;
                            }
                            
                            // Add vertices to the vertex buffer
                            // Position (x,y,z) + Color (r,g,b)
                            vertices.push_back(v0.x); vertices.push_back(v0.y); vertices.push_back(v0.z);
                            vertices.push_back(color.r * brightness); vertices.push_back(color.g * brightness); vertices.push_back(color.b * brightness);
                            
                            vertices.push_back(v1.x); vertices.push_back(v1.y); vertices.push_back(v1.z);
                            vertices.push_back(color.r * brightness); vertices.push_back(color.g * brightness); vertices.push_back(color.b * brightness);
                            
                            vertices.push_back(v2.x); vertices.push_back(v2.y); vertices.push_back(v2.z);
                            vertices.push_back(color.r * brightness); vertices.push_back(color.g * brightness); vertices.push_back(color.b * brightness);
                            
                            vertices.push_back(v3.x); vertices.push_back(v3.y); vertices.push_back(v3.z);
                            vertices.push_back(color.r * brightness); vertices.push_back(color.g * brightness); vertices.push_back(color.b * brightness);
                            
                            // Add indices for the two triangles (counter-clockwise)
                            uint32_t baseIndex = static_cast<uint32_t>(vertices.size() / 6 - 4);
                            indices.push_back(baseIndex + 0);
                            indices.push_back(baseIndex + 1);
                            indices.push_back(baseIndex + 2);
                            
                            indices.push_back(baseIndex + 0);
                            indices.push_back(baseIndex + 2);
                            indices.push_back(baseIndex + 3);
                        }
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
