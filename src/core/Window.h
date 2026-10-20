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

private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    bool should_close_;
};

#endif // WINDOW_H
