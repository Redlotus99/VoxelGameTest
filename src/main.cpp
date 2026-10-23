#define SDL_MAIN_HANDLED
#include "core/Window.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include "world/World.h"
#include "game/Player.h"
#include "render/LineRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <limits>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
struct PlanetMetadata { std::string id; std::uint32_t seed = 0; std::string biome = "grassland"; int width = 128; int depth = 128; int height = 64; };
bool loadPlanetMetadata(const std::string& path, const std::string& expectedId, PlanetMetadata& out) {
 std::ifstream in(path); if (!in) return false; std::string magic; int version=0; unsigned long long seed=0; PlanetMetadata m;
 if (!(in>>magic>>version>>m.id>>seed>>m.biome>>m.width>>m.depth>>m.height)) return false;
 if (magic!="VOXEL_PLANET_META" || version!=1 || m.id!=expectedId || seed>std::numeric_limits<std::uint32_t>::max() || m.width!=128 || m.depth!=128 || m.height!=64) return false;
 m.seed=static_cast<std::uint32_t>(seed); out=m; return true;
}
bool savePlanetMetadata(const std::string& path, const PlanetMetadata& m) {
 const std::string tmp=path+".tmp"; { std::ofstream out(tmp,std::ios::trunc); if(!out) return false; out<<"VOXEL_PLANET_META 1 "<<m.id<<' '<<m.seed<<' '<<m.biome<<' '<<m.width<<' '<<m.depth<<' '<<m.height<<'\n'; out.flush(); if(!out.good()) return false; }
 std::remove(path.c_str()); return std::rename(tmp.c_str(),path.c_str())==0;
}
}

int main(int argc, char* argv[]) {
    try {
        Window window(1280, 720, "Voxel Game");
        
        // Enable depth testing and face culling
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // Build and compile our shader program
        Shader ourShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
        
        // Create LineRenderer
        LineRenderer lines;
        
        // Select the world mode before generating chunks.
        World world;
        bool shapeTestMode = false;
        bool terrainShapeDebugMode = false;
        bool planetTestMode = false;
        std::string planetId = "grassland-01";
        std::uint32_t requestedSeed = 0; bool seedWasProvided = false;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--shape-test") shapeTestMode = true;
            else if (argument == "--terrain-shape-debug") terrainShapeDebugMode = true;
            else if (argument == "--planet-test") planetTestMode = true;
            else if (argument == "--planet-id" && i + 1 < argc) planetId = argv[++i];
            else if (argument == "--planet-seed" && i + 1 < argc) { try { auto n=std::stoull(argv[++i]); if(n<=std::numeric_limits<std::uint32_t>::max()){requestedSeed=static_cast<std::uint32_t>(n);seedWasProvided=true;} else std::cerr<<"Planet seed out of range.\n"; } catch(...) { std::cerr<<"Invalid planet seed.\n"; } }
        }

        if (planetTestMode) {
            // Keep planet IDs filename-safe and deterministic.
            for (char& ch : planetId) {
                const bool safe = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                    || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
                if (!safe) ch = '_';
            }
            if (planetId.empty()) planetId = "grassland-01";
            const std::string metadataPath = "planet_" + planetId + ".meta";
            PlanetMetadata metadata;
            if (!loadPlanetMetadata(metadataPath, planetId, metadata)) { metadata.id=planetId; metadata.seed=seedWasProvided?requestedSeed:0; metadata.biome="grassland"; if(!savePlanetMetadata(metadataPath,metadata)) std::cerr<<"Warning: could not save planet metadata.\n"; }
            else if(seedWasProvided && requestedSeed!=metadata.seed) std::cout<<"Existing planet seed "<<metadata.seed<<" takes precedence over requested seed "<<requestedSeed<<".\n";
            world.generatePlanetPrototype(metadata.seed);
            const std::string planetSavePath = "planet_" + planetId + "_edits.dat";
            bool loadedPlanet = world.loadPlanetEdits(planetSavePath, planetId);
            if (!loadedPlanet && planetId == "grassland-01") {
                // Migrate the previous prototype save without discarding edits.
                loadedPlanet = world.loadPlanetEdits("planet_edits.dat", planetId);
            }
            if (loadedPlanet) {
                std::cout << "Loaded saved planet '" << planetId << "' (save: " << planetSavePath << " or legacy save).\n";
            } else {
                std::cout << "No compatible save for planet '" << planetId << "'; starting with fresh terrain.\n";
            }
            std::cout << "Planet prototype: 128x128 blocks, biome=" << metadata.biome << ", seed=" << metadata.seed << ", wraparound enabled.\n";
        } else {
            // Preserve the established terrain and shape-test baseline.
            world.generate(4);
        }
        world.setShapeDebugMode(terrainShapeDebugMode);
        if (shapeTestMode && !planetTestMode) {
            world.generateCornerCutTestArrangement();
        }
        // Camera setup
        Camera camera(glm::vec3(8.0f, 16.0f, 24.0f));
        
        // Create player
        Player player;
        if (shapeTestMode && !planetTestMode) {
            player.position = glm::vec3(5.5f, 41.5f, 8.5f);
        } else if (planetTestMode) {
            float spawnHeight = 24.0f;
            world.getTerrainSurfaceHeight(64.5f, 64.5f, spawnHeight);
            player.position = glm::vec3(64.5f, spawnHeight, 64.5f);
            camera.Position = player.getEyePosition();
        }
        
        // Main loop
        Uint32 lastTime = SDL_GetTicks();
        bool previousJumpKey = false;
        bool previousFlyKey = false;
        bool flyMode = false;
        while (!window.shouldClose()) {
            // Calculate delta time
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert to seconds
            lastTime = currentTime;
            
            // Clamp dt to at most 0.05
            if (deltaTime > 0.05f) {
                deltaTime = 0.05f;
            }
            
            window.update();
            
            // Get mouse delta after update
            int mouseDX = window.getMouseDX();
            int mouseDY = window.getMouseDY();
            
            // Process input
            bool forward = window.isKeyDown(SDL_SCANCODE_W);
            bool back = window.isKeyDown(SDL_SCANCODE_S);
            bool left = window.isKeyDown(SDL_SCANCODE_A);
            bool right = window.isKeyDown(SDL_SCANCODE_D);
            bool down = window.isKeyDown(SDL_SCANCODE_LSHIFT);
            bool jumpKey = window.isKeyDown(SDL_SCANCODE_SPACE);
            bool flyKey = window.isKeyDown(SDL_SCANCODE_F);

            // Toggle flight once per F key press.
            if (flyKey && !previousFlyKey) {
                flyMode = !flyMode;
                player.velocity = glm::vec3(0.0f);
                player.onGround = false;
            }
            previousFlyKey = flyKey;

            // Create movement direction vector.
            glm::vec3 moveDir(0.0f);
            const glm::vec3 front = camera.getFront();
            const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            const glm::vec3 horizontalFront = glm::vec3(front.x, 0.0f, front.z);
            const glm::vec3 forwardDir = glm::length(horizontalFront) > 0.0001f
                ? glm::normalize(horizontalFront) : glm::vec3(0.0f, 0.0f, -1.0f);
            const glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, worldUp));

            if (flyMode) {
                // Flight follows the camera pitch, with Space/Shift for vertical movement.
                if (forward) moveDir += front;
                if (back) moveDir -= front;
                if (right) moveDir += rightDir;
                if (left) moveDir -= rightDir;
                if (jumpKey) moveDir += worldUp;
                if (down) moveDir -= worldUp;

                if (glm::length(moveDir) > 0.0f) {
                    moveDir = glm::normalize(moveDir);
                    player.position += moveDir * (Player::WALK_SPEED * 2.0f) * deltaTime;
                }
                player.velocity = glm::vec3(0.0f);
                player.onGround = false;
            } else {
                if (forward) moveDir += forwardDir;
                if (back) moveDir -= forwardDir;
                if (right) moveDir += rightDir;
                if (left) moveDir -= rightDir;

                if (glm::length(moveDir) > 0.0f) {
                    moveDir = glm::normalize(moveDir);
                }

                // Jump only on the key-down edge, not continuously while Space is held.
                const bool jump = jumpKey && !previousJumpKey;
                player.update(deltaTime, world, moveDir, jump);
            }
            previousJumpKey = jumpKey;
            
            // Wrap logical coordinates while preserving the existing controller.
            world.wrapPosition(player.position);

            // Update camera to follow player
            camera.Position = player.getEyePosition();
            camera.processMouse(mouseDX, mouseDY);
            
            window.clear();
            
            // Use shader and set uniforms
            ourShader.use();
            
            // Projection matrix
            float aspectRatio = (float)window.getWidth() / (float)window.getHeight();
            glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
            
            // View matrix
            glm::mat4 view = camera.getViewMatrix();
            
            // Raycast for block interaction
            glm::ivec3 hit, prev;
            bool found = world.raycast(camera.Position, camera.Front, 6.0f, hit, prev);
            
            // Handle mouse clicks for block placement/removal
            if (window.wasMousePressed(SDL_BUTTON_LEFT) && found) {
                // Remove block
                world.setBlockWorld(hit.x, hit.y, hit.z, BlockType::Air);
            }
            else if (window.wasMousePressed(SDL_BUTTON_RIGHT) && found) {
                // Place block
                // A placed block occupies [prev, prev + 1) on each axis.
                // Reject placement if that cube intersects the player's AABB.
                const float playerMinX = player.position.x - Player::HALF_WIDTH;
                const float playerMaxX = player.position.x + Player::HALF_WIDTH;
                const float playerMinY = player.position.y;
                const float playerMaxY = player.position.y + Player::HEIGHT;
                const float playerMinZ = player.position.z - Player::HALF_WIDTH;
                const float playerMaxZ = player.position.z + Player::HALF_WIDTH;

                const bool overlaps =
                    static_cast<float>(prev.x + 1) > playerMinX &&
                    static_cast<float>(prev.x) < playerMaxX &&
                    static_cast<float>(prev.y + 1) > playerMinY &&
                    static_cast<float>(prev.y) < playerMaxY &&
                    static_cast<float>(prev.z + 1) > playerMinZ &&
                    static_cast<float>(prev.z) < playerMaxZ;

                if (!overlaps) {
                    world.setBlockWorld(prev.x, prev.y, prev.z, BlockType::Stone);
                }
            }
            
            // Draw the world
            world.draw(ourShader, view, projection);
            
            // Draw crosshair and block outline
            lines.drawCrosshair(aspectRatio);
            if (found) {
                lines.drawBlockOutline(view, projection, hit);
            }
        }
        
        if (planetTestMode) {
            const std::string planetSavePath = "planet_" + planetId + "_edits.dat";
            if (world.savePlanetEdits(planetSavePath, planetId)) {
                std::cout << "Saved planet '" << planetId << "' edits to " << planetSavePath << "\n";
            } else {
                std::cerr << "Warning: could not save planet '" << planetId << "' to " << planetSavePath << "\n";
            }
        }

        // Cleanup is handled by Mesh destructor
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
