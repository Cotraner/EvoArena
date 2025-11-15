#include "Simulation.h"
#include "../constants.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
    const int PANEL_WIDTH = 300; // Largeur du panneau de statistiques
}

// --- CONSTRUCTEUR ET INITIALISATION ---

Simulation::Simulation(int maxEntities) : selectedEntity(nullptr) {
    // Initialise le panneau en position "cachée"
    panelCurrentX = (float)WINDOW_SIZE_WIDTH;
    panelTargetX = (float)WINDOW_SIZE_WIDTH;
    initialize(maxEntities);
}

Simulation::~Simulation() {
    // Le destructeur est vide
}

void Simulation::initialize(int maxEntities) {
    entities.clear();
    projectiles.clear();
    lastShotTime.clear();
    selectedEntity = nullptr;

    // Réinitialise la position du panneau au cas où
    panelCurrentX = (float)WINDOW_SIZE_WIDTH;
    panelTargetX = (float)WINDOW_SIZE_WIDTH;

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

        entities.emplace_back(name, randomX, randomY, randomRad, color, isRangedGene);
    }
}

// --- GESTION DES ÉVÉNEMENTS ---

void Simulation::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;

        // --- SUPPRIMÉ : Le blocage des clics a été retiré ---
        // if (selectedEntity != nullptr && mouseX >= (WINDOW_SIZE_WIDTH - PANEL_WIDTH)) {
        //     return;
        // }

        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;

            int dx = mouseX - entity.getX();
            int dy = mouseY - entity.getY();
            float distance = std::sqrt((float)dx * dx + (float)dy * dy);

            if (distance < entity.getRad()) {
                selectedEntity = &entity;
                entityClicked = true;
                break;
            }
        }
        if (!entityClicked) {
            selectedEntity = nullptr;
        }
    }
}

// --- BOUCLE DE MISE À JOUR ---

void Simulation::update() {
    updateLogic();
    updatePhysics();
    updateProjectiles();
    cleanupDead();

    // --- NOUVEAU : Logique d'animation du panneau ---

    // 1. Mettre à jour la cible
    if (selectedEntity != nullptr && selectedEntity->getIsAlive()) {
        panelTargetX = (float)(WINDOW_SIZE_WIDTH - PANEL_WIDTH); // Cible = visible
    } else {
        if (selectedEntity != nullptr && !selectedEntity->getIsAlive()) {
            selectedEntity = nullptr; // Nettoyage au cas où
        }
        panelTargetX = (float)WINDOW_SIZE_WIDTH; // Cible = cachée
    }

    // 2. Interpoler la position actuelle vers la cible
    float distance = panelTargetX - panelCurrentX;

    // Si la distance est très faible, "snapper" en position
    if (std::abs(distance) < 1.0f) {
        panelCurrentX = panelTargetX;
    } else {
        // Vitesse de glissement (10% de la distance restante par frame)
        panelCurrentX += distance * 0.1f;
    }
}

void Simulation::updateLogic() {
    // ... (Aucun changement dans updateLogic) ...
    for (auto &entity : entities) {
        if (!entity.getIsAlive()) continue;

        Entity* closestEnemy = nullptr;
        auto closestDistance = (float)(WINDOW_SIZE_WIDTH + WINDOW_SIZE_HEIGHT);
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
            int attackRange = entity.getAttackRange();
            Uint32 cooldown = entity.getAttackCooldown();
            int damage = entity.getDamage();
            int target[2] = {closestEnemy->getX(), closestEnemy->getY()};

            if (closestDistance < entity.getSightRadius()){
                if (isRanged) {
                    const int idealRange = attackRange * 2 / 3;
                    if (closestDistance < idealRange) {
                        if (entity.getStamina() > 0) {
                            int fleeTarget[2] = {
                                    entity.getX() + (entity.getX() - closestEnemy->getX()) * 2,
                                    entity.getY() + (entity.getY() - closestEnemy->getY()) * 2
                            };
                            entity.chooseDirection(fleeTarget);
                            entity.setIsFleeing(true);
                        } else {
                            entity.chooseDirection(nullptr);
                            entity.setIsFleeing(false);
                        }
                    }
                    else if (closestDistance < attackRange) {
                        entity.chooseDirection(nullptr);
                        entity.setIsFleeing(false);
                    }
                    else {
                        entity.chooseDirection(target);
                        entity.setIsFleeing(false);
                    }
                } else {
                    entity.setIsFleeing(false);
                    if (closestDistance < attackRange) {
                        int stopTarget[2] = {entity.getX(), entity.getY()};
                        entity.chooseDirection(stopTarget);
                    } else {
                        entity.chooseDirection(target);
                    }
                }
            } else {
                entity.chooseDirection(nullptr);
                entity.setIsFleeing(false);
            }

            if (closestDistance < attackRange) {
                Uint32 currentTime = SDL_GetTicks();
                if (lastShotTime.find(entity.getName()) == lastShotTime.end() ||
                    currentTime > lastShotTime[entity.getName()] + cooldown) {
                    if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                        if (isRanged) {
                            Projectile newP(
                                    entity.getX(), entity.getY(),
                                    closestEnemy->getX(), closestEnemy->getY(),
                                    entity.getProjectileSpeed(),
                                    damage,
                                    attackRange,
                                    entity.getColor(),
                                    entity.getProjectileRadius(),
                                    entity.getName()
                            );
                            projectiles.push_back(newP);
                        } else {
                            closestEnemy->takeDamage(damage);
                        }
                        lastShotTime[entity.getName()] = currentTime;
                    }
                }
            }
        } else {
            entity.chooseDirection(nullptr);
            entity.setIsFleeing(false);
        }
    }
}

void Simulation::updatePhysics() {
    // ... (Aucun changement dans updatePhysics) ...
    // 1. Appliquer le mouvement
    for (auto &entity : entities) {
        entity.update();
    }

    // 2. Corriger les collisions (séparation)
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
}

void Simulation::updateProjectiles() {
    // ... (Aucun changement dans updateProjectiles) ...
    projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(), [&](Projectile& proj) {
                proj.update();
                if (!proj.isAlive()) return true;
                for (auto &entity : entities) {
                    if (entity.getIsAlive()) {
                        if (entity.getName() == proj.getShooterName()) continue;
                        int dx = proj.getX() - entity.getX();
                        int dy = proj.getY() - entity.getY();
                        float distance = std::sqrt((float)dx * dx + (float)dy * dy);
                        if (distance < proj.getRadius() + entity.getRad()) {
                            entity.takeDamage(proj.getDamage());
                            return true;
                        }
                    }
                }
                return false;
            }),
            projectiles.end()
    );
}

void Simulation::cleanupDead() {
    // ... (Aucun changement dans cleanupDead) ...
    entities.erase(
            std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) {
                return !entity.getIsAlive();
            }),
            entities.end()
    );
}

// --- BOUCLE DE RENDU ---

void Simulation::render(SDL_Renderer *renderer, bool showDebug) {
    // 1. Dessiner les objets du jeu
    for (auto &entity : entities) {
        // --- MODIFIÉ : Transmet le booléen ---
        entity.draw(renderer, showDebug);
    }

    for (auto &proj : projectiles) {
        proj.draw(renderer);
    }

    // 2. Dessiner la surbrillance (si sélectionnée)
    if (selectedEntity != nullptr) {
        circleRGBA(renderer,
                   selectedEntity->getX(),
                   selectedEntity->getY(),
                   selectedEntity->getRad() + 4,
                   255, 255, 0, 200);
    }

    // 3. Dessiner le panneau (toujours, il se cachera tout seul)
    if (panelCurrentX < WINDOW_SIZE_WIDTH) {
        drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
    }
}

// --- SIGNATURE MODIFIÉE ---
void Simulation::drawStatsPanel(SDL_Renderer* renderer, int panelX) {
    // (Cette fonction n'est appelée que si panelX < WINDOW_SIZE_WIDTH)

    // Si l'entité n'existe plus (ex: morte et désélectionnée), ne rien dessiner
    if (!selectedEntity) return;

    const Entity& entity = *selectedEntity;

    // 1. Fond du panneau
    // --- POSITION MODIFIÉE ---
    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_SIZE_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 200);
    SDL_RenderFillRect(renderer, &panelRect);

    // 2. Texte
    // --- POSITION MODIFIÉE ---
    int x = panelX + 15;
    int y = 20;
    const int lineHeight = 18;
    const SDL_Color titleColor = {255, 255, 0, 255};
    const SDL_Color statColor = {255, 255, 255, 255};
    const SDL_Color geneColor = {150, 200, 255, 255};

    auto float_to_string = [](float val, int precision = 2) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    };

    // --- Affichage des stats (Inchangé, mais affiché par-dessus le panneau à 'panelX') ---
    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255);
    y += lineHeight * 1.5;

    stringRGBA(renderer, x, y,
               ("Health: " + std::to_string(entity.getHealth()) + "/" + std::to_string(entity.getMaxHealth())).c_str(),
               statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina()) + "/" +
                                std::to_string(entity.getMaxStamina())).c_str(), statColor.r, statColor.g, statColor.b,
               255);
    y += lineHeight * 1.5;

    // --- Gènes (Corps) ---
    stringRGBA(renderer, x, y, "--- Body Genes ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), geneColor.r, geneColor.g,
               geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, (std::string("Type: ") + (entity.getIsRanged() ? "Ranged" : "Melee")).c_str(),
               geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;

    // --- Stats (Corps) ---
    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g,
               statColor.b, 255);
    y += lineHeight;

    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor() * 100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Regen: " + std::to_string(entity.getRegenAmount()) + " HP/s").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;

    stringRGBA(renderer, x, y, ("Sight Radius: " + std::to_string(entity.getSightRadius())).c_str(), statColor.r,
               statColor.g, statColor.b, 255);
    y += lineHeight * 1.5;

    // --- Gènes (Arme) ---
    stringRGBA(renderer, x, y, "--- Weapon Genes ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;

    stringRGBA(renderer, x, y, ("Weapon Gene (0-100): " + std::to_string(entity.getWeaponGene())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;

    // --- Stats (Arme) ---
    stringRGBA(renderer, x, y, "--- Weapon Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Damage: " + std::to_string(entity.getDamage())).c_str(), statColor.r, statColor.g,
               statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Range: " + std::to_string(entity.getAttackRange())).c_str(), statColor.r,
               statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Cooldown: " + std::to_string(entity.getAttackCooldown()) + " ms").c_str(),
               statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina Cost: " + std::to_string(entity.getStaminaAttackCost())).c_str(), statColor.r,
               statColor.g, statColor.b, 255);
    y += lineHeight;

    if (entity.getIsRanged()) {
        stringRGBA(renderer, x, y, ("Projectile Speed: " + std::to_string(entity.getProjectileSpeed())).c_str(),
                   statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, ("Projectile Radius: " + std::to_string(entity.getProjectileRadius())).c_str(),
                   statColor.r, statColor.g, statColor.b, 255);
    }
}