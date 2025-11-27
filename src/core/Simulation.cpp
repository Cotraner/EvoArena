#include "Simulation.h"
#include "../constants.h"
#include "../Entity/TraitManager.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
    const int PANEL_WIDTH = 300;
    const int SURVIVOR_COUNT = 5;

    const int MUTATION_CHANCE_PERCENT = 5;
    const int RAD_MUTATION_AMOUNT = 3;
    const int GENE_MUTATION_AMOUNT = 10;
    const int NUMBER_OF_TRAITS = 12;

    const float NEUTRAL_FLOAT = 0.0f;
    const float FRAGILITY_MAX = 0.3f;
    const float EFFICIENCY_MAX = 0.5f;
    const float REGEN_MAX = 0.2f;
    const float MYOPIA_MAX = 0.5f;
    const float AIM_MAX = 15.0f;
    const int FERTILITY_MAX_INT = 2;
    const float AGING_MAX_FLOAT = 0.01f;
}

Simulation::Simulation(int maxEntities) :
        maxEntities(maxEntities),
        selectedLivingEntity(nullptr)
{
    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;

    TraitManager::loadTraits("../assets/json/mutations.JSON");

    initialize(maxEntities);
}

Simulation::~Simulation() {
}

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
        newGeneticCode[0] = 10.0f + (float)(std::rand() % 31);
        int randomRad = (int)newGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));
        std::string name = "G0-E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        newGeneticCode[1] = (float)(std::rand() % 101); // Weapon
        newGeneticCode[2] = (10 + (std::rand() % 91)) / 100.0f; // Kite
        newGeneticCode[10] = (std::rand() % 2 == 0) ? 1.0f : 0.0f; // Ranged

        // 2. Initialisation des Gènes BIO à Neutre
        for(int j = 3; j <= 9; ++j) { newGeneticCode[j] = NEUTRAL_FLOAT; }

        // 3. Choix du trait dominant (80% Classique, 20% Spécial)
        int dominantTraitID = 0;
        int roll = std::rand() % 100;
        if (roll >= 80) {
            dominantTraitID = 1 + (std::rand() % (NUMBER_OF_TRAITS - 1));
        }
        newGeneticCode[11] = (float)dominantTraitID;

        // Si spécial, on initialise les valeurs bio correspondantes
        switch (dominantTraitID) {
            case 2: newGeneticCode[3] = (float)(std::rand() % 101) / 100.0f * FRAGILITY_MAX; break;
            case 3: newGeneticCode[4] = (float)(std::rand() % 101) / 100.0f * EFFICIENCY_MAX; break;
            case 4: newGeneticCode[5] = (float)(std::rand() % 101) / 100.0f * REGEN_MAX; break;
            case 5: newGeneticCode[6] = (float)(std::rand() % 101) / 100.0f * MYOPIA_MAX; break;
            case 6: newGeneticCode[7] = (float)(std::rand() % 101) / 100.0f * AIM_MAX; break;
            case 7: newGeneticCode[8] = (float)(std::rand() % 101) / 100.0f * AGING_MAX_FLOAT; break;
            case 8: newGeneticCode[9] = (float)(std::rand() % (FERTILITY_MAX_INT + 1)); break;
                // Obèse (1), Hyperactif (9), etc. gérés par TraitManager via ID uniquement
        }

        entities.emplace_back(
                name, randomX, randomY, color,
                newGeneticCode, currentGeneration, "NONE", "NONE"
        );
    }
}

// --- LOGIQUE DE REPRODUCTION CORRIGÉE (65/30/5) + NETTOYAGE CLASSIQUE ---
void Simulation::triggerReproduction(const std::vector<Entity>& parents) {
    std::vector<Entity> newGeneration;
    int numParents = parents.size();
    if (parents.empty() || numParents < 2) { initialize(this->maxEntities); return; }
    int newGen = parents[0].getGeneration() + 1;
    this->currentGeneration = newGen;

    const std::vector<int> bioGeneIndices = {3, 4, 5, 6, 7, 8, 9};

    for (int i = 0; i < maxEntities; ++i) {
        const Entity& parent1 = parents[std::rand() % numParents];
        const Entity* parent2_ptr = &parents[std::rand() % numParents];
        if (numParents > 1) {
            while (parent1.getName() == parent2_ptr->getName()) {
                parent2_ptr = &parents[std::rand() % numParents];
            }
        }
        const Entity& parent2 = *parent2_ptr;
        float childGeneticCode[12];

        // 1. Structurels (0, 1, 2, 10)
        int structuralGenes[] = {0, 1, 2, 10};
        for (int idx : structuralGenes) {
            if (std::rand() % 2 == 0) childGeneticCode[idx] = parent1.getGeneticCode()[idx];
            else childGeneticCode[idx] = parent2.getGeneticCode()[idx];

            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                if (idx == 0) {
                    childGeneticCode[idx] += (float)((std::rand() % (2 * RAD_MUTATION_AMOUNT + 1)) - RAD_MUTATION_AMOUNT);
                    childGeneticCode[idx] = std::max(10.0f, std::min(40.0f, childGeneticCode[idx]));
                } else if (idx == 10) {
                    childGeneticCode[idx] = (childGeneticCode[idx] < 0.5f) ? 1.0f : 0.0f;
                } else {
                    childGeneticCode[idx] += (float)((std::rand() % (2 * GENE_MUTATION_AMOUNT + 1)) - GENE_MUTATION_AMOUNT) / 100.0f;
                }
            }
        }

        // 2. Trait Dominant & Bio Genes
        // Reset
        for (int idx : bioGeneIndices) childGeneticCode[idx] = 0.0f;

        int roll = std::rand() % 100;
        int chosenID = 0;

        if (roll < 65) { // Classique
            chosenID = 0;
        } else if (roll < 95) { // Héritage
            const Entity& pModel = (std::rand() % 2 == 0) ? parent1 : parent2;
            chosenID = pModel.getCurrentTraitID();
            if (chosenID != 0) {
                for (int idx : bioGeneIndices) childGeneticCode[idx] = pModel.getGeneticCode()[idx];
            }
        } else { // Mutation (Rare)
            chosenID = 1 + (std::rand() % (NUMBER_OF_TRAITS - 1));
            float randomStrength = (float)(20 + (std::rand() % 81)) / 100.0f;
            switch (chosenID) {
                case 2: childGeneticCode[3] = randomStrength * FRAGILITY_MAX; break;
                case 3: childGeneticCode[4] = randomStrength * EFFICIENCY_MAX; break;
                case 4: childGeneticCode[5] = randomStrength * REGEN_MAX; break;
                case 5: childGeneticCode[6] = randomStrength * MYOPIA_MAX; break;
                case 6: childGeneticCode[7] = randomStrength * AIM_MAX; break;
                case 7: childGeneticCode[9] = (float)(1 + std::rand() % 2); break;
                case 8: childGeneticCode[8] = randomStrength * AGING_MAX_FLOAT; break;
            }
        }
        childGeneticCode[11] = (float)chosenID;

        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomRad = (int)childGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        newGeneration.emplace_back(
                newName, randomX, randomY, parent1.getColor(),
                childGeneticCode, newGen,
                parent1.getName(), parent2.getName()
        );
    }
    entities = std::move(newGeneration);
}

void Simulation::triggerManualRestart() { triggerReproduction(lastSurvivors); }

void Simulation::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        SDL_Point mousePoint = {mouseX, mouseY};

        // GESTION UI PRIORITAIRE
        if (!inspectionStack.empty() && mouseX > panelCurrentX) {
            if (SDL_PointInRect(&mousePoint, &panelBack_rect) && inspectionStack.size() > 1) {
                inspectionStack.pop_back();
            }
            else if (SDL_PointInRect(&mousePoint, &panelParent1_rect)) {
                std::string p1_name = inspectionStack.back().getParent1Name();
                if (genealogyArchive.count(p1_name)) inspectionStack.push_back(genealogyArchive.at(p1_name));
            }
            else if (SDL_PointInRect(&mousePoint, &panelParent2_rect)) {
                std::string p2_name = inspectionStack.back().getParent2Name();
                if (genealogyArchive.count(p2_name)) inspectionStack.push_back(genealogyArchive.at(p2_name));
            }
            return; // STOP : On a cliqué sur le panel, on ne clique pas dans le monde
        }

        // GESTION MONDE
        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;
            int dx = mouseX - entity.getX();
            int dy = mouseY - entity.getY();
            if (std::sqrt((float)dx*dx + dy*dy) < entity.getRad()) {
                selectedLivingEntity = &entity;
                inspectionStack.clear();
                inspectionStack.push_back(entity);
                entityClicked = true;
                break;
            }
        }
        if (!entityClicked) {
            selectedLivingEntity = nullptr;
            inspectionStack.clear();
        }
    }
}

// --- UPDATE & RENDER (Standards) ---
Simulation::SimUpdateStatus Simulation::update(int speedMultiplier, bool autoRestart) {
    updateLogic(speedMultiplier);
    updatePhysics(speedMultiplier);
    updateProjectiles();
    cleanupDead();

    if (entities.size() <= SURVIVOR_COUNT && !entities.empty()) {
        lastSurvivors = entities;
        for (const auto& winner : lastSurvivors) genealogyArchive.insert({winner.getName(), winner});

        if (autoRestart) {
            triggerReproduction(lastSurvivors);
            return SimUpdateStatus::RUNNING;
        } else {
            return SimUpdateStatus::FINISHED;
        }
    }

    // Animation Panel
    if (!inspectionStack.empty()) panelTargetX = (float)(WINDOW_WIDTH - PANEL_WIDTH);
    else panelTargetX = (float)WINDOW_WIDTH;

    if(selectedLivingEntity != nullptr && !selectedLivingEntity->getIsAlive()) selectedLivingEntity = nullptr;

    float distance = panelTargetX - panelCurrentX;
    if (std::abs(distance) < 1.0f) panelCurrentX = panelTargetX;
    else panelCurrentX += distance * 0.1f;

    return SimUpdateStatus::RUNNING;
}
void Simulation::updateLogic(int speedMultiplier) {
    for (auto &entity : entities) {
        if (!entity.getIsAlive()) continue;

        Entity* closestEnemy = nullptr;
        auto closestDistance = (float)(WINDOW_WIDTH + WINDOW_HEIGHT);

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
            // --- RÉCUPÉRATION DES STATS (Nécessaires pour la suite) ---
            bool isRanged = entity.getIsRanged();
            int attackRange = entity.getAttackRange();
            Uint32 cooldown = entity.getAttackCooldown();
            int damage = entity.getDamage();
            int target[2] = {closestEnemy->getX(), closestEnemy->getY()};

            // --- IA DE MOUVEMENT ---
            if (closestDistance < entity.getSightRadius()){

                if (isRanged) {
                    // --- COMPORTEMENT TIREUR (RANGED) ---
                    entity.setIsCharging(false); // Un ranged ne charge jamais

                    // Calcul de la distance idéale (Kite)
                    float kiteRatio = entity.getKiteRatio();
                    int idealRange = (int)(attackRange * kiteRatio);

                    if (closestDistance < idealRange) {
                        // TROP PRÈS -> FUITE (Kiting)
                        if (entity.getStamina() > 0) {
                            int fleeTarget[2] = {
                                    entity.getX() + (entity.getX() - closestEnemy->getX()) * 2,
                                    entity.getY() + (entity.getY() - closestEnemy->getY()) * 2
                            };
                            entity.chooseDirection(fleeTarget);
                            entity.setIsFleeing(true);
                        } else {
                            // Trop fatigué pour fuir
                            entity.chooseDirection(nullptr);
                            entity.setIsFleeing(false);
                        }
                    }
                    else if (closestDistance < attackRange) {
                        // BONNE DISTANCE -> ON S'ARRÊTE POUR VISER
                        entity.chooseDirection(nullptr);
                        entity.setIsFleeing(false);
                    }
                    else {
                        // TROP LOIN -> ON APPROCHE
                        entity.chooseDirection(target);
                        entity.setIsFleeing(false);
                    }
                }
                else {
                    // --- COMPORTEMENT MÊLÉE ---
                    entity.setIsFleeing(false);

                    // Si on voit l'ennemi mais qu'on est trop loin : CHARGE !
                    if (closestDistance > attackRange) {
                        entity.setIsCharging(true);
                        entity.chooseDirection(target);
                    }
                    else {
                        // À PORTÉE -> ON S'ARRÊTE ET ON TAPPE
                        entity.setIsCharging(false);
                        int stopTarget[2] = {entity.getX(), entity.getY()};
                        entity.chooseDirection(stopTarget);
                    }
                }
            }
            else {
                // Ennemi trop loin (hors vue) -> Repos
                entity.chooseDirection(nullptr);
                entity.setIsFleeing(false);
                entity.setIsCharging(false);
            }

            // --- LOGIQUE D'ATTAQUE ---
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
                                    entity.getProjectileSpeed(), damage, attackRange,
                                    entity.getColor(), entity.getProjectileRadius(), entity.getName()
                            );
                            projectiles.push_back(newP);
                        } else {
                            // --- ATTAQUE MELEE ---
                            closestEnemy->takeDamage(damage);

                            // KNOCKBACK (Recul)
                            closestEnemy->knockBackFrom(entity.getX(), entity.getY(), 40);
                        }
                        // Mise à jour du temps de tir
                        lastShotTime[entity.getName()] = currentTime;
                    }
                }
            }
        } else {
            // Aucun ennemi vivant sur la carte
            entity.chooseDirection(nullptr);
            entity.setIsFleeing(false);
            entity.setIsCharging(false);
        }
    }
}


void Simulation::updatePhysics(int speedMultiplier) {
    for (auto &entity : entities) entity.update(speedMultiplier);
    for (auto &entity : entities) {
        for (auto &other : entities) {
            if (&entity != &other && other.getIsAlive()) {
                int dx = entity.getX() - other.getX(); int dy = entity.getY() - other.getY();
                float distance = std::sqrt((float)dx * dx + (float)dy * dy);
                int minDistance = entity.getRad() + other.getRad();
                if (distance < minDistance) {
                    float overlap = minDistance - distance; float separationFactor = 0.5f; float separationDistance = overlap * separationFactor;
                    float normX = (distance > 0) ? dx / distance : 1.0f; float normY = (distance > 0) ? dy / distance : 0.0f;
                    entity.setX(entity.getX() + static_cast<int>(normX * separationDistance));
                    entity.setY(entity.getY() + static_cast<int>(normY * separationDistance));
                }
            }
        }
    }
}


void Simulation::updateProjectiles() {
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [&](Projectile& proj) {
        proj.update();
        if (!proj.isAlive()) return true;
        for (auto &entity : entities) {
            if (entity.getIsAlive()) {
                if (entity.getName() == proj.getShooterName()) continue;
                int dx = proj.getX() - entity.getX(); int dy = proj.getY() - entity.getY();
                float distance = std::sqrt((float)dx * dx + (float)dy * dy);
                if (distance < proj.getRadius() + entity.getRad()) { entity.takeDamage(proj.getDamage()); return true; }
            }
        }
        return false;
    }), projectiles.end());
}

void Simulation::cleanupDead() {
    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) { return !entity.getIsAlive(); }), entities.end());
}

void Simulation::render(SDL_Renderer *renderer, bool showDebug) {
    for (auto &entity : entities) entity.draw(renderer, showDebug);
    for (auto &proj : projectiles) proj.draw(renderer);
    if (selectedLivingEntity != nullptr) circleRGBA(renderer, selectedLivingEntity->getX(), selectedLivingEntity->getY(), selectedLivingEntity->getRad() + 4, 255, 255, 0, 200);
    if (panelCurrentX < WINDOW_WIDTH) drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
}

void Simulation::drawStatsPanel(SDL_Renderer* renderer, int panelX) {
    auto float_to_string = [](float val, int precision = 2) { std::stringstream ss; ss << std::fixed << std::setprecision(precision) << val; return ss.str(); };
    if (inspectionStack.empty()) return;
    const Entity* entityToDisplay = (inspectionStack.size() == 1 && selectedLivingEntity) ? selectedLivingEntity : &inspectionStack.back();
    if (!entityToDisplay) return;
    const Entity& entity = *entityToDisplay;

    // 1. Fond
    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 240);
    SDL_RenderFillRect(renderer, &panelRect);

    int x = panelX + 15, y = 20, lineHeight = 18;
    SDL_Color titleColor = {255, 255, 0, 255}, statColor = {255, 255, 255, 255};
    SDL_Color geneColor = {150, 200, 255, 255}, linkColor = {100, 100, 255, 255}, impactColor = {255, 100, 100, 255}, bonusColor = {100, 255, 100, 255};

    // 2. Bouton Back
    if (inspectionStack.size() > 1) {
        stringRGBA(renderer, x + 10, y + 8, "< Back", statColor.r, statColor.g, statColor.b, 255);
        panelBack_rect = {x, y, 80, 25};
        rectangleRGBA(renderer, panelBack_rect.x, panelBack_rect.y, panelBack_rect.x+panelBack_rect.w, panelBack_rect.y+panelBack_rect.h, 255,255,255,100);
        y += 40;
    } else panelBack_rect = {0,0,0,0};

    // 3. Info
    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight*1.5;
    stringRGBA(renderer, x, y, (entityToDisplay == selectedLivingEntity ? "Health: " + std::to_string(entity.getHealth()) : "Health: (Decede)").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;

    // 4. Généalogie
    stringRGBA(renderer, x, y, "--- Genealogie ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Gen: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;

    stringRGBA(renderer, x, y, ("P1: " + entity.getParent1Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent1_rect = {x, y - 2, 250, lineHeight}; y += lineHeight;

    stringRGBA(renderer, x, y, ("P2: " + entity.getParent2Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent2_rect = {x, y - 2, 250, lineHeight}; y += lineHeight*1.5;

    // 5. Body & JSON Traits
    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;

    int traitID = entity.getCurrentTraitID();


    stringRGBA(renderer, x, y, ("Rad: " + std::to_string(entity.getRad())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor()*100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;

    // 6. Bio Traits
    stringRGBA(renderer, x, y, "--- Bio-Traits ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    const TraitStats& stats = TraitManager::get(traitID);
    stringRGBA(renderer, x, y, ("Trait: " + stats.name).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight*1.5;
    if (traitID != 0 && !stats.description.empty()) {
        stringRGBA(renderer, x, y, stats.description.c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight;
    } else if (traitID == 0) {
        stringRGBA(renderer, x, y, "Classic : Neutral", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    }

    // Bio Float genes
    float frag = entity.getDamageFragility()*100;
    if(frag > 0.1) { stringRGBA(renderer, x, y, ("Weak: +" + float_to_string(frag,1) + "% Dmg").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }

    float myop = entity.getMyopiaFactor()*100;
    if(myop > 0.1) { stringRGBA(renderer, x, y, ("Myopic: -" + float_to_string(myop,1) + "% Vision").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }

    y += 10;
    stringRGBA(renderer, x, y, "--- Weapon Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Damage: " + std::to_string(entity.getDamage())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Attack Range: " + std::to_string(entity.getAttackRange())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Cooldown: " + std::to_string(entity.getAttackCooldown()) + " ms").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina Cost: " + std::to_string(entity.getStaminaAttackCost())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    if (entity.getIsRanged()) {
        stringRGBA(renderer, x, y, ("Proj Speed: " + std::to_string(entity.getProjectileSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
        stringRGBA(renderer, x, y, ("Proj Radius: " + std::to_string(entity.getProjectileRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    }
}
