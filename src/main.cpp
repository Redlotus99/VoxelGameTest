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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
        
        // Create and generate world
        World world;
        world.generate(2);

        const bool shapeTestMode = argc > 1 && std::string(argv[1]) == "--shape-test";
        if (shapeTestMode) {
            world.generateCornerCutTestArrangement();
        }
        
        // Camera setup
        Camera camera(glm::vec3(8.0f, 16.0f, 24.0f));
        
        // Create player
        Player player;
        if (shapeTestMode) {
            // Start on the elevated test platform, looking toward the shapes.
            player.position = glm::vec3(8.5f, 41.05f, 10.5f);
        }
        
        // Main loop
        Uint32 lastTime = SDL_GetTicks();
        bool previousJumpKey = false;
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
            
            // Create movement direction vector
            glm::vec3 moveDir(0.0f);
            
            // Get camera front vector
            glm::vec3 front = camera.getFront();
            
            // Compute movement in walking mode (flyMode false)
            glm::vec3 forwardDir = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
            glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0, 1, 0)));
            
            if (forward) moveDir += forwardDir;
            if (back) moveDir -= forwardDir;
            if (right) moveDir += rightDir;
            if (left) moveDir -= rightDir;
            
            // Normalize the movement direction to maintain consistent speed
            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
            }
            
            // Jump only on the key-down edge, not continuously while Space is held.
            bool jump = jumpKey && !previousJumpKey;
            previousJumpKey = jumpKey;

            // Update player
            player.update(deltaTime, world, moveDir, jump);
            
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
        
        // Cleanup is handled by Mesh destructor
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
