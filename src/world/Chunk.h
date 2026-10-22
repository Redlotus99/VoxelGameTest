#pragma once

#include <array>
#include "world/Block.h"
#include "world/BlockShape.h"
#include "render/Mesh.h"
#include <glm/glm.hpp>
#include <functional>

class Chunk {
public:
    static constexpr int SIZE_X = 16;
    static constexpr int SIZE_Y = 64;
    static constexpr int SIZE_Z = 16;

    Chunk();
    Chunk(glm::ivec3 pos);
    
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType t);

    BlockShape getBlockShape(int x, int y, int z) const;
    void setBlockShape(int x, int y, int z, BlockShape shape);

    void generateFlat();
    void generateTerrain(int wrapSize = 0);
    Mesh buildMesh(
        const std::function<BlockType(int wx, int wy, int wz)>& getWorldBlock,
        const std::function<BlockShape(int wx, int wy, int wz)>& getWorldShape,
        bool shapeDebugMode = false) const;
    
    glm::ivec3 getPosition() const { return m_position; }
    
private:
    static inline int index(int x, int y, int z) {
        return x + SIZE_X * (y + SIZE_Y * z);
    }
    
    std::array<BlockType, SIZE_X * SIZE_Y * SIZE_Z> blocks;
    // Shape is stored separately from material so every material can use the
    // same geometry. Existing world generation remains cube-shaped by default.
    std::array<BlockShape, SIZE_X * SIZE_Y * SIZE_Z> shapes;
    glm::ivec3 m_position;
};
