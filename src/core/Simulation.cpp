#include "Simulation.h"
#include "../constants.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
    const int PANEL_WIDTH = 300; // Largeur du panneau de statistiques
    const int SURVIVOR_COUNT = 5; // Nombre de survivants pour la reproduction

    // --- Constantes de l'algorithme génétique ---
    const int MUTATION_CHANCE_PERCENT = 5; //
    const int RAD_MUTATION_AMOUNT = 3; // +- 3
    const int GENE_MUTATION_AMOUNT = 10; // +- 10
    const float KITE_RATIO_MUTATION_AMOUNT = 0.1f;
}

// --- CONSTRUCTEUR ET INITIALISATION ---

Simulation::Simulation(int maxEntities) :
        maxEntities(maxEntities), // Stocke le max
        selectedLivingEntity(nullptr)
{
    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;
    initialize(maxEntities); // Lance la Génération 0
}

Simulation::~Simulation() {
    // Le destructeur est vide
}

// Initialise la GENERATION 0
void Simulation::initialize(int initialEntityCount) {
    currentGeneration = 0;
    entities.clear();
    projectiles.clear();
    lastShotTime.clear();
    genealogyArchive.clear(); // Vide l'historique
    inspectionStack.clear(); // Vide la pile d'inspection
    selectedLivingEntity = nullptr;

    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;

    std::srand(std::time(0));
    const int RANGED_COUNT = initialEntityCount / 3;

    for (int i = 0; i < initialEntityCount; ++i) {
        int randomRad = 10 + (std::rand() % 31);
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        std::string name = "G0-E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        int randomWeaponGene = std::rand() % 101; // Gène 0-100
        float randomKiteRatio = (10 + (std::rand() % 91)) / 100.0f;

        bool isRangedGene = (i < RANGED_COUNT) ? true : (std::rand() % 2 == 0);

        // Appel au constructeur modifié pour la Gen 0
        entities.emplace_back(name, randomX, randomY, randomRad, color, isRangedGene,
                              0, "NONE", "NONE",
                              randomWeaponGene, randomKiteRatio);
    }
}

// --- NOUVEAU : Logique de Reproduction ---
void Simulation::triggerReproduction(const std::vector<Entity> &parents) {
    entities.clear();
    projectiles.clear();
    lastShotTime.clear();
    inspectionStack.clear();
    selectedLivingEntity = nullptr;

    // Si 'parents' est vide (ex: redémarrage), repartir de zéro
    if (parents.empty()) {
        initialize(this->maxEntities);
        return;
    }

    int newGen = parents[0].getGeneration() + 1;
    this->currentGeneration = newGen;

    for (int i = 0; i < maxEntities; ++i) {
        // 1. Sélection des parents
        const Entity& parent1 = parents[std::rand() % parents.size()];

        const Entity* parent2_ptr = &parents[std::rand() % parents.size()];

        // S'il y a plus d'un parent, s'assurer qu'ils sont différents
        if (parents.size() > 1) {
            while (parent1.getName() == parent2_ptr->getName()) {
                // Re-tirer le parent 2
                parent2_ptr = &parents[std::rand() % parents.size()];
            }
        }
        const Entity& parent2 = *parent2_ptr;

        // 2. Enjambement (Crossover)
        float crossoverRad = (std::rand() % 101) / 100.0f;
        int newRad = (int)(parent1.getRad() * crossoverRad + parent2.getRad() * (1.0f - crossoverRad));

        float crossoverGene = (std::rand() % 101) / 100.0f;
        int newWeaponGene = (int)(parent1.getWeaponGene() * crossoverGene + parent2.getWeaponGene() * (1.0f - crossoverGene));

        bool newIsRanged = (std::rand() % 2 == 0) ? parent1.getIsRanged() : parent2.getIsRanged();

        // --- NOUVEAU : Crossover pour Kite Ratio ---
        float crossoverKite = (std::rand() % 101) / 100.0f;
        float newKiteRatio = (parent1.getKiteRatio() * crossoverKite) + (parent2.getKiteRatio() * (1.0f - crossoverKite));

        // 3. Mutation
        if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
            newRad += (std::rand() % (RAD_MUTATION_AMOUNT * 2 + 1)) - RAD_MUTATION_AMOUNT;
        }
        if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
            newWeaponGene += (std::rand() % (GENE_MUTATION_AMOUNT * 2 + 1)) - GENE_MUTATION_AMOUNT;
        }
        // --- NOUVEAU : Mutation pour Kite Ratio ---
        if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
            newKiteRatio += (std::rand() % 3 - 1) * KITE_RATIO_MUTATION_AMOUNT; // -0.1, 0, ou +0.1
        }

        // Clamp values
        newRad = std::clamp(newRad, 10, 40);
        newWeaponGene = std::clamp(newWeaponGene, 0, 100);
        newKiteRatio = std::clamp(newKiteRatio, 0.1f, 1.0f); // Reste entre 10% et 100%

        // 4. Création
        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomX = newRad + (std::rand() % (WINDOW_WIDTH - 2 * newRad));
        int randomY = newRad + (std::rand() % (WINDOW_HEIGHT - 2 * newRad));
        SDL_Color newColor = parent1.getColor();

        // Appel au constructeur modifié
        entities.emplace_back(newName, randomX, randomY, newRad, newColor, newIsRanged,
                              newGen, parent1.getName(), parent2.getName(),
                              newWeaponGene, newKiteRatio); // Gènes passés
    }
}

void Simulation::triggerManualRestart() {
    // Utilise les survivants stockés pour lancer la prochaine génération
    triggerReproduction(lastSurvivors);
}

// --- GESTION DES ÉVÉNEMENTS (MODIFIÉE) ---

void Simulation::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        SDL_Point mousePoint = {mouseX, mouseY};

        bool clickHandled = false;

        // 1. Gérer les clics sur le panneau d'inspection (si ouvert)
        if (!inspectionStack.empty() && mouseX > panelCurrentX) {

            if (SDL_PointInRect(&mousePoint, &panelBack_rect) && inspectionStack.size() > 1) {
                inspectionStack.pop_back(); // Revenir en arrière
                clickHandled = true;
            }
            else if (SDL_PointInRect(&mousePoint, &panelParent1_rect)) {
                std::string p1_name = inspectionStack.back().getParent1Name();
                if (genealogyArchive.count(p1_name)) { // "count" vérifie si la clé existe
                    inspectionStack.push_back(genealogyArchive.at(p1_name)); // ".at" récupère la valeur
                    clickHandled = true;
                }
            }
            else if (SDL_PointInRect(&mousePoint, &panelParent2_rect)) {
                std::string p2_name = inspectionStack.back().getParent2Name();
                if (genealogyArchive.count(p2_name)) {
                    inspectionStack.push_back(genealogyArchive.at(p2_name));
                    clickHandled = true;
                }
            }
        }

        if (clickHandled) return; // Clic sur l'UI, ne pas vérifier les entités

        // 2. Gérer les clics sur les entités
        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;

            int dx = mouseX - entity.getX();
            int dy = mouseY - entity.getY();
            float distance = std::sqrt((float)dx * dx + (float)dy * dy);

            if (distance < entity.getRad()) {
                selectedLivingEntity = &entity;
                inspectionStack.clear(); // Nouvelle sélection, vider la pile
                inspectionStack.push_back(entity); // Ajouter l'entité vivante
                entityClicked = true;
                break;
            }
        }

        // 3. Clic dans le vide
        if (!entityClicked) {
            selectedLivingEntity = nullptr;
            inspectionStack.clear(); // Vider le panneau
        }
    }
}

// --- BOUCLE DE MISE À JOUR (MODIFIÉE) ---

Simulation::SimUpdateStatus Simulation::update(int speedMultiplier, bool autoRestart) {
    updateLogic(speedMultiplier);
    updatePhysics(speedMultiplier);
    updateProjectiles();
    cleanupDead();

    // Déclencher la reproduction
    if (entities.size() <= SURVIVOR_COUNT && !entities.empty()) {

        lastSurvivors = entities; // Stocke les gagnants

        // Archiver les gagnants
        for (const auto& winner : lastSurvivors) {
            genealogyArchive.insert({winner.getName(), winner});
        }

        // --- MODIFIÉ : Gère l'auto-redémarrage ---
        if (autoRestart) {
            triggerReproduction(lastSurvivors);
            return SimUpdateStatus::RUNNING; // Continue
        } else {
            // S'arrête et attend l'intervention manuelle
            return SimUpdateStatus::FINISHED;
        }
    }

    // --- Animation du panneau ---
    if (!inspectionStack.empty()) {
        panelTargetX = (float)(WINDOW_WIDTH - PANEL_WIDTH);
    } else {
        panelTargetX = (float)WINDOW_WIDTH;
    }
    if(selectedLivingEntity != nullptr && !selectedLivingEntity->getIsAlive()) {
        selectedLivingEntity = nullptr;
    }
    float distance = panelTargetX - panelCurrentX;
    if (std::abs(distance) < 1.0f) {
        panelCurrentX = panelTargetX;
    } else {
        panelCurrentX += distance * 0.1f;
    }

    return SimUpdateStatus::RUNNING; // La simulation continue
}

// ... (updateLogic, updatePhysics, updateProjectiles, cleanupDead sont inchangés) ...
void Simulation::updateLogic(int speedMultiplier) {
    // ... (code inchangé)
    for (auto &entity : entities) {
        if (!entity.getIsAlive()) continue;

        Entity* closestEnemy = nullptr;
        auto closestDistance = (float)(WINDOW_WIDTH + WINDOW_HEIGHT);
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
                Uint32 effectiveCooldown = (speedMultiplier > 0) ? (cooldown / speedMultiplier) : cooldown;
                if (lastShotTime.find(entity.getName()) == lastShotTime.end() ||
                    currentTime > lastShotTime[entity.getName()] + effectiveCooldown) {
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
void Simulation::updatePhysics(int speedMultiplier) {
    // ... (code inchangé)
    // 1. Appliquer le mouvement
    for (auto &entity : entities) {
        entity.update(speedMultiplier);
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
    // ... (code inchangé)
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
    // ... (code inchangé)
    entities.erase(
            std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) {
                return !entity.getIsAlive();
            }),
            entities.end()
    );
}


// --- BOUCLE DE RENDU (MODIFIÉE) ---

void Simulation::render(SDL_Renderer *renderer, bool showDebug) {
    // 1. Dessiner les objets du jeu
    for (auto &entity : entities) {
        entity.draw(renderer, showDebug);
    }
    for (auto &proj : projectiles) {
        proj.draw(renderer);
    }

    // 2. Dessiner la surbrillance (sur l'entité vivante)
    if (selectedLivingEntity != nullptr) {
        circleRGBA(renderer,
                   selectedLivingEntity->getX(),
                   selectedLivingEntity->getY(),
                   selectedLivingEntity->getRad() + 4,
                   255, 255, 0, 200);
    }

    // 3. Dessiner le panneau (s'il est en cours d'affichage)
    if (panelCurrentX < WINDOW_WIDTH) {
        drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
    }
}

// --- drawStatsPanel (MODIFIÉ) ---
void Simulation::drawStatsPanel(SDL_Renderer* renderer, int panelX) {

    // Si la pile est vide, ne rien dessiner
    if (inspectionStack.empty()) return;

    // --- DÉBUT DE LA CORRECTION ---

    const Entity* entityToDisplay = nullptr;

    // Si nous inspectons l'entité vivante sélectionnée (la base de la pile ET elle est sélectionnée)
    if (inspectionStack.size() == 1 && selectedLivingEntity != nullptr) {
        // Nous utilisons le POINTEUR "live" pour obtenir les données en temps réel
        entityToDisplay = selectedLivingEntity;
    } else {
        // Sinon (nous inspectons un ancêtre ou l'entité vivante est morte),
        // nous utilisons la COPIE statique du sommet de la pile
        entityToDisplay = &inspectionStack.back();
    }

    // (Sécurité au cas où, bien que cela ne devrait pas arriver)
    if (entityToDisplay == nullptr) return;

    // 'entity' est maintenant une référence à la bonne source (soit "live", soit "copie")
    const Entity& entity = *entityToDisplay;

    // --- FIN DE LA CORRECTION ---


    // 1. Fond du panneau
    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 200);
    SDL_RenderFillRect(renderer, &panelRect);

    // 2. Texte (Utilise 'entity' comme avant)
    int x = panelX + 15;
    int y = 20;
    const int lineHeight = 18;
    const SDL_Color titleColor = {255, 255, 0, 255};
    const SDL_Color statColor = {255, 255, 255, 255};
    const SDL_Color geneColor = {150, 200, 255, 255};
    const SDL_Color linkColor = {100, 100, 255, 255};

    auto float_to_string = [](float val, int precision = 2) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    };

    // Bouton Retour
    if (inspectionStack.size() > 1) {
        // ... (code inchangé)
        std::string backText = "< Back";
        panelBack_rect = {x, y, 70, 25};
        boxRGBA(renderer, panelBack_rect.x, panelBack_rect.y, panelBack_rect.x + panelBack_rect.w, panelBack_rect.y + panelBack_rect.h, 80, 80, 80, 255);
        stringRGBA(renderer, x + 10, y + 8, backText.c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += 40;
    }

    // Affichage des stats
    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255);
    y += lineHeight * 1.5;

    // --- MODIFIÉ : Simplification de la logique d'affichage des PV ---
    // Si nous regardons le pointeur "live", il est vivant.
    // Si nous regardons un ancêtre, il est mort (et n'est pas 'selectedLivingEntity')
    if (entityToDisplay == selectedLivingEntity) {
        stringRGBA(renderer, x, y, ("Health: " + std::to_string(entity.getHealth()) + "/" + std::to_string(entity.getMaxHealth())).c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
    } else {
        // C'est un ancêtre
        stringRGBA(renderer, x, y, "Health: (Deceased)", 150, 150, 150, 255);
        y += lineHeight;
    }

    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina()) + "/" + std::to_string(entity.getMaxStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight * 1.5;

    // --- NOUVEAU : Généalogie ---
    stringRGBA(renderer, x, y, "--- Genealogy ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Generation: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;

    std::string p1_name = entity.getParent1Name();
    std::string p2_name = entity.getParent2Name();

    // Lien Parent 1
    panelParent1_rect = {x, y, 270, 15}; // Définir la zone cliquable
    stringRGBA(renderer, x, y, ("Parent 1: " + p1_name).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    y += lineHeight;

    // Lien Parent 2
    panelParent2_rect = {x, y, 270, 15};
    stringRGBA(renderer, x, y, ("Parent 2: " + p2_name).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    y += lineHeight * 1.5;


    // --- Gènes (Corps) ---
    stringRGBA(renderer, x, y, "--- Body Genes ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, (std::string("Type: ") + (entity.getIsRanged() ? "Ranged" : "Melee")).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;

    // --- Stats (Corps) ---
    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor() * 100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Regen: " + std::to_string(entity.getRegenAmount()) + " HP/tick").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Sight Radius: " + std::to_string(entity.getSightRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight * 1.5;

    // --- Gènes (Arme) ---
    stringRGBA(renderer, x, y, "--- Weapon Genes ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Weapon Gene (0-100): " + std::to_string(entity.getWeaponGene())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, ("Kite Ratio: " + float_to_string(entity.getKiteRatio(), 2)).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;

    // --- Stats (Arme) ---
    stringRGBA(renderer, x, y, "--- Weapon Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Damage: " + std::to_string(entity.getDamage())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Range: " + std::to_string(entity.getAttackRange())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Cooldown: " + std::to_string(entity.getAttackCooldown()) + " ms").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina Cost: " + std::to_string(entity.getStaminaAttackCost())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;

    if (entity.getIsRanged()) {
        stringRGBA(renderer, x, y, ("Projectile Speed: " + std::to_string(entity.getProjectileSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, ("Projectile Radius: " + std::to_string(entity.getProjectileRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    }
}