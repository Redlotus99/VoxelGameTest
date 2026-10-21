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
    // Wedge profiles with one raised corner sloping down to the other three.
    CornerWedgeSouthWest,
    CornerWedgeSouthEast,
    CornerWedgeNorthEast,
    CornerWedgeNorthWest,
    // Complements of the corner wedges: solid above a diagonal bottom.
    CornerComplementSouthWest,
    CornerComplementSouthEast,
    CornerComplementNorthEast,
    CornerComplementNorthWest,
    // Explicit mesh-template shapes: diagonal cuts remove a corner from
    // either the bottom or top of the cell.
    CornerCutSouthWest,
    CornerCutTopSouthWest,
    // Complementary top diagonal wedge: removes the top northeast corner,
    // leaving the two front corners and one back corner on the top profile.
    CornerCutTopNorthEast,
    CornerCutTopNorthWest,
    CornerCutTopSouthEast,
    // Tetrahedral complement of CornerCutTopNorthEast: flat triangular top,
    // one northeast bottom vertex, and a matching diagonal cut face.
    OpenBottomCornerComplementNorthEast,
    // 180-degree rotated orientation of the tetrahedral complement.
    OpenBottomCornerComplementSouthWest,
    // Quarter-turn orientations of the same complementary tetrahedron.
    OpenBottomCornerComplementNorthWest,
    OpenBottomCornerComplementSouthEast,
    // Saddle profiles keep opposite corners high and the other two low.
    SaddleSouthWestNorthEast,
    SaddleSouthEastNorthWest,
    // Natural terrain top built from shared world-space corner heights.
    TerrainSurface,
    Count
};

// Corner order is viewed from above:
// 0 = southwest (x=0,z=0), 1 = southeast (x=1,z=0),
// 2 = northeast (x=1,z=1), 3 = northwest (x=0,z=1).
//
// A cell occupies the volume between its bottom and top corner profiles.
// Corner order is SW, SE, NE, NW for both profiles. This supports ordinary
// ramps (bottom=0, sloped top) and their complementary triangular prisms
// (sloped bottom, top=1) using the same representation.
struct CellShape {
    std::array<float, 4> bottom;
    std::array<float, 4> top;
};

// Legacy top-only profile retained for existing mesh paths while CellShape
// support is introduced incrementally.
struct BlockShapeData {
    std::array<float, 4> topHeights;
};

// Canonical classification for a natural terrain cell's four shared corner
// heights. Inputs may be absolute heights or offsets; the lowest corner is
// normalized to zero so generation and mesh diagnostics use identical rules.
// Corner order is SW, SE, NE, NW.
inline constexpr BlockShape getTerrainProfileShape(int sw, int se, int ne, int nw) {
    // Inputs are corner offsets from the cell's lowest corner: each is 0 or 1.
    // Keep the all-zero and all-one cases distinct: flat low profiles use the
    // natural surface label, while flat high profiles are full cubes.
    if      (sw == 0 && se == 0 && ne == 0 && nw == 0) return BlockShape::TerrainSurface;
    if      (sw == 1 && se == 1 && ne == 1 && nw == 1) return BlockShape::Cube;
    if      (sw == 0 && se == 0 && ne == 1 && nw == 1) return BlockShape::RampNorth;
    if      (sw == 1 && se == 1 && ne == 0 && nw == 0) return BlockShape::RampSouth;
    if      (sw == 0 && se == 1 && ne == 1 && nw == 0) return BlockShape::RampEast;
    if      (sw == 1 && se == 0 && ne == 0 && nw == 1) return BlockShape::RampWest;
    if      (sw == 0 && se == 1 && ne == 1 && nw == 1) return BlockShape::CornerRampSouthWest;
    if      (sw == 1 && se == 0 && ne == 1 && nw == 1) return BlockShape::CornerRampSouthEast;
    if      (sw == 1 && se == 1 && ne == 0 && nw == 1) return BlockShape::CornerRampNorthEast;
    if      (sw == 1 && se == 1 && ne == 1 && nw == 0) return BlockShape::CornerRampNorthWest;
    if      (sw == 1 && se == 0 && ne == 0 && nw == 0) return BlockShape::CornerWedgeSouthWest;
    if      (sw == 0 && se == 1 && ne == 0 && nw == 0) return BlockShape::CornerWedgeSouthEast;
    if      (sw == 0 && se == 0 && ne == 1 && nw == 0) return BlockShape::CornerWedgeNorthEast;
    if      (sw == 0 && se == 0 && ne == 0 && nw == 1) return BlockShape::CornerWedgeNorthWest;
    if      (sw == 1 && se == 0 && ne == 1 && nw == 0) return BlockShape::SaddleSouthWestNorthEast;
    if      (sw == 0 && se == 1 && ne == 0 && nw == 1) return BlockShape::SaddleSouthEastNorthWest;
    return BlockShape::TerrainSurface;
}

inline constexpr CellShape getCellShapeData(BlockShape shape) {
    switch (shape) {
        case BlockShape::CornerComplementSouthWest:
            return {{{1.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::CornerComplementSouthEast:
            return {{{0.0f, 1.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::CornerComplementNorthEast:
            return {{{0.0f, 0.0f, 1.0f, 0.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::CornerComplementNorthWest:
            return {{{0.0f, 0.0f, 0.0f, 1.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
        case BlockShape::RampNorth:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f, 1.0f}}};
        case BlockShape::RampSouth:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 0.0f, 0.0f}}};
        case BlockShape::RampEast:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 1.0f, 0.0f}}};
        case BlockShape::RampWest:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f, 1.0f}}};
        case BlockShape::CornerWedgeSouthWest:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f, 0.0f}}};
        case BlockShape::CornerWedgeSouthEast:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f, 0.0f}}};
        case BlockShape::CornerWedgeNorthEast:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f, 0.0f}}};
        case BlockShape::CornerWedgeNorthWest:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 0.0f, 1.0f}}};
        case BlockShape::Cube:
        case BlockShape::CornerRampSouthWest:
        case BlockShape::CornerRampSouthEast:
        case BlockShape::CornerRampNorthEast:
        case BlockShape::CornerRampNorthWest:
        case BlockShape::SaddleSouthWestNorthEast:
        case BlockShape::SaddleSouthEastNorthWest:
        case BlockShape::CornerCutSouthWest:
        case BlockShape::CornerCutTopSouthWest:
        case BlockShape::CornerCutTopNorthEast:
        case BlockShape::CornerCutTopNorthWest:
        case BlockShape::CornerCutTopSouthEast:
        case BlockShape::OpenBottomCornerComplementNorthEast:
        case BlockShape::OpenBottomCornerComplementSouthWest:
        case BlockShape::OpenBottomCornerComplementNorthWest:
        case BlockShape::OpenBottomCornerComplementSouthEast:
        case BlockShape::TerrainSurface:
        case BlockShape::Count:
            return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
    }
    return {{{0.0f, 0.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 1.0f, 1.0f}}};
}

inline constexpr bool usesExplicitBlockShapeGeometry(BlockShape shape) {
    return shape == BlockShape::CornerCutSouthWest ||
           shape == BlockShape::CornerCutTopSouthWest ||
           shape == BlockShape::CornerCutTopNorthEast ||
           shape == BlockShape::CornerCutTopNorthWest ||
           shape == BlockShape::CornerCutTopSouthEast ||
           shape == BlockShape::OpenBottomCornerComplementNorthEast ||
           shape == BlockShape::OpenBottomCornerComplementSouthWest ||
           shape == BlockShape::OpenBottomCornerComplementNorthWest ||
           shape == BlockShape::OpenBottomCornerComplementSouthEast;
}

// Select the top-corner cut for a perpendicular ramp junction. Each
// mapping is a rotation of the validated west-RampNorth/south-RampEast case.
// The corner-cut shape is chosen to match the two adjacent ramp orientations.
inline constexpr BlockShape getTopCornerJunctionShape(
    BlockShape westRamp,
    BlockShape southRamp,
    BlockShape eastRamp,
    BlockShape northRamp) {
    if (westRamp == BlockShape::RampNorth &&
        southRamp == BlockShape::RampEast) {
        return BlockShape::CornerCutTopSouthWest;
    }
    if (northRamp == BlockShape::RampEast &&
        westRamp == BlockShape::RampSouth) {
        return BlockShape::CornerCutTopNorthWest;
    }
    if (eastRamp == BlockShape::RampSouth &&
        northRamp == BlockShape::RampWest) {
        return BlockShape::CornerCutTopNorthEast;
    }
    if (southRamp == BlockShape::RampWest &&
        eastRamp == BlockShape::RampNorth) {
        return BlockShape::CornerCutTopSouthEast;
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
        case BlockShape::CornerWedgeSouthWest:
            return {{{1.0f, 0.0f, 0.0f, 0.0f}}};
        case BlockShape::CornerWedgeSouthEast:
            return {{{0.0f, 1.0f, 0.0f, 0.0f}}};
        case BlockShape::CornerWedgeNorthEast:
            return {{{0.0f, 0.0f, 1.0f, 0.0f}}};
        case BlockShape::CornerWedgeNorthWest:
            return {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        case BlockShape::SaddleSouthWestNorthEast:
            return {{{1.0f, 0.0f, 1.0f, 0.0f}}};
        case BlockShape::SaddleSouthEastNorthWest:
            return {{{0.0f, 1.0f, 0.0f, 1.0f}}};
        case BlockShape::CornerCutSouthWest:
        case BlockShape::CornerCutTopSouthWest:
        case BlockShape::CornerCutTopNorthEast:
        case BlockShape::CornerCutTopNorthWest:
        case BlockShape::CornerCutTopSouthEast:
        case BlockShape::OpenBottomCornerComplementNorthEast:
        case BlockShape::OpenBottomCornerComplementSouthWest:
        case BlockShape::TerrainSurface:
        case BlockShape::Count:
            break;
    }

    return {{{1.0f, 1.0f, 1.0f, 1.0f}}};
}
