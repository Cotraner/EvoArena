#include "Graphics.h"
#include "constants.h"
#include "Entity/Entity.h"
#include "Entity/Projectile.h"
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>
#include <ctime>

namespace {
    // Carte pour suivre le temps du dernier tir de chaque entité (par nom)
    std::map<std::string, Uint32> lastShotTime;
}

int main() {
    Graphics graphics;
    std::vector<Entity> entities;
    std::vector<Projectile> projectiles;

    // --- GENERATION ALÉATOIRE ET GARANTIE DE DIVERSITÉ (Étape 2) ---
    const int NUM_ENTITIES = 15;
    const int RANGED_COUNT = 5;

    std::srand(std::time(0));

    for (int i = 0; i < NUM_ENTITIES; ++i) {
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

        entities.emplace_back(name, randomX, randomY, randomRad, color, isRangedGene);
    }

    // --- SUPPRIMÉ : Constantes de projectiles (remplacées par des gènes) ---
    // const int PROJECTILE_SPEED = 8;
    // const int PROJECTILE_RADIUS = 8;

    SDL_Event event;
    bool running = true;

    while (running) {

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }

        // Nettoyer l'écran et dessiner le fond
        SDL_RenderClear(graphics.getRenderer());
        graphics.drawBackground();

        // 1. LOGIQUE DE CIBLAGE ET DE TIR (Détermine où l'entité doit aller et si elle doit tirer)
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

                // --- MODIFIÉ : Utilisation des getters de gènes ---
                int attackRange = entity.getAttackRange();
                Uint32 cooldown = entity.getAttackCooldown();
                int damage = entity.getDamage();


                // *** SECTION A. MOUVEMENT (MODIFIÉE POUR LE TIR EN ERRANCE) ***
                int target[2] = {closestEnemy->getX(), closestEnemy->getY()};

                if (closestDistance < entity.getSightRadius()){

                    // --- NOUVELLE LOGIQUE DE KITING ---

                    if (isRanged) {
                        // *** COMPORTEMENT RANGED ***
                        const int kiteDistance = attackRange / 3; // Définir la distance de fuite

                        if (closestDistance < kiteDistance) {
                            // 1. Trop près -> FUIR
                            int fleeTarget[2] = {
                                    entity.getX() + (entity.getX() - closestEnemy->getX()),
                                    entity.getY() + (entity.getY() - closestEnemy->getY())
                            };
                            entity.chooseDirection(fleeTarget);
                        }
                        else if (closestDistance < attackRange) {
                            // 2. Distance idéale -> BOUGER EN ERRANT (WANDER)
                            // (Au lieu de s'arrêter net)
                            entity.chooseDirection(nullptr);
                        }
                        else {
                            // 3. En vue, mais hors de portée -> AVANCER
                            entity.chooseDirection(target);
                        }

                    } else {
                        // *** COMPORTEMENT MELEE (Inchangé) ***
                        if (closestDistance < attackRange) {
                            // 1. À portée -> S'ARRÊTER ET FRAPPER
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


                // B. TIR/ATTAQUE : Si dans la portée de combat spécifique au type
                if (closestDistance < attackRange) {
                    Uint32 currentTime = SDL_GetTicks();

                    if (lastShotTime.find(entity.getName()) == lastShotTime.end() ||
                        currentTime > lastShotTime[entity.getName()] + cooldown) {

                        if (isRanged) {
                            // --- COMBAT A DISTANCE (MODIFIÉ : Utilise les gènes) ---
                            Projectile newP(
                                    entity.getX(), entity.getY(),
                                    closestEnemy->getX(), closestEnemy->getY(),
                                    entity.getProjectileSpeed(), // Gène
                                    damage,
                                    attackRange,
                                    entity.getColor(),
                                    entity.getProjectileRadius(), // Gène
                                    entity.getName()
                            );
                            projectiles.push_back(newP);
                        } else {
                            // --- COMBAT CORPS A CORPS (Coup direct) ---
                            closestEnemy->takeDamage(damage);
                        }

                        lastShotTime[entity.getName()] = currentTime;
                    }
                }
            } else {
                // Si aucun ennemi trouvé, mouvement aléatoire
                entity.chooseDirection(nullptr);
            }
        }

        // 2. MOUVEMENT et CORRECTION DES COLLISIONS (Inchangé)
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

        // 3. Mise à jour et Collision Projectile-Entité (Inchangé)
        projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(), [&](Projectile& proj) {
                    proj.update();
                    if (!proj.isAlive()) return true;

                    for (auto &entity : entities) {
                        if (entity.getName() == proj.getShooterName()) {
                            continue;
                        }
                        if (entity.getIsAlive()) {
                            int dx = proj.getX() - entity.getX();
                            int dy = proj.getY() - entity.getY();
                            float distance = std::sqrt((float)dx * dx + (float)dy * dy);

                            if (distance < proj.getRadius() + entity.getRad()) {
                                entity.takeDamage(proj.getDamage());
                                return true; // Supprimer le projectile
                            }
                        }
                    }
                    return false;
                }),
                projectiles.end()
        );

        // 4. Dessin des entités et des projectiles (Inchangé)
        for (auto &entity : entities) {
            entity.draw(graphics.getRenderer());
        }
        for (auto &proj : projectiles) {
            proj.draw(graphics.getRenderer());
        }

        // 5. Suppression des entités mortes (Inchangé)
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

    return 0;
}