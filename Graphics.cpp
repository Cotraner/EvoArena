#include "Graphics.h"

Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "EvoArena");

    //Fond vert
    SDL_SetRenderDrawColor(renderer, 0, 150, 0, 0);
    SDL_RenderClear(renderer);


    // Dessiner un cercle rempli rouge (x=320, y=240, rayon=50)
    filledCircleRGBA(renderer, 320, 240, 50, 255, 0, 0, 255);

    // Dessiner un contour noir autour
    circleRGBA(renderer, 320, 240, 50, 0, 0, 0, 255);


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