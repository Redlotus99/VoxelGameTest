#define SDL_MAIN_HANDLED
#include "core/Window.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include "world/World.h"
#include "game/Player.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main(int argc, char* argv[]) {
    try {
        Window window(800, 600, "Voxel Game");
        
        // Enable depth testing and face culling
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // Build and compile our shader program
        Shader ourShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
        
        // Create and generate world
        World world;
        world.generate(2);
        
        // Camera setup
        Camera camera(glm::vec3(8.0f, 16.0f, 24.0f));
        
        // Create player
        Player player;
        
        // Main loop
        Uint32 lastTime = SDL_GetTicks();
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
            bool up = window.isKeyDown(SDL_SCANCODE_SPACE);
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
            
            // Update player
            bool jump = false;
            if (jumpKey) {
                jump = true;
                
                // Print player state once per second while space is held
                static Uint32 lastPrintTime = 0;
                Uint32 now = SDL_GetTicks();
                if (now - lastPrintTime > 1000) {
                    std::cout << "onGround: " << player.onGround << ", velocity.y: " << player.velocity.y << std::endl;
                    lastPrintTime = now;
                }
            }
            
            player.update(deltaTime, world, moveDir, jump);
            
            // Update camera to follow player
            camera.Position = player.getEyePosition();
            camera.processMouse(mouseDX, mouseDY);
            
            window.clear();
            
            // Use shader and set uniforms
            ourShader.use();
            
            // Projection matrix
            float aspectRatio = 800.0f / 600.0f;
            glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
            
            // View matrix
            glm::mat4 view = camera.getViewMatrix();
            
            // Draw the world
            world.draw(ourShader, view, projection);
        }
        
        // Cleanup is handled by Mesh destructor
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
