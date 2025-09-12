#include "Graphics.h"
#include <iostream>
#include <vector>

int main() {
    Graphics graphics;
    auto *e1 = new Entity(320, 240, 50, 100, 2, {255, 0, 0});
    auto *e2 = new Entity(100, 100, 30, 100, 2, {0, 255, 0});
    auto *e3 = new Entity(500, 400, 20, 100, 2, {0, 0, 255});
    auto *e4 = new Entity(200, 300, 40, 100, 2, {255, 255, 0});
    std::vector<Entity> entities = {*e1, *e2, *e3, *e4};

    SDL_Event event;
    bool running = true;

    while (running) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Nettoyer l'écran
        SDL_RenderClear(graphics.getRenderer());

        // Dessiner le fond
        graphics.drawBackground();


        // Dessiner les entités
        for (auto &entity : entities) {
            entity.update();
            entity.draw(graphics.getRenderer());
        }

        // Afficher le rendu
        SDL_RenderPresent(graphics.getRenderer());

        SDL_Delay(16); // Approx ~60 FPS
    }

    return 0;
}