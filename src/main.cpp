#include "core/Window.h"
#include <glad/glad.h>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Window window(800, 600, "Voxel Game");
        
        // Main loop
        while (!window.shouldClose()) {
            window.clear();
            window.update();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
