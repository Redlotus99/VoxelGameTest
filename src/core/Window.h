#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <memory>
#include <array>

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
    
    // For mouse motion accumulation
    int getMouseDX() const { return m_mouseXrel; }
    int getMouseDY() const { return m_mouseYrel; }
    
    // Check if a mouse button was pressed during the most recent update
    bool wasMousePressed(Uint8 button) const {
        if (button >= m_mousePressed.size()) {
            return false;
        }
        return m_mousePressed[button];
    }
    
    // Keyboard method
    bool isKeyDown(SDL_Scancode scancode) const {
        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
        return keyboardState[scancode];
    }

    // Window size methods
    int getWidth() const;
    int getHeight() const;

private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    bool should_close_;
    
    // Mouse state
    int m_mouseXrel;
    int m_mouseYrel;
    std::array<bool, 6> m_mousePressed;
    
    // Window dimensions
    int m_width;
    int m_height;
};

#endif // WINDOW_H
