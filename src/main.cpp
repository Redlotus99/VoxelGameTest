#define SDL_MAIN_HANDLED
#include "core/Window.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include "render/Mesh.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main(int argc, char* argv[]) {
    try {
        Window window(800, 600, "Voxel Game");
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Build and compile our shader program
        Shader ourShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
        
        // Set up vertex data and buffers and configure vertex attributes
        float vertices[] = {
            // positions          // colors
            -0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f
        };
        
        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0, // front
            4, 5, 6, 6, 7, 4, // back
            0, 4, 7, 7, 3, 0, // left
            1, 5, 6, 6, 2, 1, // right
            0, 1, 5, 5, 4, 0, // top
            2, 3, 7, 7, 6, 2  // bottom
        };
        
        Mesh mesh;
        mesh.upload(std::vector<float>(vertices, vertices + sizeof(vertices) / sizeof(vertices[0])), 
                    std::vector<uint32_t>(indices, indices + sizeof(indices) / sizeof(indices[0])));
        
        // Camera setup
        Camera camera(glm::vec3(3.0f, 2.0f, 3.0f));
        
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
            
            // Model matrix (no rotation now, since we can fly around the cube)
            glm::mat4 model = glm::mat4(1.0f);
            
            ourShader.setMat4("model", model);
            ourShader.setMat4("view", view);
            ourShader.setMat4("projection", projection);
            
            // Draw the cube
            mesh.draw();
        }
        
        // Cleanup is handled by Mesh destructor
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
