#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "render/Shader.h"

class LineRenderer {
public:
    LineRenderer();
    ~LineRenderer();
    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;

    void drawCrosshair(float aspect);
    void drawBlockOutline(const glm::mat4& view, const glm::mat4& projection, glm::ivec3 block);

private:
    void draw(const float* verts, int vertCount, const glm::mat4& mvp, bool white);

    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};