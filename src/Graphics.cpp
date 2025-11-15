#include "Graphics.h"


Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(WINDOW_SIZE_WIDTH, WINDOW_SIZE_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "EvoArena");

    //Fond vert
    SDL_SetRenderDrawColor(renderer, 0, 150, 0, 0);
    SDL_RenderClear(renderer);
    SDL_Surface* surface = IMG_Load("../assets/images/background.jpg");
    if (!surface) {
        std::cerr << "Erreur IMG_Load: " << IMG_GetError() << std::endl;
    } else {
        background = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!background) {
            std::cerr << "Erreur création texture : " << SDL_GetError() << std::endl;
        }
    }
    SDL_Surface* menuBgSurface = IMG_Load("../assets/images/backgroundMenu.jpg");
    if (!menuBgSurface) {
        std::cerr << "Erreur IMG_Load pour le fond du menu: " << IMG_GetError() << std::endl;
    } else {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBgSurface);
        SDL_FreeSurface(menuBgSurface);
        if (!menuBackgroundTexture) {
            std::cerr << "Erreur création texture menu : " << SDL_GetError() << std::endl;
        }
    }
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
    if (background) {
        SDL_DestroyTexture(background);
        background = nullptr;
    }
    if (menuBackgroundTexture) { // <<< NOUVEAU : Destruction du fond du menu >>>
        SDL_DestroyTexture(menuBackgroundTexture);
        menuBackgroundTexture = nullptr;
    }

    SDL_Quit();
}

void Graphics::drawBackground() {
    if (background) {
        SDL_RenderCopy(renderer, background, nullptr, nullptr);
    }
}