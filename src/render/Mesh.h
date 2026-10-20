#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <vector>
#include <cstdint>

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Delete copy constructor and copy assignment
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move constructor and move assignment
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
    void draw() const;

private:
    GLuint VAO, VBO, EBO;
    GLsizei indexCount;
};

#endif // MESH_H
