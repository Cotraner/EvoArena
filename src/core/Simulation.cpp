#include "Simulation.h"
#include "../constants.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
    const int PANEL_WIDTH = 300; // Largeur du panneau de stats
    const int SURVIVOR_COUNT = 5; // Nombre de survivants pour la reproduction

    // --- Constantes de l'algorithme génétique ---
    const int MUTATION_CHANCE_PERCENT = 5; //
    const int RAD_MUTATION_AMOUNT = 3; // +- 3
    const int GENE_MUTATION_AMOUNT = 10; // +- 10
    const float KITE_RATIO_MUTATION_AMOUNT = 0.1f;
    const int NUMBER_OF_TRAITS = 12;

    const float NEUTRAL_FLOAT = 0.0f;
    const int NEUTRAL_INT = 0;

    // réécriture manuelle du JSON
    const float FRAGILITY_MAX = 0.3f;
    const float EFFICIENCY_MAX = 0.5f;
    const float REGEN_MAX = 0.2f;
    const float MYOPIA_MAX = 0.5f;
    const float AIM_MAX = 15.0f;
    const int FERTILITY_MAX_INT = 2;
    const float AGING_MAX_FLOAT = 0.01f;

    const std::string TRAIT_NAMES[NUMBER_OF_TRAITS] = {
            "Classique",    // ID 0
            "Obèse",        // ID 1
            "Fragile",      // ID 2
            "Efficace",     // ID 3
            "Régénérateur", // ID 4
            "Myope",        // ID 5
            "Maladroit",    // ID 6
            "Fertile",      // ID 7
            "Vieillissant", // ID 8
            "Hyperactif",   // ID 9
            "Sédentaire",   // ID 10
            "Robuste"       // ID 11
    };
}

Simulation::Simulation(int maxEntities) :
        maxEntities(maxEntities),
        selectedLivingEntity(nullptr)
{
    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;
    initialize(maxEntities); // Lance la Génération 0
}

Simulation::~Simulation() {
}

// Initialise la GENERATION 0
void Simulation::initialize(int initialEntityCount) {
    currentGeneration = 0;
    entities.clear();
    projectiles.clear();
    lastShotTime.clear();
    genealogyArchive.clear();
    inspectionStack.clear();
    selectedLivingEntity = nullptr;

    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;

    std::srand(std::time(0));

    for (int i = 0; i < initialEntityCount; ++i) {
        float newGeneticCode[12];

        // 1. Initialisation des Gènes de Base
        newGeneticCode[0] = 10.0f + (float)(std::rand() % 31);
        int randomRad = (int)newGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        std::string name = "G0-E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        newGeneticCode[1] = (float)(std::rand() % 101);
        newGeneticCode[2] = (10 + (std::rand() % 91)) / 100.0f;
        newGeneticCode[10] = (std::rand() % 2 == 0) ? 1.0f : 0.0f;

        // 2. Initialisation des Nouveaux Gènes à Neutre (3 à 9)
        for(int j = 3; j <= 9; ++j) {
            newGeneticCode[j] = NEUTRAL_FLOAT;
        }

        // choix du gêne prédominant
        int dominantTraitID = std::rand() % NUMBER_OF_TRAITS;
        newGeneticCode[11] = (float)dominantTraitID;

        switch (dominantTraitID) {
            case 0:
                break;

            case 1: // Obèse
                newGeneticCode[1] = (float)(std::rand() % 99) / 100.0f * EFFICIENCY_MAX;
                break;

            case 2: // Fragile
                newGeneticCode[3] = (float)(std::rand() % 101) / 100.0f * FRAGILITY_MAX;
                break;
            case 3: // Efficace
                newGeneticCode[4] = (float)(std::rand() % 101) / 100.0f * EFFICIENCY_MAX;
                break;
            case 4: // Régénérateur
                newGeneticCode[5] = (float)(std::rand() % 101) / 100.0f * REGEN_MAX;
                break;
            case 5: // Myope
                newGeneticCode[6] = (float)(std::rand() % 101) / 100.0f * MYOPIA_MAX;
                break;
            case 6: // Maladroit
                newGeneticCode[7] = (float)(std::rand() % 101) / 100.0f * AIM_MAX;
                break;
            case 7: // Vieillissant
                newGeneticCode[8] = (float)(std::rand() % 101) / 100.0f * AGING_MAX_FLOAT;
                break;
            case 8: // Fertile
                newGeneticCode[9] = (float)(std::rand() % (FERTILITY_MAX_INT + 1));
                break;
        }


        // 4. Création de l'Entité (8 arguments)
        entities.emplace_back(
                name, randomX, randomY, color,
                newGeneticCode,
                currentGeneration,
                "NONE",
                "NONE"
        );
    }
}

//Logique de Reproduction
void Simulation::triggerReproduction(const std::vector<Entity>& parents) {
    std::vector<Entity> newGeneration;
    int numParents = parents.size();

    // Si 'parents' est vide
    if (parents.empty() || numParents < 2) {
        initialize(this->maxEntities); // Reviens à G0
        return;
    }

    int newGen = parents[0].getGeneration() + 1;
    this->currentGeneration = newGen;

    for (int i = 0; i < maxEntities; ++i) {
        // 1. Sélection des parents
        const Entity& parent1 = parents[std::rand() % numParents];
        const Entity* parent2_ptr = &parents[std::rand() % numParents];

        // S'assurer que Parent 2 est différent de Parent 1 (si possible)
        if (numParents > 1) {
            while (parent1.getName() == parent2_ptr->getName()) {
                parent2_ptr = &parents[std::rand() % numParents];
            }
        }
        const Entity& parent2 = *parent2_ptr;

        float childGeneticCode[12];

        // 2. Crossover et Mutation des 12 Gènes (Indices 0 à 11)
        for (int j = 0; j < 12; ++j) {
            // Crossover: 50% de chance d'hériter de Parent 1 ou Parent 2
            if (std::rand() % 2 == 0) {
                childGeneticCode[j] = parent1.getGeneticCode()[j];
            } else {
                childGeneticCode[j] = parent2.getGeneticCode()[j];
            }

            // Mutation: 5% de chance de muter
            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                if (j == 11) { // Gène ID (Discret)
                    int currentID = (int)std::round(childGeneticCode[j]);
                    int newID;
                    // Muter vers un ID aléatoire différent
                    do {
                        newID = std::rand() % NUMBER_OF_TRAITS;
                    } while (newID == currentID && NUMBER_OF_TRAITS > 1);
                    childGeneticCode[j] = (float)newID;
                }
                else if (j == 0) { // Rad (Continu)
                    childGeneticCode[j] += (float)((std::rand() % (2 * RAD_MUTATION_AMOUNT + 1)) - RAD_MUTATION_AMOUNT);
                    childGeneticCode[j] = std::max(10.0f, std::min(40.0f, childGeneticCode[j])); // Clamp rad
                }
                else if (j == 10) { // IsRanged (Binaire)
                    childGeneticCode[j] = (childGeneticCode[j] == 0.0f) ? 1.0f : 0.0f;
                }
                else if (j == 2) { // KiteRatio (Continu, entre 0.1 et 1.0)
                    childGeneticCode[j] += ((std::rand() % 3) - 1) * KITE_RATIO_MUTATION_AMOUNT; // -0.1, 0, ou +0.1
                    childGeneticCode[j] = std::clamp(childGeneticCode[j], 0.1f, 1.0f);
                }
                else { // Autres gènes Floats (WeaponGene, Fragility, Efficiency, etc.)
                    float mutation = (float)((std::rand() % (2 * GENE_MUTATION_AMOUNT + 1)) - GENE_MUTATION_AMOUNT) / 100.0f;
                    childGeneticCode[j] += mutation;
                }
            }
        }

        // 3. Création de l'Entité Enfant
        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomRad = (int)childGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));
        SDL_Color newColor = parent1.getColor(); // Couleur héritée de P1

        newGeneration.emplace_back(
                newName, randomX, randomY, newColor,
                childGeneticCode, // Tableau float[12]
                newGen,
                parent1.getName(),
                parent2.getName()
        );
    }

    entities = std::move(newGeneration);
}

void Simulation::triggerManualRestart() {
    triggerReproduction(lastSurvivors);
}

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
                std::string p1_name = inspectionStack.back().getParent1Name(); //
                if (genealogyArchive.count(p1_name)) { // "count" vérifie l'existence
                    inspectionStack.push_back(genealogyArchive.at(p1_name)); // Ajouter le parent 1
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

        if (clickHandled) return;

        // Gérer les clics sur les entités
        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;

            int dx = mouseX - entity.getX();
            int dy = mouseY - entity.getY();
            float distance = std::sqrt((float)dx * dx + (float)dy * dy);

            if (distance < entity.getRad()) {
                selectedLivingEntity = &entity;
                inspectionStack.clear();
                inspectionStack.push_back(entity); // Ajouter l'entité vivante
                entityClicked = true;
                break;
            }
        }

        //Clic dans le vide
        if (!entityClicked) {
            selectedLivingEntity = nullptr;
            inspectionStack.clear(); // Vider le panneau
        }
    }
}

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

        // Gère l'auto-redémarrage
        if (autoRestart) {
            triggerReproduction(lastSurvivors);
            return SimUpdateStatus::RUNNING; // Continue
        } else {
            // S'arrête et attend l'intervention manuelle
            return SimUpdateStatus::FINISHED;
        }
    }

    //Animation du panneau
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

void Simulation::updateLogic(int speedMultiplier) {
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
    // Appliquer le mouvement
    for (auto &entity : entities) {
        entity.update(speedMultiplier);
    }
    // Corriger les collisions (séparation)
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
    entities.erase(
            std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) {
                return !entity.getIsAlive();
            }),
            entities.end()
    );
}

void Simulation::render(SDL_Renderer *renderer, bool showDebug) {
    // Dessiner les objets du jeu
    for (auto &entity : entities) {
        entity.draw(renderer, showDebug);
    }
    for (auto &proj : projectiles) {
        proj.draw(renderer);
    }

    // Dessiner la surbrillance (sur l'entité vivante)
    if (selectedLivingEntity != nullptr) {
        circleRGBA(renderer,
                   selectedLivingEntity->getX(),
                   selectedLivingEntity->getY(),
                   selectedLivingEntity->getRad() + 4,
                   255, 255, 0, 200);
    }

    // Dessiner le panneau (s'il est en cours d'affichage)
    if (panelCurrentX < WINDOW_WIDTH) {
        drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
    }
}

void Simulation::drawStatsPanel(SDL_Renderer* renderer, int panelX) {

    // Utilitaire lambda pour la conversion float/string
    auto float_to_string = [](float val, int precision = 2) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    };

    // Si la pile est vide, ne rien dessiner
    if (inspectionStack.empty()) return;

    const Entity* entityToDisplay = nullptr;

    // Détermine si on affiche les stats en temps réel ou une copie archivée.
    if (inspectionStack.size() == 1 && selectedLivingEntity != nullptr) {
        entityToDisplay = selectedLivingEntity;
    } else {
        entityToDisplay = &inspectionStack.back();
    }

    if (entityToDisplay == nullptr) return;

    // 'entity' est la référence constante aux données
    const Entity& entity = *entityToDisplay;


    // --- LOGIQUE D'IDENTIFICATION DU TRAIT DOMINANT ---
    std::string dominantTrait = "Inconnu";
    int traitID = entity.getCurrentTraitID();

    // Utilisation du tableau statique TRAIT_NAMES
    const int NUMBER_OF_TRAITS = 12; // Définie dans le namespace
    if (traitID >= 0 && traitID < NUMBER_OF_TRAITS) {
        dominantTrait = TRAIT_NAMES[traitID];
    } else {
        dominantTrait = "Invalide (" + std::to_string(traitID) + ")";
    }


    // 1. Fond du panneau
    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 200);
    SDL_RenderFillRect(renderer, &panelRect);

    // 2. Définitions de style
    int x = panelX + 15;
    int y = 20;
    const int lineHeight = 18;
    const SDL_Color titleColor = {255, 255, 0, 255};
    const SDL_Color statColor = {255, 255, 255, 255};
    const SDL_Color geneColor = {150, 200, 255, 255};
    const SDL_Color linkColor = {100, 100, 255, 255};

    // --- Entête et Bouton Retour
    if (inspectionStack.size() > 1) {
        std::string backText = "< Back";
        stringRGBA(renderer, x + 10, y + 8, backText.c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += 40;
    }

    // --- BLOC 1 : STATS DE BASE ET GÉNÉALOGIE ---
    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255);
    y += lineHeight * 1.5;

    // Santé et Stamina
    if (entityToDisplay == selectedLivingEntity) {
        stringRGBA(renderer, x, y, ("Health: " + std::to_string(entity.getHealth()) + "/" + std::to_string(entity.getMaxHealth())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    } else {
        stringRGBA(renderer, x, y, "Health: (Decede)", 150, 150, 150, 255);
    }
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina()) + "/" + std::to_string(entity.getMaxStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight * 1.5;

    // Généalogie
    stringRGBA(renderer, x, y, "--- Genealogie ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Generation: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;

    // Liens Parents
    stringRGBA(renderer, x, y, ("Parent 1: " + entity.getParent1Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Parent 2: " + entity.getParent2Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    y += lineHeight * 1.5;

    // --- BLOC 2 : TRAIT DOMINANT ET CORPS ---
    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;

    // AFFICHAGE DU TRAIT DOMINANT
    stringRGBA(renderer, x, y, ("Dominant Trait: " + dominantTrait).c_str(), titleColor.r, titleColor.g, titleColor.b, 255);
    y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor() * 100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Sight Radius: " + std::to_string(entity.getSightRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight * 1.5;

    // --- BLOC 3 : GÈNES BRUTS (Bio-Inspirés) ---
    stringRGBA(renderer, x, y, "--- Genetic Traits ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, (std::string("Type: ") + (entity.getIsRanged() ? "Ranged" : "Melee")).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Weapon Gene: " + float_to_string(entity.getWeaponGene())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Kite Ratio: " + float_to_string(entity.getKiteRatio(), 2)).c_str(), geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight * 1.5;


    // --- AFFICHAGE DES BIO-TRAITS SIMPLIFIÉS ET TRADUITS (Correction des ID de Trait) ---
    stringRGBA(renderer, x, y, "--- Bio-Traits ---", geneColor.r, geneColor.g, geneColor.b, 255);
    y += lineHeight;

    const SDL_Color impactColor = {255, 100, 100, 255};
    const SDL_Color bonusColor = {100, 255, 100, 255};


    // --- AFFICHAGE DES TRAITS NON-FLOAT (Basés sur l'ID : 0, 1, 9, 10, 11) ---
    if (traitID == 1) { // Obèse
        stringRGBA(renderer, x, y, "Obese: -20.0% Stamina Max", impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }
    else if (traitID == 9) { // Hyperactif
        stringRGBA(renderer, x, y, "Hyperactif: +20.0% Vitesse", bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, "            x2.0 Cout Stamina Mouvement", impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }
    else if (traitID == 10) { // Sédentaire
        stringRGBA(renderer, x, y, "Sedentaire: -30.0% Vitesse", impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, "            +15.0% Stamina Max", bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
    }
    else if (traitID == 11) { // Robuste
        stringRGBA(renderer, x, y, "Robuste: Defense Base augmentee", bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
    }
    else if (traitID == 0) { // Classique
        stringRGBA(renderer, x, y, "Classique: Neutre", statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
    }


    // --- AFFICHAGE DES TRAITS

    // 1. Damage_Fragility (ID 2)
    float fragPercent = entity.getDamageFragility() * 100.0f;
    if (fragPercent > 0.001f) {
        std::string fragStr = "Fragilite: +" + float_to_string(fragPercent, 1) + "% Degats Subis";
        stringRGBA(renderer, x, y, fragStr.c_str(), impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }

    // 2. Stamina_Efficiency (ID 3)
    float effPercent = entity.getStaminaEfficiency() * 100.0f;
    if (effPercent > 0.001f) {
        std::string effStr = "Stamina Eff: -" + float_to_string(effPercent, 1) + "% Cout";
        stringRGBA(renderer, x, y, effStr.c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
    }

    // 3. Base_Health_Regen (ID 4)
    float regenRate = entity.getBaseHealthRegen();
    if (regenRate > 0.0001f) {
        std::string regenStr = "Regeneration: +" + float_to_string(regenRate, 3) + " HP/cycle";
        stringRGBA(renderer, x, y, regenStr.c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
    }

    // 4. MyopiaFactor (ID 5)
    float myopiaPercent = entity.getMyopiaFactor() * 100.0f;
    if (myopiaPercent > 0.001f) {
        std::string myopiaStr = "Myopie: -" + float_to_string(myopiaPercent, 1) + "% Vision";
        stringRGBA(renderer, x, y, myopiaStr.c_str(), impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }

    // 5. AimingPenalty (ID 6)
    float aimPenalty = entity.getAimingPenalty();
    if (entity.getIsRanged() && aimPenalty > 0.001f) {
        std::string aimStr = "Maladresse: +" + float_to_string(aimPenalty, 1) + " deg Erreur";
        stringRGBA(renderer, x, y, aimStr.c_str(), impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }

    // 6. FertilityFactor (ID 7)
    int fertility = entity.getFertilityFactor();
    if (fertility > 0) {
        float healthCost = fertility * 5.0f;
        std::string fertStr = "Fecondite: +" + std::to_string(fertility) + " Enfants (Cout -" + float_to_string(healthCost, 1) + "% PV Initiaux)";
        stringRGBA(renderer, x, y, fertStr.c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255);
        y += lineHeight;
    }

    // 7. AgingRate (ID 8)
    float agingRate = entity.getAgingRate();
    if (agingRate > 0.00001f) {
        float agingPerThousand = agingRate * 1000.0f;
        std::string agingStr = "Vieillissement: -" + float_to_string(agingPerThousand, 3) + "%o Max PV/cycle";
        stringRGBA(renderer, x, y, agingStr.c_str(), impactColor.r, impactColor.g, impactColor.b, 255);
        y += lineHeight;
    }

    y += 10;

    // --- BLOC 4 : STATS D'ARMEMENT ---
    stringRGBA(renderer, x, y, "--- Weapon Stats ---", statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Damage: " + std::to_string(entity.getDamage())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Range: " + std::to_string(entity.getAttackRange())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Cooldown: " + std::to_string(entity.getAttackCooldown()) + " ms").c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina Cost: " + std::to_string(entity.getStaminaAttackCost())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    y += lineHeight;

    if (entity.getIsRanged()) {
        stringRGBA(renderer, x, y, ("Proj Speed: " + std::to_string(entity.getProjectileSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, ("Proj Radius: " + std::to_string(entity.getProjectileRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    }
}