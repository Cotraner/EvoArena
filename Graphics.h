#ifndef EVOARENA_GRAPHICS_H
#define EVOARENA_GRAPHICS_H

#include <SDL2/SDL.h>

struct SDL_Window;
struct SDL_Renderer;

class Graphics {
public:
    Graphics();
    ~Graphics();
private:
    SDL_Window* _window;
    SDL_Renderer* _renderer;
};


#endif //EVOARENA_GRAPHICS_H
