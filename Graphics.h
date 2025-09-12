#ifndef EVOARENA_GRAPHICS_H
#define EVOARENA_GRAPHICS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

class Graphics {
public:
    Graphics();
    ~Graphics();

    SDL_Renderer* getRenderer() { return renderer; } ;
    SDL_Window* getWindow() { return window; } ;
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
};


#endif //EVOARENA_GRAPHICS_H
