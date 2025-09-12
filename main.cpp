#include "graphics.h"
#include <iostream>

int main() {
    Graphics graphics;

    bool running = true;
    SDL_Event event;

    SDL_Renderer* renderer = graphics.getRenderer();

    SDL_Rect square = {200, 200, 50, 50};


    while (running) {

    // 1️⃣ Effacer l'écran (fond noir)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // 2️⃣ Dessiner le carré
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &square);

        // 3️⃣ Afficher à l'écran
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if(event.type == SDL_KEYDOWN){
                switch(event.key.keysym.scancode){
                    case SDL_SCANCODE_UP:
                        std::cout << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                        square.y -= 5;
                        break;
                    case SDL_SCANCODE_DOWN:
                        square.y += 5;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        square.x += 5;
                        break;
                    case SDL_SCANCODE_LEFT:
                        square.x -= 5;
                        break;
                }
            }

        }

    }

    return 0;
}