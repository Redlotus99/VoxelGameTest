#pragma once

#include <array>
#include <cstdint>

// Geometry is independent of material: a grass ramp and a stone ramp use
// the same BlockShape with different BlockType values.
enum class BlockShape : std::uint8_t {
    Cube = 0,
    RampNorth,
    RampSouth,
    RampEast,
    RampWest,
    Count
};

// Corner order is viewed from above:
// 0 = southwest (x=0,z=0), 1 = southeast (x=1,z=0),
// 2 = northeast (x=1,z=1), 3 = northwest (x=0,z=1).
//
// Heights describe the top profile of a solid cell whose bottom is y=0.
// A value of 1 at every corner is a full cube. Keep this canonical data
// shared by meshing, collision, picking, and placement code.
struct BlockShapeData {
    std::array<float, 4> topHeights;
};

inline constexpr BlockShapeData getBlockShapeData(BlockShape shape) {
    switch (shape) {
        case BlockShape::Cube:
            return {{{1.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::RampNorth:
            return {{{0.0f, 0.0f, 1.0f, 1.0f}}};
        case BlockShape::RampSouth:
            return {{{1.0f, 1.0f, 0.0f, 0.0f}}};
        case BlockShape::RampEast:
            return {{{0.0f, 1.0f, 1.0f, 0.0f}}};
        case BlockShape::RampWest:
            return {{{1.0f, 0.0f, 0.0f, 1.0f}}};
        case BlockShape::Count:
            break;
    }

    return {{{1.0f, 1.0f, 1.0f, 1.0f}}};
}
