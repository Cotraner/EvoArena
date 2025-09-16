#include "Graphics.h"
#include "constants.h"
#include <iostream>
#include <vector>

int main() {
    Graphics graphics;
    auto *e1 = new Entity("e1",320, 240, 50, 100,5, 100 ,{255, 0, 0});
    auto *e2 = new Entity("e2",100, 100, 30, 100,5, 100,{0, 255, 0});
    auto *e3 = new Entity("e3",500, 400, 20, 100,5, 100,{0, 0, 255});
    auto *e4 = new Entity("e4",200, 300, 40, 100,5, 100,{255, 255, 0});
    std::vector<Entity> entities = {*e1, *e2, *e3, *e4};

    SDL_Event event;
    bool running = true;

    while (running) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
        // Nettoyer l'écran
        SDL_RenderClear(graphics.getRenderer());

        // Dessiner le fond
        graphics.drawBackground();

        //Mettre a jour les entités
        //Si elle sont trop proche elles s'attaquent
        for (auto &entity : entities) {
            Entity* closestEnemy = nullptr;
            int closestDistance = entity.getSightRadius();

            for (auto &other : entities) {
                if (&entity != &other) {
                    // Calculer la distance entre les deux entités
                    int dx = entity.getX() - other.getX();
                    int dy = entity.getY() - other.getY();
                    int distance = std::sqrt(dx * dx + dy * dy);

                    // Vérifier si l'autre entité est dans le rayon de vision et plus proche
                    if (distance < closestDistance) {
                        closestDistance = distance;
                        closestEnemy = &other;
                    }
                }
            }

            // Si un ennemi le plus proche est trouvé, le cibler
            if (closestEnemy) {
                int target[2] = {closestEnemy->getX(), closestEnemy->getY()};
                std::cout << entity.getName() << " targets " << closestEnemy->getName()
                          << " at distance " << closestDistance << std::endl;
                entity.chooseDirection(target);
            }
        }

        for (auto &entity : entities) {
            for (auto &other : entities) {
                if (&entity != &other) {
                    // Calculer la distance entre les deux entités
                    int dx = entity.getX() - other.getX();
                    int dy = entity.getY() - other.getY();
                    int distance = std::sqrt(dx * dx + dy * dy);

                    // Vérifier si les entités se chevauchent
                    int minDistance = entity.getRad() + other.getRad();
                    if (distance < minDistance) {
                        // Calculer le vecteur de knockback
                        float overlap = minDistance - distance;
                        float normX = dx / (float)distance;
                        float normY = dy / (float)distance;

                        // Appliquer le knockback
                        entity.setX(entity.getX() + static_cast<int>(normX * overlap));
                        entity.setY(entity.getY() + static_cast<int>(normY * overlap));
                    }
                }
            }
            entity.update();
        }



        // Dessiner les entités
        for (auto &entity : entities) {
            entity.update();
            entity.draw(graphics.getRenderer());
        }

        // Mettre à jour les entités et gérer les attaques
        for (auto &entity : entities) {
            for (auto &other : entities) {
                if (&entity != &other) {
                    int dx = entity.getX() - other.getX();
                    int dy = entity.getY() - other.getY();
                    int distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < entity.getRad() + other.getRad()) {
                        std::cout << "Entities are attacking each other!" << std::endl;
                        entity.attack(other);
                        if (entity.getHealth() <= 0 && entity.getIsAlive()) {
                            entity.die();
                        }
                        entity.chooseDirection();
                    }
                }
            }
        }

        // Supprimer les entités mortes
        entities.erase(
                std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) {
                    return !entity.getIsAlive();
                }),
                entities.end()
        );
        // Afficher le rendu
        SDL_RenderPresent(graphics.getRenderer());

        SDL_Delay(64);
    }

    return 0;
}