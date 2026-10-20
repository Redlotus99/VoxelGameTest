#pragma once

#include <array>
#include "world/Block.h"
#include "render/Mesh.h"
#include <glm/glm.hpp>

class Chunk {
public:
    Chunk();
    Chunk(glm::ivec3 pos);
    
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType t);
    void generateFlat();
    Mesh buildMesh() const;
    
    glm::ivec3 getPosition() const { return m_position; }
    
private:
    static inline int index(int x, int y, int z) {
        return x + 16 * (y + 16 * z);
    }
    
    std::array<BlockType, 4096> blocks;
    glm::ivec3 m_position;
};
