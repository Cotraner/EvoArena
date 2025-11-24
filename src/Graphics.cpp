#include "Graphics.h"
#include <SDL2/SDL_image.h> // Assurez-vous que IMG_Load est disponible

Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
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

    // Chargement de l'icône des paramètres
    SDL_Surface* settingsIconSurface = IMG_Load("../assets/images/settings-icon.png");
    if (!settingsIconSurface) {
        std::cerr << "Erreur IMG_Load pour l'icône settings: " << IMG_GetError() << std::endl;
    } else {
        settingsIconTexture = SDL_CreateTextureFromSurface(renderer, settingsIconSurface);
        SDL_FreeSurface(settingsIconSurface);
        if (!settingsIconTexture) {
            std::cerr << "Erreur création texture icône settings : " << SDL_GetError() << std::endl;
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
    if (menuBackgroundTexture) {
        SDL_DestroyTexture(menuBackgroundTexture);
        menuBackgroundTexture = nullptr;
    }
    // *** NOUVEAU : Destruction de l'icône ***
    if (settingsIconTexture) {
        SDL_DestroyTexture(settingsIconTexture);
        settingsIconTexture = nullptr;
    }

    SDL_Quit();
}

void Graphics::drawBackground() {
    if (background) {
        SDL_RenderCopy(renderer, background, nullptr, nullptr);
    }
}