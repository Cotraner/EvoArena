#include "Graphics.h"
#include "constants.h"
#include "Menu.h"
#include "Entity/Entity.h"
#include "Entity/Projectile.h"
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>
#include <ctime>

// --- DÉFINITION DE L'ÉTAT DU JEU ---
enum GameState {
    MENU,
    SIMULATION
};

namespace {
    // Carte pour suivre le temps du dernier tir de chaque entité (par nom)
    std::map<std::string, Uint32> lastShotTime;
}

// --- DÉCLARATION DE LA FONCTION D'INITIALISATION DE LA SIMULATION ---
std::vector<Entity> initializeSimulation(int maxEntities);

// ----------------------------------------------------------------------

int main() {
    Graphics graphics;
    std::vector<Entity> entities;
    std::vector<Projectile> projectiles;

    // --- PARAMÈTRES DE JEU AJUSTABLES ---
    int maxEntities = 15; // Taille de la population initiale (par défaut)
    const int MIN_CELLS = 5;
    const int MAX_CELLS = 50;

    // --- PARAMÈTRES FIXES POUR LE PROJECTILE (RNG) ---
    const int PROJECTILE_SPEED = 8;
    const int PROJECTILE_RADIUS = 8;

    // --- INITIALISATION DU MENU ET DE L'ÉTAT ---
    Menu menu(graphics.getRenderer(), graphics.getMenuBackgroundTexture());
    GameState currentState = MENU;

    SDL_Event event;
    bool running = true;

    while (running) {
        // GESTION DES ÉVÉNEMENTS
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }

            // GESTION DES ÉVÉNEMENTS DU MENU
            if (currentState == MENU) {
                Menu::MenuAction action = menu.handleEvents(event);

                if (menu.getCurrentScreenState() == Menu::MAIN_MENU) {
                    if (action == Menu::START_SIMULATION) {
                        entities = initializeSimulation(maxEntities);
                        projectiles.clear();
                        lastShotTime.clear();
                        currentState = SIMULATION;
                    } else if (action == Menu::QUIT) {
                        running = false;
                    } else if (action == Menu::OPEN_SETTINGS) {
                        menu.setScreenState(Menu::SETTINGS_SCREEN);
                    }
                }
                else if (menu.getCurrentScreenState() == Menu::SETTINGS_SCREEN) {
                    if (action == Menu::SAVE_SETTINGS) {
                        menu.setScreenState(Menu::MAIN_MENU);
                    }
                    else if (action == Menu::CHANGE_CELL_COUNT) {
                        int mouseX, mouseY;
                        SDL_GetMouseState(&mouseX, &mouseY);

                        // Logique d'ajustement du nombre de cellules
                        if (SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &menu.countUpButton.rect)) {
                            if (maxEntities < MAX_CELLS) maxEntities++;
                        }
                        else if (SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &menu.countDownButton.rect)) {
                            if (maxEntities > MIN_CELLS) maxEntities--;
                        }
                    }
                }
            }
        }

        // --- LOGIQUE D'AFFICHAGE ET DE JEU PAR ÉTAT ---

        if (currentState == MENU) {
            menu.draw(maxEntities);
        }
        else if (currentState == SIMULATION) {

            // Nettoyer l'écran et dessiner le fond de la simulation
            SDL_RenderClear(graphics.getRenderer());
            graphics.drawBackground();

            // 1. LOGIQUE DE CIBLAGE ET DE TIR (Logique Avancée)
            for (auto &entity : entities) {
                if (!entity.getIsAlive()) continue;

                Entity* closestEnemy = nullptr;
                float closestDistance = (float)(WINDOW_SIZE_WIDTH + WINDOW_SIZE_HEIGHT);

                // Recherche de l'ennemi le plus proche
                for (auto &other : entities) {
                    if (&entity != &other && other.getIsAlive()) {
                        int dx = entity.getX() - other.getX();
                        int dy = entity.getY() - other.getY();
                        float distance = std::sqrt((float)dx * dx + (float)dy * dy);

                        if (distance < closestDistance) {
                            closestDistance = distance;
                            closestEnemy = &other;
                        }
                    }
                }

                if (closestEnemy) {

                    bool isRanged = entity.getIsRanged();

                    // Déterminer les stats d'attaque de l'entité actuelle
                    int attackRange = isRanged ? entity.getRangedRange() : entity.getMeleeRange();
                    Uint32 cooldown = isRanged ? entity.getRangedCooldownMS() : entity.getMeleeCooldownMS();
                    int damage = isRanged ? entity.getRangedDamage() : entity.getMeleeDamage();


                    // A. MOUVEMENT (Logique Avancée)
                    int target[2] = {closestEnemy->getX(), closestEnemy->getY()};

                    if (closestDistance < entity.getSightRadius()){

                        if (isRanged) {
                            // *** COMPORTEMENT RANGED : KITING / ERRANCE ***
                            const int idealRange = attackRange * 2 / 3;

                            if (closestDistance < idealRange) {
                                // 1. Trop près -> FUIR (Kiting)
                                int fleeTarget[2] = {
                                        entity.getX() + (entity.getX() - closestEnemy->getX()) * 2,
                                        entity.getY() + (entity.getY() - closestEnemy->getY()) * 2
                                };
                                entity.chooseDirection(fleeTarget);
                            }
                            else if (closestDistance < attackRange) {
                                // 2. Distance idéale -> ERRER (WANDER)
                                entity.chooseDirection(nullptr);
                            }
                            else {
                                // 3. Hors de portée -> AVANCER
                                entity.chooseDirection(target);
                            }

                        } else {
                            // *** COMPORTEMENT MELEE : ARRÊT ET FRAPPE ***
                            if (closestDistance < attackRange) {
                                // 1. À portée -> S'ARRÊTER
                                int stopTarget[2] = {entity.getX(), entity.getY()};
                                entity.chooseDirection(stopTarget);
                            } else {
                                // 2. Hors de portée -> AVANCER
                                entity.chooseDirection(target);
                            }
                        }

                    } else {
                        // Ennemi hors de vue -> Mouvement aléatoire
                        entity.chooseDirection(nullptr);
                    }

                    // B. TIR/ATTAQUE
                    if (closestDistance < attackRange) {
                        Uint32 currentTime = SDL_GetTicks();

                        if (lastShotTime.find(entity.getName()) == lastShotTime.end() ||
                            currentTime > lastShotTime[entity.getName()] + cooldown) {

                            if (isRanged) {
                                // COMBAT A DISTANCE (Projectile)
                                Projectile newP(
                                        entity.getX(), entity.getY(),
                                        closestEnemy->getX(), closestEnemy->getY(),
                                        PROJECTILE_SPEED, damage, attackRange, entity.getColor(), PROJECTILE_RADIUS,
                                        entity.getName()
                                );
                                projectiles.push_back(newP);
                            } else {
                                // COMBAT CORPS A CORPS (Coup direct)
                                closestEnemy->setHealth(closestEnemy->getHealth() - damage);
                                if (closestEnemy->getHealth() <= 0 && closestEnemy->getIsAlive()) {
                                    closestEnemy->die();
                                }
                            }

                            lastShotTime[entity.getName()] = currentTime;
                        }
                    }
                } else {
                    entity.chooseDirection(nullptr);
                }
            }

            // 2. MOUVEMENT et CORRECTION DES COLLISIONS
            for (auto &entity : entities) {
                entity.update();
            }

            for (auto &entity : entities) {
                for (auto &other : entities) {
                    if (&entity != &other && other.getIsAlive()) {
                        int dx = entity.getX() - other.getX();
                        int dy = entity.getY() - other.getY();

                        float distance = std::sqrt((float)dx * dx + (float)dy * dy);
                        int minDistance = entity.getRad() + other.getRad();

                        if (distance < minDistance) {
                            float overlap = minDistance - distance;
                            float separationFactor = 0.5f;
                            float separationDistance = overlap * separationFactor;

                            float normX = (distance > 0) ? dx / distance : 1.0f;
                            float normY = (distance > 0) ? dy / distance : 0.0f;

                            entity.setX(entity.getX() + static_cast<int>(normX * separationDistance));
                            entity.setY(entity.getY() + static_cast<int>(normY * separationDistance));
                        }
                    }
                }
            }

            // 3. Mise à jour et Collision Projectile-Entité
            projectiles.erase(
                    std::remove_if(projectiles.begin(), projectiles.end(), [&](Projectile& proj) {
                        proj.update();
                        if (!proj.isAlive()) return true;

                        for (auto &entity : entities) {
                            if (entity.getIsAlive()) {
                                // Empêche le suicide
                                if (entity.getName() == proj.getShooterName()) continue;

                                int dx = proj.getX() - entity.getX();
                                int dy = proj.getY() - entity.getY();
                                float distance = std::sqrt((float)dx * dx + (float)dy * dy);

                                if (distance < proj.getRadius() + entity.getRad()) {
                                    entity.setHealth(entity.getHealth() - proj.getDamage());
                                    if (entity.getHealth() <= 0 && entity.getIsAlive()) {
                                        entity.die();
                                    }
                                    return true;
                                }
                            }
                        }
                        return false;
                    }),
                    projectiles.end()
            );

            // 4. Dessin des entités et des projectiles
            for (auto &entity : entities) {
                entity.draw(graphics.getRenderer());
            }

            for (auto &proj : projectiles) {
                proj.draw(graphics.getRenderer());
            }

            // 5. Suppression des entités mortes
            entities.erase(
                    std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) {
                        return !entity.getIsAlive();
                    }),
                    entities.end()
            );

            // Afficher le rendu
            SDL_RenderPresent(graphics.getRenderer());

            SDL_Delay(16);
        }
    }

    return 0;
}

// --- DÉFINITION DE LA FONCTION D'INITIALISATION DE LA SIMULATION ---
std::vector<Entity> initializeSimulation(int maxEntities) {
    std::vector<Entity> newEntities;
    std::srand(std::time(0));

    const int RANGED_COUNT = maxEntities / 3;

    for (int i = 0; i < maxEntities; ++i) {
        int randomRad = 10 + (std::rand() % 31);
        int randomX = randomRad + (std::rand() % (WINDOW_SIZE_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_SIZE_HEIGHT - 2 * randomRad));

        std::string name = "E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        bool isRangedGene;
        if (i < RANGED_COUNT) {
            isRangedGene = true;
        } else {
            isRangedGene = (std::rand() % 2 == 0);
        }

        // Utilisation du constructeur à 6 arguments
        newEntities.emplace_back(name, randomX, randomY, randomRad, color, isRangedGene);
    }
    return newEntities;
}