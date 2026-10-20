# VoxelGameTest

A small C++17 voxel-world prototype built with SDL2, OpenGL, GLAD, and GLM.

## Requirements

- Git
- CMake 3.16 or newer
- A C++17-capable compiler
  - Windows: Visual Studio 2022 with the **Desktop development with C++** workload, or another supported C++ toolchain
- An OpenGL 3.3-compatible graphics driver

CMake downloads SDL2, GLM, and GLAD through FetchContent during configuration, so the first configure requires an internet connection.

## Build on Windows

From the repository root in PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

CMake places the executable according to the selected generator. With the Visual Studio generator, it is usually `build\Release\VoxelGame.exe`; with a single-configuration generator, it is usually `build\VoxelGame.exe`.

The build copies the `assets` directory beside the executable and copies the SDL2 runtime DLL when the shared SDL2 target is available. Run the executable from its output directory so the relative shader paths under `assets/shaders` resolve correctly.

If you use a different build directory, replace `build` in the commands above with that directory.

## Controls

- **W / A / S / D** — move
- **Mouse** — look around
- **Space** — jump
- **Left mouse button** — remove the targeted block
- **Right mouse button** — place a stone block next to the targeted block
- **F11** — toggle fullscreen
- **Escape** — quit

Block interactions use the crosshair target and a limited reach.

## Project layout

- `src/core` — SDL window and input handling
- `src/render` — shaders, camera, meshes, and line rendering
- `src/world` — chunk and world generation
- `src/game` — player movement and collision
- `assets/shaders` — GLSL shaders

## Current scope

This is an experimental prototype, not a finished game. World generation currently uses a small fixed area of chunks, and gameplay and rendering systems are still evolving.
