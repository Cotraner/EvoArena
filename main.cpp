#include "graphics.h"

int main() {
    Graphics graphics; // construit la fenêtre

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false; // on sort de la boucle
            }
        }

    }

    return 0;
}