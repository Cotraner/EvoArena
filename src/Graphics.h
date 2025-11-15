// Graphics.h

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

    // NOUVEAU GETTER : Permet à main.cpp d'accéder à la texture
    SDL_Texture* getMenuBackgroundTexture() { return menuBackgroundTexture; } ;

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background = nullptr;
    SDL_Texture* menuBackgroundTexture = nullptr; // Note : Déjà privé
};


#endif //EVOARENA_GRAPHICS_H