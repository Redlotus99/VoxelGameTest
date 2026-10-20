#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <memory>

#include <SDL.h>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // Delete copy constructor and assignment operator
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void clear();
    void update();
    bool shouldClose() const;
    void setShouldClose(bool close);
    
    // Mouse methods
    SDL_Point getMouseDelta() const { return SDL_Point{m_mouseXrel, m_mouseYrel}; }
    void resetMouseDelta() { m_mouseXrel = 0; m_mouseYrel = 0; }
    
    // Keyboard method
    bool isKeyDown(SDL_Scancode scancode) const {
        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
        return keyboardState[scancode];
    }

private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    bool should_close_;
    
    // Mouse state
    int m_mouseXrel;
    int m_mouseYrel;
};

#endif // WINDOW_H
