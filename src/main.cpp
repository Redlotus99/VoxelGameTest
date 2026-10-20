#define SDL_MAIN_HANDLED
#include "core/Window.h"
#include "render/Shader.h"
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
        
        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        // Camera setup
        glm::vec3 cameraPos = glm::vec3(3.0f, 2.0f, 3.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Projection matrix
        float aspectRatio = 800.0f / 600.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        
        // Main loop
        float angle = 0.0f;
        while (!window.shouldClose()) {
            window.clear();
            
            // Update rotation angle
            angle += 0.01f;
            
            // Model matrix - rotate around Y axis
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // View matrix
            glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
            
            // Use shader and set uniforms
            ourShader.use();
            ourShader.setMat4("model", model);
            ourShader.setMat4("view", view);
            ourShader.setMat4("projection", projection);
            
            // Draw the cube
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            window.update();
        }
        
        // Cleanup
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
