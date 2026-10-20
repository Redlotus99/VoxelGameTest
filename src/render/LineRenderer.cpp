#include "render/LineRenderer.h"

LineRenderer::LineRenderer()
    : m_shader("assets/shaders/line.vert", "assets/shaders/line.frag") {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

LineRenderer::~LineRenderer() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void LineRenderer::draw(const float* verts, int vertCount, const glm::mat4& mvp, bool white) {
    m_shader.use();
    m_shader.setMat4("mvp", mvp);
    m_shader.setInt("useWhite", white ? 1 : 0);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertCount * 3 * sizeof(float), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, vertCount);
    glBindVertexArray(0);
}

void LineRenderer::drawCrosshair(float aspect) {
    const float s = 0.025f;          // arm length in NDC (vertical)
    const float sx = s / aspect;     // keep arms the same length on screen
    const float v[] = {
        -sx, 0.0f, 0.0f,   sx, 0.0f, 0.0f,
        0.0f, -s, 0.0f,    0.0f, s, 0.0f
    };
    glDisable(GL_DEPTH_TEST);
    draw(v, 4, glm::mat4(1.0f), true);
    glEnable(GL_DEPTH_TEST);
}

void LineRenderer::drawBlockOutline(const glm::mat4& view, const glm::mat4& projection, glm::ivec3 block) {
    const float lo = -0.002f, hi = 1.002f;  // slightly larger than the block to avoid z-fighting
    glm::vec3 b(block);
    glm::vec3 c[8] = {
        b + glm::vec3(lo, lo, lo), b + glm::vec3(hi, lo, lo),
        b + glm::vec3(hi, hi, lo), b + glm::vec3(lo, hi, lo),
        b + glm::vec3(lo, lo, hi), b + glm::vec3(hi, lo, hi),
        b + glm::vec3(hi, hi, hi), b + glm::vec3(lo, hi, hi)
    };
    const int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},   // back face
        {4,5},{5,6},{6,7},{7,4},   // front face
        {0,4},{1,5},{2,6},{3,7}    // connecting edges
    };
    float v[24 * 3];
    int n = 0;
    for (int i = 0; i < 12; ++i) {
        for (int k = 0; k < 2; ++k) {
            v[n++] = c[e[i][k]].x;
            v[n++] = c[e[i][k]].y;
            v[n++] = c[e[i][k]].z;
        }
    }
    draw(v, 24, projection * view, false);
}