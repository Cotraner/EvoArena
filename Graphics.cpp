#include "Graphics.h"

Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "EvoArena");

    //Fond vert
    SDL_SetRenderDrawColor(renderer, 0, 150, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

Graphics::~Graphics() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        this->renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}