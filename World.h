#pragma once

#include <unordered_map>
#include <memory>
#include "world/Chunk.h"
#include "render/Mesh.h"
#include <glm/glm.hpp>

class Shader;

class World {
public:
    World();
    ~World();

    void generate(int radius);
    Chunk* getChunk(glm::ivec3 position);
    void draw(Shader& shader, const glm::mat4& viewProj);

private:
    int64_t packCoordinates(glm::ivec3 coords) const;
    glm::ivec3 unpackCoordinates(int64_t packed) const;

    std::unordered_map<int64_t, std::unique_ptr<Chunk>> m_chunks;
    std::unordered_map<int64_t, Mesh> m_meshes;
};
