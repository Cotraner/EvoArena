#include "Graphics.h"
#include <iostream>
#include <vector>

int main() {
    Graphics graphics;
    auto *e1 = new Entity(320, 240, 50, 100, 5, {255, 0, 0});
    auto *e2 = new Entity(100, 100, 30, 100, 5, {0, 255, 0});
    auto *e3 = new Entity(500, 400, 20, 100, 5, {0, 0, 255});
    auto *e4 = new Entity(200, 300, 40, 100, 5, {255, 255, 0});
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

        //Mettre a jour les entités
        //Si elle sont trop proche elles s'attaquent
        for(auto &entity : entities){
            for(auto &other : entities){
                if(&entity != &other){
                    int dx = entity.getX() - other.getX();
                    int dy = entity.getY() - other.getY();
                    int distance = std::sqrt(dx * dx + dy * dy);
                    if(distance < entity.getSightRadius()){
                        std::cout << "Entity attack" << std::endl;
                        entity.chooseDirection();
                        }
                }
            }
        }



        // Dessiner les entités
        for (auto &entity : entities) {
            entity.update();
            entity.draw(graphics.getRenderer());
        }


        // Afficher le rendu
        SDL_RenderPresent(graphics.getRenderer());

        SDL_Delay(16);
    }

    return 0;
}