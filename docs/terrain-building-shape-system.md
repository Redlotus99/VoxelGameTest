# Shared Terrain and Building Shape System

## Goal

Natural terrain and player construction use the same grid-cell shape definitions. Terrain generation chooses shapes automatically; building tools choose them intentionally. Material (grass, dirt, stone, etc.) is independent of geometry.

## Current prototype constraints

The current world stores one `BlockType` and one `BlockShape` per cell. `Chunk::buildMesh` supports cube/ramp profiles and explicit corner-cut templates. `World::raycast` traverses grid cells, and `Player::collides` treats every non-air cell as a full cube. The cut shapes are rendering milestones, not yet fully interactive collision shapes. The supporting mesh preserves the triangular portion of a full block's top face exposed beneath a bottom-corner cut; beneath a top-corner cut, the supporting block's full top remains exposed because the cut shape touches it only at a point. The opt-in `--shape-test` mode builds an elevated stone platform with both cut orientations and an L-shaped arrangement: the west ramp uses `RampNorth`, the south ramp uses `RampEast`, and the center uses `CornerCutTopSouthWest`. Relative to the prior arrangement, the two ramp orientations are changed in opposite quarter-turn directions and the center wedge is rotated 180 degrees (the removed top corner changes from northeast to southwest). The same southwest-cut wedge remains isolated for reference. Terrain generation samples neighboring surface shapes in world coordinates and selects a matching top-corner cut for all four rotated versions of the validated perpendicular-ramp pairing. All four top-corner cut orientations now share one mesh template.

## Proposed cell model

Each cell should ultimately store:

- **Material** — Air, Grass, Dirt, Stone, and future materials.
- **Shape** — the geometric form occupying that cell.
- **Optional state later** — orientation or rotation if not already encoded by the shape identifier.

Material and shape must not be conflated. A grass ramp and a stone ramp are the same shape with different materials.

## Shape families

- Full cube.
- Four directional ramps (one for each horizontal direction).
- Four corner-transition wedges, each with one low corner and three high corners, for cases where two slopes converge around a cell.
- **Bottom-corner cut:** removes the bottom southwest corner using a diagonal plane from one upper corner to two lower corners.
- **Top-corner cut (front corner removed):** removes the top southwest corner, leaving the two back corners and one front corner on the triangular top.
- **Complementary top-corner cut (back corner removed):** removes the top northeast corner, leaving the two front corners and one back corner on the triangular top.
- Four top-corner cut orientations are supported: southwest, southeast, northeast, and northwest. They share a mesh template that rotates the cut geometry and visible triangular faces according to the removed corner. Terrain generation recognizes the four rotations of the validated perpendicular-ramp pairing and chooses the matching cut shape.

## Shared geometry contract

A shape definition must eventually be the single source of truth for:

1. Render triangles and outward-facing normals.
2. Collision surfaces and walkable-slope information.
3. Picking/raycast intersection behavior.
4. Neighbor-face visibility and edge transitions.
5. Placement validation and terrain generation.

Do not assume every occupied cell is a full cube once shape support is enabled. The explicit corner cut currently has dedicated mesh emission; collision and raycasting are still cube-based.

## Mesh generation plan

- Build exposed surfaces from the cell's actual shape geometry.
- Suppress internal faces only when the neighboring shape fully covers the same surface region; a simple `neighbor != Air` check is insufficient for general partial-shape combinations.
- Keep vertices in chunk-local coordinates and preserve the existing chunk model transform.
- When a cell changes, rebuild its chunk mesh and neighboring chunk meshes when the edited cell touches a chunk boundary.
- Keep the explicit cut surface separate from the six axis-aligned faces, since it is a diagonal plane.

## Collision and editing plan

The current grid DDA raycast can remain useful for broad-phase cell traversal, but shape-aware interaction must test the candidate cell's actual triangles or supported shape intersection. Player collision should query shape surfaces rather than treating every occupied cell as a unit AABB. Implement these after rendering the shape geometry is verified.

## Incremental implementation order

1. Add the shape vocabulary and shared data contract.
2. Make cells store material plus shape, preserving cube defaults for existing world generation.
3. Render cube and ramp geometry using shared shape definitions.
4. Add one explicit corner-cut mesh template and validate its visible faces (launch the built executable with the `--shape-test` argument).
5. Add the other cut orientations and neighboring partial-shape surface handling.
6. Add shape-aware picking and placement.
7. Add slope collision and grounding.
8. Generate a small test hillside and validate terrain/building interchangeability.
9. Add more materials and larger terrain features.

## Out of scope for the first milestone

- Marching Cubes or a density-field terrain engine.
- Arbitrary free-form meshes per cell.
- Caves and overhangs.
- Large-scale streaming or performance redesign.

The goal is to prove the shared-shape architecture on a small, testable area before expanding it.
