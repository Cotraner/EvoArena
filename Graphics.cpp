#include "Graphics.h"

Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(640, 480, 0, &this->_window, &this->_renderer);
    SDL_SetWindowTitle(this->_window, "EvoArena");

    //Fond vert
    SDL_SetRenderDrawColor(this->_renderer, 0, 150, 0, 0);
    SDL_RenderClear(this->_renderer);
    SDL_RenderPresent(this->_renderer);
}

Graphics::~Graphics() {
    if (this->_renderer) {
        SDL_DestroyRenderer(this->_renderer);
        this->_renderer = nullptr;
    }
    if (this->_window) {
        SDL_DestroyWindow(this->_window);
        this->_window = nullptr;
    }
    printf("BBAC"); //LE big burger au caca

    SDL_Quit();
}