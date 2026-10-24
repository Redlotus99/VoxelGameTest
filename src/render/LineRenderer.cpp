#include "render/LineRenderer.h"
#include <array>
#include <string>
#include <vector>
#include <cmath>

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
    const float s = 0.025f;
    const float sx = s / aspect;
    const float v[] = {
        -sx, 0.0f, 0.0f, sx, 0.0f, 0.0f,
        0.0f, -s, 0.0f, 0.0f, s, 0.0f
    };
    glDisable(GL_DEPTH_TEST);
    draw(v, 4, glm::mat4(1.0f), true);
    glEnable(GL_DEPTH_TEST);
}

// Small vector-font HUD; positions are in screen-space NDC and remain fixed
// in the upper-left corner regardless of camera orientation.
void LineRenderer::drawCoordinates(const glm::vec3& position, float aspect) {
    const std::string text = "X:" + std::to_string(static_cast<int>(std::floor(position.x)))
        + " Y:" + std::to_string(static_cast<int>(std::floor(position.y)))
        + " Z:" + std::to_string(static_cast<int>(std::floor(position.z)));
    // Seven-segment glyphs: top, upper-right, lower-right, bottom,
    // lower-left, upper-left, middle. Letters are drawn as line strokes.
    const bool digits[10][7] = {
        {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},
        {1,1,1,1,0,0,1},{0,1,1,0,0,1,1},{1,0,1,1,0,1,1},
        {1,0,1,1,1,1,1},{1,1,1,0,0,0,0},{1,1,1,1,1,1,1},
        {1,1,1,1,0,1,1}
    };
    const float w = 0.010f / aspect;
    const float h = 0.026f;
    const float gap = 0.009f / aspect;
    const float startX = -0.975f;
    const float topY = 0.94f;
    std::vector<float> verts;
    auto line = [&](float x1, float y1, float x2, float y2) {
        verts.insert(verts.end(), {x1, y1, 0.0f, x2, y2, 0.0f});
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        const float x = startX + static_cast<float>(i) * (w + gap);
        const float y = topY;
        if (ch >= '0' && ch <= '9') {
            const bool* s = digits[ch - '0'];
            const float mid = y - h * 0.5f;
            const float bot = y - h;
            if (s[0]) line(x, y, x+w, y);
            if (s[1]) line(x+w, y, x+w, mid);
            if (s[2]) line(x+w, mid, x+w, bot);
            if (s[3]) line(x, bot, x+w, bot);
            if (s[4]) line(x, mid, x, bot);
            if (s[5]) line(x, y, x, mid);
            if (s[6]) line(x, mid, x+w, mid);
        } else if (ch == 'X') {
            line(x, y, x+w, y-h); line(x+w, y, x, y-h);
        } else if (ch == 'Y') {
            line(x, y, x+w*0.5f, y-h*0.5f); line(x+w, y, x+w*0.5f, y-h*0.5f);
            line(x+w*0.5f, y-h*0.5f, x+w*0.5f, y-h);
        } else if (ch == 'Z') {
            line(x, y, x+w, y); line(x+w, y, x, y-h); line(x, y-h, x+w, y-h);
        } else if (ch == ':') {
            line(x+w*0.5f, y-h*0.25f, x+w*0.5f, y-h*0.25f);
            line(x+w*0.5f, y-h*0.8f, x+w*0.5f, y-h*0.8f);
        } else if (ch == '-') {
            line(x, y-h*0.5f, x+w, y-h*0.5f);
        }
    }
    if (!verts.empty()) {
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        draw(verts.data(), static_cast<int>(verts.size() / 3), glm::mat4(1.0f), true);
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
    }
}

void LineRenderer::drawBlockOutline(const glm::mat4& view, const glm::mat4& projection, glm::ivec3 block) {
    const float lo = -0.002f, hi = 1.002f;
    glm::vec3 b(block);
    glm::vec3 c[8] = {
        b + glm::vec3(lo, lo, lo), b + glm::vec3(hi, lo, lo),
        b + glm::vec3(hi, hi, lo), b + glm::vec3(lo, hi, lo),
        b + glm::vec3(lo, lo, hi), b + glm::vec3(hi, lo, hi),
        b + glm::vec3(hi, hi, hi), b + glm::vec3(lo, hi, hi)
    };
    const int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
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
