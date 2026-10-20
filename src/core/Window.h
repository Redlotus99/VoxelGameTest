#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <memory>

struct SDL_Window;
struct SDL_GLContext;

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void clear();
    void update();
    bool shouldClose() const;
    void setShouldClose(bool close);

private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_;
    std::unique_ptr<SDL_GLContext, decltype(&SDL_GL_DeleteContext)> context_;
    bool should_close_;
};

#endif // WINDOW_H
