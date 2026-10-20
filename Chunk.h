#pragma once

#include <array>
#include "world/Block.h"
#include "render/Mesh.h"

class Chunk {
public:
    Chunk();
    
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType t);
    void generateFlat();
    Mesh buildMesh() const;
    
private:
    static inline int index(int x, int y, int z) {
        return x + 16 * (y + 16 * z);
    }
    
    std::array<BlockType, 4096> blocks;
};
