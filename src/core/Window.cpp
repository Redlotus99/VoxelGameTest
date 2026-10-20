#include "Window.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdexcept>

Window::Window(int width, int height, const std::string& title) 
    : should_close_(false) {
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Create window
    window_ = {SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    ), &SDL_DestroyWindow};

    if (!window_) {
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }

    // Create OpenGL context
    context_ = {SDL_GL_CreateContext(window_.get()), &SDL_GL_DeleteContext};
    if (!context_) {
        throw std::runtime_error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
    }

    // Enable vsync
    SDL_GL_SetSwapInterval(1);
}

Window::~Window() {
    // Context and window are automatically destroyed due to unique_ptr
    SDL_Quit();
}

void Window::clear() {
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f); // Sky blue color
    glClear(GL_COLOR_BUFFER_BIT);
}

void Window::update() {
    SDL_GL_SwapWindow(window_.get());
    
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            should_close_ = true;
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            should_close_ = true;
        }
    }
}

bool Window::shouldClose() const {
    return should_close_;
}

void Window::setShouldClose(bool close) {
    should_close_ = close;
}
