#define SDL_MAIN_HANDLED
#include "core/Window.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include "render/Mesh.h"
#include "world/Chunk.h"
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
        
        // Create and generate chunk
        Chunk chunk;
        chunk.generateFlat();
        Mesh chunkMesh = chunk.buildMesh();
        
        // Camera setup
        Camera camera(glm::vec3(8.0f, 16.0f, 24.0f));
        
        // Main loop
        Uint32 lastTime = SDL_GetTicks();
        while (!window.shouldClose()) {
            // Calculate delta time
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert to seconds
            lastTime = currentTime;
            
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
            
            camera.processMouse(mouseDX, mouseDY);
            camera.processKeyboard(forward, back, left, right, up, down, deltaTime);
            
            window.clear();
            
            // Use shader and set uniforms
            ourShader.use();
            
            // Projection matrix
            float aspectRatio = 800.0f / 600.0f;
            glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
            
            // View matrix
            glm::mat4 view = camera.getViewMatrix();
            
            // Model matrix (identity for static world)
            glm::mat4 model = glm::mat4(1.0f);
            
            ourShader.setMat4("model", model);
            ourShader.setMat4("view", view);
            ourShader.setMat4("projection", projection);
            
            // Draw the chunk
            chunkMesh.draw();
        }
        
        // Cleanup is handled by Mesh destructor
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
