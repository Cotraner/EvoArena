#include "Graphics.h"
#include <SDL2/SDL_image.h>

Graphics::Graphics(){
    SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
    SDL_SetWindowTitle(window, "EvoArena");

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    // Chargement de la musique
    // Assurez-vous que le fichier est bien à cet endroit !
    bgMusic = Mix_LoadMUS("../assets/sounds/music.mp3");
    if (bgMusic == nullptr) {
        std::cerr << "Failed to load beat music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }
    menuMusic = Mix_LoadMUS("../assets/sounds/menu_music.mp3");
    if (menuMusic == nullptr) {
        std::cerr << "Erreur chargement menu_music: " << Mix_GetError() << std::endl;
    }
    soundPlus = Mix_LoadWAV("../assets/sounds/nb_entite_plus.mp3");
    if (soundPlus == nullptr) {
        std::cerr << "Erreur chargement soundPlus: " << Mix_GetError() << std::endl;
    }

    soundMin = Mix_LoadWAV("../assets/sounds/nb_entite_moin.mp3");
    if (soundMin == nullptr) {
        std::cerr << "Erreur chargement soundMinus: " << Mix_GetError() << std::endl;
    }

    // Fond vert par défaut
    SDL_SetRenderDrawColor(renderer, 0, 50, 0, 255);
    SDL_RenderClear(renderer);

    // Chargement texture jeu
    SDL_Surface* surface = IMG_Load("../assets/images/background.jpg");
    if (surface) {
        background = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Chargement texture menu
    SDL_Surface* menuBgSurface = IMG_Load("../assets/images/backgroundMenu.jpg");
    if (menuBgSurface) {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBgSurface);
        SDL_FreeSurface(menuBgSurface);
    }

    // Icône settings
    SDL_Surface* settingsIconSurface = IMG_Load("../assets/images/settings-icon.png");
    if (settingsIconSurface) {
        settingsIconTexture = SDL_CreateTextureFromSurface(renderer, settingsIconSurface);
        SDL_FreeSurface(settingsIconSurface);
    }
}

Graphics::~Graphics() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (background) SDL_DestroyTexture(background);
    if (menuBackgroundTexture) SDL_DestroyTexture(menuBackgroundTexture);
    if (settingsIconTexture) SDL_DestroyTexture(settingsIconTexture);

    if (soundPlus) Mix_FreeChunk(soundPlus);
    if (soundMin) Mix_FreeChunk(soundMin);
    if (bgMusic) {
        Mix_FreeMusic(bgMusic);
        bgMusic = nullptr;
    }
    if (menuMusic) {
        Mix_FreeMusic(menuMusic);
        menuMusic = nullptr;
    }
    SDL_Quit();
}

void Graphics::drawBackground(const Camera& cam) {
    // 1. DESSIN DU FOND (Image étirée sur tout le monde)
    if (background) {
        // Calcul de la position du monde vue par la caméra
        SDL_Rect dest;
        dest.x = (int)((0 - cam.x) * cam.zoom);
        dest.y = (int)((0 - cam.y) * cam.zoom);
        dest.w = (int)(WORLD_WIDTH * cam.zoom);
        dest.h = (int)(WORLD_HEIGHT * cam.zoom);

        // On dessine l'image de fond sur toute la surface du monde
        SDL_RenderCopy(renderer, background, nullptr, &dest);
    } else {
        // Fallback couleur unie si pas d'image
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
    }

    // 2. DESSIN D'UNE GRILLE (Indispensable pour voir le mouvement si le fond est uni)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 100); // Gris foncé

    int gridSize = 100; // Taille des cases en pixels monde


    // 3. DESSIN DES BORDURES DU MONDE (Cadre Rouge)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect worldBorder;
    worldBorder.x = (int)((0 - cam.x) * cam.zoom);
    worldBorder.y = (int)((0 - cam.y) * cam.zoom);
    worldBorder.w = (int)(WORLD_WIDTH * cam.zoom);
    worldBorder.h = (int)(WORLD_HEIGHT * cam.zoom);
    SDL_RenderDrawRect(renderer, &worldBorder);
}

void Graphics::playMusic() {
    if (bgMusic) {
        // Si la musique ne joue pas déjà
        if (Mix_PlayingMusic() == 0) {
            // -1 signifie "boucle infinie"
            Mix_PlayMusic(bgMusic, -1);
        }
        else {
            // Si elle était en pause, on reprend
            if (Mix_PausedMusic() == 1) {
                Mix_ResumeMusic();
            }
        }
    }
}

void Graphics::playMenuMusic() {
    if (menuMusic) {
        // Si cette musique ne joue pas déjà
        if (Mix_PlayingMusic() == 0 || Mix_GetMusicType(NULL) != MUS_MP3) {
            Mix_PlayMusic(menuMusic, -1); // -1 = boucle infinie
        }
    }
}

void Graphics::stopMusic() {
    // On arrête complètement la musique (Halt)
    Mix_HaltMusic();
}

void Graphics::playSoundPlus() {
    if (soundPlus) {
        Mix_PlayChannel(-1, soundPlus, 0); // -1 = premier canal libre, 0 = pas de boucle
    }
}

void Graphics::playSoundMin() {
    if (soundMin) {
        Mix_PlayChannel(-1, soundMin, 0);
    }
}