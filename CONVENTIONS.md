# Project: Voxel survival game (C++)

## Stack

- C++17, CMake, SDL2, OpenGL 3.3 core, GLAD, GLM, stb_image
- No other dependencies without asking

## Layout

- src/core/ window, input, timing, main loop
- src/render/ shader, mesh, texture, camera, renderer
- src/world/ block, chunk, world, terrain generation
- src/game/ player, inventory, crafting, survival stats, entities
- assets/shaders/, assets/textures/

## Rules

- One class per .h/.cpp pair; keep files under ~300 lines
- Use #pragma once; include only what you need
- Ownership via std::unique_ptr / std::shared_ptr; no raw new/delete
- All math via GLM types; no custom vector classes
- Left-handed? NO: right-handed, Y-up, 1 block = 1 unit
- Namespace: none; PascalCase classes, camelCase methods, m\_ prefix for members
- Never change files outside the ones I /add unless you tell me first
- Do not rewrite working code when adding a feature; make minimal edits
- Every new .cpp must be added to CMakeLists.txt
- Always create files at the exact path given. Never create source files in the project root.
- Write complete code. Never use placeholder comments like "... existing code ...".
- Include <glad/glad.h> before any SDL header; never use GLEW.
