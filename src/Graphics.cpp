#include "Graphics.h"
#include <SDL2/SDL_image.h>
#include <iostream>

// Constructor: Initializes the graphics system, loads textures, and audio resources
Graphics::Graphics() {
    SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
    SDL_SetWindowTitle(window, "EvoArena");

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
    }

    // Load background music
    bgMusic = Mix_LoadMUS("../assets/sounds/music.mp3");
    if (!bgMusic) {
        std::cerr << "Failed to load background music: " << Mix_GetError() << std::endl;
    }
    menuMusic = Mix_LoadMUS("../assets/sounds/menu_music.mp3");
    if (!menuMusic) {
        std::cerr << "Failed to load menu music: " << Mix_GetError() << std::endl;
    }

    // Load sound effects
    soundPlus = Mix_LoadWAV("../assets/sounds/nb_entite_plus.mp3");
    if (!soundPlus) {
        std::cerr << "Failed to load soundPlus: " << Mix_GetError() << std::endl;
    }
    soundMin = Mix_LoadWAV("../assets/sounds/nb_entite_moin.mp3");
    if (!soundMin) {
        std::cerr << "Failed to load soundMin: " << Mix_GetError() << std::endl;
    }

    // Set default background color
    SDL_SetRenderDrawColor(renderer, 0, 50, 0, 255);
    SDL_RenderClear(renderer);

    // Load textures
    SDL_Surface* surface = IMG_Load("../assets/images/background.jpg");
    if (surface) {
        background = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    SDL_Surface* menuBgSurface = IMG_Load("../assets/images/backgroundMenu.jpg");
    if (menuBgSurface) {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBgSurface);
        SDL_FreeSurface(menuBgSurface);
    }

    SDL_Surface* settingsIconSurface = IMG_Load("../assets/images/settings-icon.png");
    if (settingsIconSurface) {
        settingsIconTexture = SDL_CreateTextureFromSurface(renderer, settingsIconSurface);
        SDL_FreeSurface(settingsIconSurface);
    }
}

// Destructor: Cleans up resources (textures, audio, renderer, and window)
Graphics::~Graphics() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (background) SDL_DestroyTexture(background);
    if (menuBackgroundTexture) SDL_DestroyTexture(menuBackgroundTexture);
    if (settingsIconTexture) SDL_DestroyTexture(settingsIconTexture);

    if (soundPlus) Mix_FreeChunk(soundPlus);
    if (soundMin) Mix_FreeChunk(soundMin);
    if (bgMusic) Mix_FreeMusic(bgMusic);
    if (menuMusic) Mix_FreeMusic(menuMusic);

    SDL_Quit();
}

// Draws the game background and grid
void Graphics::drawBackground(const Camera& cam) {
    if (background) {
        SDL_Rect dest;
        dest.x = (int)((0 - cam.x) * cam.zoom);
        dest.y = (int)((0 - cam.y) * cam.zoom);
        dest.w = (int)(WORLD_WIDTH * cam.zoom);
        dest.h = (int)(WORLD_HEIGHT * cam.zoom);

        SDL_RenderCopy(renderer, background, nullptr, &dest);
    } else {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
    }

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 100);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect worldBorder;
    worldBorder.x = (int)((0 - cam.x) * cam.zoom);
    worldBorder.y = (int)((0 - cam.y) * cam.zoom);
    worldBorder.w = (int)(WORLD_WIDTH * cam.zoom);
    worldBorder.h = (int)(WORLD_HEIGHT * cam.zoom);
    SDL_RenderDrawRect(renderer, &worldBorder);
}

// Plays the background music
void Graphics::playMusic() {
    if (bgMusic) {
        if (Mix_PlayingMusic() == 0) {
            Mix_PlayMusic(bgMusic, -1);
        } else if (Mix_PausedMusic() == 1) {
            Mix_ResumeMusic();
        }
    }
}

// Plays the menu music
void Graphics::playMenuMusic() {
    if (menuMusic) {
        if (Mix_PlayingMusic() == 0 || Mix_GetMusicType(NULL) != MUS_MP3) {
            Mix_PlayMusic(menuMusic, -1);
        }
    }
}

// Stops any currently playing music
void Graphics::stopMusic() {
    Mix_HaltMusic();
}

// Plays the "increment" sound effect
void Graphics::playSoundPlus() {
    if (soundPlus) {
        Mix_PlayChannel(-1, soundPlus, 0);
    }
}

// Plays the "decrement" sound effect
void Graphics::playSoundMin() {
    if (soundMin) {
        Mix_PlayChannel(-1, soundMin, 0);
    }
}