#ifndef EVOARENA_GRAPHICS_H
#define EVOARENA_GRAPHICS_H

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_mixer.h> // <--- NOUVEAU : Inclure Mixer
#include "constants.h"

class Graphics {
public:
    Graphics();
    ~Graphics();

    SDL_Renderer* getRenderer() { return renderer; } ;
    SDL_Window* getWindow() { return window; } ;

    void drawBackground(const Camera& cam);

    SDL_Texture* getMenuBackgroundTexture() { return menuBackgroundTexture; } ;
    SDL_Texture* getSettingsIconTexture() { return settingsIconTexture; } ;

    void playMusic();
    void stopMusic();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background = nullptr;
    SDL_Texture* menuBackgroundTexture = nullptr;
    SDL_Texture* settingsIconTexture = nullptr;

    Mix_Music* bgMusic = nullptr;
};

#endif //EVOARENA_GRAPHICS_H