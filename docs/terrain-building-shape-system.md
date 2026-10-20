# Shared Terrain and Building Shape System

## Goal

Natural terrain and player construction use the same grid-cell shape definitions. Terrain generation chooses shapes automatically; building tools choose them intentionally. Material (grass, dirt, stone, etc.) is independent of geometry.

## Current prototype constraints

The current world stores one `BlockType` per cell. `Chunk::buildMesh` emits six axis-aligned cube faces, `World::raycast` traverses grid cells, and `Player::collides` treats every non-air cell as a full cube. Replacing only the renderer would therefore produce incorrect picking and collision.

## Proposed cell model

Each cell should ultimately store:

- **Material** — Air, Grass, Dirt, Stone, and future materials.
- **Shape** — the geometric form occupying that cell.
- **Optional state later** — orientation or rotation if not already encoded by the shape identifier.

Material and shape must not be conflated. A grass ramp and a stone ramp are the same shape with different materials.

## First shape family

Start with a deliberately small, grid-aligned library:

- Full cube.
- Four directional ramps (one for each horizontal direction).
- Four corner-transition wedges, each with one low corner and three high corners, for cases where two slopes converge around a cell.
- True concave/inverted corner cuts remain a separate shape family: they remove material from a corner and cannot be represented correctly by a top-height profile alone.

The precise ramp naming convention and vertex winding should be centralized in the shape definitions, not reimplemented separately by terrain generation, meshing, collision, and placement.

## Shared geometry contract

A shape definition must eventually be the single source of truth for:

1. Render triangles and outward-facing normals.
2. Collision surfaces and walkable-slope information.
3. Picking/raycast intersection behavior.
4. Neighbor-face visibility and edge transitions.
5. Placement validation and terrain generation.

Do not assume every occupied cell is a full cube once shape support is enabled.

## Mesh generation plan

- Build exposed surfaces from the cell's actual shape geometry.
- Suppress internal faces only when the neighboring shape fully covers the same surface region; a simple `neighbor != Air` check is insufficient for partial shapes.
- Keep vertices in chunk-local coordinates and preserve the existing chunk model transform.
- When a cell changes, rebuild its chunk mesh and neighboring chunk meshes when the edited cell touches a chunk boundary.

## Collision and editing plan

The current grid DDA raycast can remain useful for broad-phase cell traversal, but shape-aware interaction must test the candidate cell's actual triangles or supported shape intersection. Player collision should query shape surfaces rather than treating every occupied cell as a unit AABB. Implement these only after rendering shape geometry is verified.

## Incremental implementation order

1. Add the shape vocabulary and shared data contract.
2. Make cells store material plus shape, preserving cube defaults for existing world generation.
3. Render cube and ramp geometry using shared shape definitions.
4. Add shape-aware picking and placement.
5. Add slope collision and grounding.
6. Generate a small test hillside and validate terrain/building interchangeability.
7. Add corner-transition wedges, then implement true concave/inverted corner cuts with explicit mesh geometry rather than trying to encode them as top-height profiles.
8. Add more materials and larger terrain features.

## Out of scope for the first milestone

- Marching Cubes or a density-field terrain engine.
- Arbitrary free-form meshes per cell.
- Caves and overhangs.
- Large-scale streaming or performance redesign.

The goal is to prove the shared-shape architecture on a small, testable area before expanding it.
