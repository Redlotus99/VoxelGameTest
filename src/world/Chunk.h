#pragma once

#include <array>
#include "world/Block.h"
#include "render/Mesh.h"
#include <glm/glm.hpp>
#include <functional>

class Chunk {
public:
    Chunk();
    Chunk(glm::ivec3 pos);
    
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType t);
    void generateFlat();
    void generateTerrain();
    Mesh buildMesh(const std::function<BlockType(int wx, int wy, int wz)>& getWorldBlock) const;
    
    glm::ivec3 getPosition() const { return m_position; }
    
private:
    static constexpr int SIZE_X = 16;
    static constexpr int SIZE_Y = 64;
    static constexpr int SIZE_Z = 16;
    
    static inline int index(int x, int y, int z) {
        return x + SIZE_X * (y + SIZE_Y * z);
    }
    
    std::array<BlockType, SIZE_X * SIZE_Y * SIZE_Z> blocks;
    glm::ivec3 m_position;
};
