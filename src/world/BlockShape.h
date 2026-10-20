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
    CornerRampSouthWest,
    CornerRampSouthEast,
    CornerRampNorthEast,
    CornerRampNorthWest,
    // Explicit mesh-template shapes: diagonal cuts remove a corner from
    // either the bottom or top of the cell.
    CornerCutSouthWest,
    CornerCutTopSouthWest,
    // Complementary top diagonal wedge: removes the top northeast corner,
    // leaving the two front corners and one back corner on the top profile.
    CornerCutTopNorthEast,
    Count
};

// Corner order is viewed from above:
// 0 = southwest (x=0,z=0), 1 = southeast (x=1,z=0),
// 2 = northeast (x=1,z=1), 3 = northwest (x=0,z=1).
//
// Heights describe the top profile of a solid cell whose bottom is y=0.
// A value of 1 at every corner is a full cube. Shapes that remove material
// from a corner use explicit mesh geometry instead of this height profile.
struct BlockShapeData {
    std::array<float, 4> topHeights;
};

inline constexpr bool usesExplicitBlockShapeGeometry(BlockShape shape) {
    return shape == BlockShape::CornerCutSouthWest ||
           shape == BlockShape::CornerCutTopSouthWest ||
           shape == BlockShape::CornerCutTopNorthEast;
}

// Select a diagonal inside-corner shape for the currently supported
// perpendicular ramp junctions. Arguments describe the ramp in each adjacent
// cell; Cube means the arrangement does not match a supported junction yet.
// The mappings mirror the validated --shape-test arrangement and its 180-degree
// counterpart. Add other rotated mappings when their explicit meshes exist.
inline constexpr BlockShape getTopCornerJunctionShape(
    BlockShape westRamp,
    BlockShape southRamp,
    BlockShape eastRamp,
    BlockShape northRamp) {
    if (westRamp == BlockShape::RampNorth &&
        southRamp == BlockShape::RampEast) {
        return BlockShape::CornerCutTopSouthWest;
    }
    if (eastRamp == BlockShape::RampSouth &&
        northRamp == BlockShape::RampWest) {
        return BlockShape::CornerCutTopNorthEast;
    }
    return BlockShape::Cube;
}

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
        // Corner ramps rise toward the three high corners, leaving one
        // low corner. These are transition wedges where slopes meet.
        case BlockShape::CornerRampSouthWest:
            return {{{0.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::CornerRampSouthEast:
            return {{{1.0f, 0.0f, 1.0f, 1.0f}}};
        case BlockShape::CornerRampNorthEast:
            return {{{1.0f, 1.0f, 0.0f, 1.0f}}};
        case BlockShape::CornerRampNorthWest:
            return {{{1.0f, 1.0f, 1.0f, 0.0f}}};
        case BlockShape::CornerCutSouthWest:
        case BlockShape::CornerCutTopSouthWest:
        case BlockShape::CornerCutTopNorthEast:
        case BlockShape::Count:
            break;
    }

    return {{{1.0f, 1.0f, 1.0f, 1.0f}}};
}
