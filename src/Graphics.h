#ifndef EVOARENA_GRAPHICS_H
#define EVOARENA_GRAPHICS_H

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "Entity/Entity.h"
#include "constants.h"

class Graphics {
public:
    Graphics();
    ~Graphics();

    SDL_Renderer* getRenderer() { return renderer; } ;
    SDL_Window* getWindow() { return window; } ;
    void drawBackground();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background = nullptr;
};


#endif //EVOARENA_GRAPHICS_H
