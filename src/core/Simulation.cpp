#include "Simulation.h"
#include "../constants.h"
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
    const int NEUTRAL_INT = 0;
    const float FRAGILITY_MAX = 0.3f;
    const float EFFICIENCY_MAX = 0.5f;
    const float REGEN_MAX = 0.2f;
    const float MYOPIA_MAX = 0.5f;
    const float AIM_MAX = 15.0f;
    const int FERTILITY_MAX_INT = 2;
    const float AGING_MAX_FLOAT = 0.01f;

    const std::string TRAIT_NAMES[NUMBER_OF_TRAITS] = {
            "Classique", "Obèse", "Fragile", "Efficace",
            "Régénérateur", "Myope", "Maladroit", "Fertile",
            "Vieillissant", "Hyperactif", "Sédentaire", "Robuste"
    };
}

Simulation::Simulation(int maxEntities) :
        maxEntities(maxEntities),
        selectedLivingEntity(nullptr)
{
    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;
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
        newGeneticCode[1] = (float)(std::rand() % 101);
        newGeneticCode[2] = (10 + (std::rand() % 91)) / 100.0f;
        newGeneticCode[10] = (std::rand() % 2 == 0) ? 1.0f : 0.0f;
        for(int j = 3; j <= 9; ++j) { newGeneticCode[j] = NEUTRAL_FLOAT; }
        int dominantTraitID = 0;
        int roll = std::rand() % 100;
        if (roll >= 80) dominantTraitID = 1 + (std::rand() % (NUMBER_OF_TRAITS - 1));
        newGeneticCode[11] = (float)dominantTraitID;
        switch (dominantTraitID) {
            case 1: newGeneticCode[1] = (float)(std::rand() % 99) / 100.0f * EFFICIENCY_MAX; break;
            case 2: newGeneticCode[3] = (float)(std::rand() % 101) / 100.0f * FRAGILITY_MAX; break;
            case 3: newGeneticCode[4] = (float)(std::rand() % 101) / 100.0f * EFFICIENCY_MAX; break;
            case 4: newGeneticCode[5] = (float)(std::rand() % 101) / 100.0f * REGEN_MAX; break;
            case 5: newGeneticCode[6] = (float)(std::rand() % 101) / 100.0f * MYOPIA_MAX; break;
            case 6: newGeneticCode[7] = (float)(std::rand() % 101) / 100.0f * AIM_MAX; break;
            case 7: newGeneticCode[8] = (float)(std::rand() % 101) / 100.0f * AGING_MAX_FLOAT; break;
            case 8: newGeneticCode[9] = (float)(std::rand() % (FERTILITY_MAX_INT + 1)); break;
        }
        entities.emplace_back(name, randomX, randomY, color, newGeneticCode, currentGeneration, "NONE", "NONE");
    }
}

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
        if (numParents > 1) { while (parent1.getName() == parent2_ptr->getName()) { parent2_ptr = &parents[std::rand() % numParents]; } }
        const Entity& parent2 = *parent2_ptr;
        float childGeneticCode[12];
        int structuralGenes[] = {0, 1, 2, 10};
        for (int idx : structuralGenes) {
            if (std::rand() % 2 == 0) childGeneticCode[idx] = parent1.getGeneticCode()[idx];
            else childGeneticCode[idx] = parent2.getGeneticCode()[idx];
            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                if (idx == 0) {
                    childGeneticCode[idx] += (float)((std::rand() % (2 * RAD_MUTATION_AMOUNT + 1)) - RAD_MUTATION_AMOUNT);
                    childGeneticCode[idx] = std::max(10.0f, std::min(40.0f, childGeneticCode[idx]));
                } else if (idx == 10) { childGeneticCode[idx] = (childGeneticCode[idx] < 0.5f) ? 1.0f : 0.0f;
                } else {
                    float mutation = (float)((std::rand() % (2 * GENE_MUTATION_AMOUNT + 1)) - GENE_MUTATION_AMOUNT) / 100.0f;
                    childGeneticCode[idx] += mutation;
                }
            }
        }
        for (int idx : bioGeneIndices) childGeneticCode[idx] = 0.0f;
        int roll = std::rand() % 100;
        int chosenID = 0;
        if (roll < 65) { chosenID = 0; }
        else if (roll < 95) {
            const Entity& pModel = (std::rand() % 2 == 0) ? parent1 : parent2;
            chosenID = pModel.getCurrentTraitID();
            if (chosenID != 0) { for (int idx : bioGeneIndices) childGeneticCode[idx] = pModel.getGeneticCode()[idx]; }
        } else {
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
        newGeneration.emplace_back(newName, randomX, randomY, parent1.getColor(), childGeneticCode, newGen, parent1.getName(), parent2.getName());
    }
    entities = std::move(newGeneration);
}

void Simulation::triggerManualRestart() { triggerReproduction(lastSurvivors); }

void Simulation::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x; int mouseY = event.button.y; SDL_Point mousePoint = {mouseX, mouseY};
        if (!inspectionStack.empty() && mouseX > panelCurrentX) {
            if (SDL_PointInRect(&mousePoint, &panelBack_rect) && inspectionStack.size() > 1) inspectionStack.pop_back();
            else if (SDL_PointInRect(&mousePoint, &panelParent1_rect)) {
                std::string p1_name = inspectionStack.back().getParent1Name();
                if (genealogyArchive.count(p1_name)) inspectionStack.push_back(genealogyArchive.at(p1_name));
            } else if (SDL_PointInRect(&mousePoint, &panelParent2_rect)) {
                std::string p2_name = inspectionStack.back().getParent2Name();
                if (genealogyArchive.count(p2_name)) inspectionStack.push_back(genealogyArchive.at(p2_name));
            }
            return;
        }
        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;
            int dx = mouseX - entity.getX(); int dy = mouseY - entity.getY();
            float distance = std::sqrt((float)dx * dx + (float)dy * dy);
            if (distance < entity.getRad()) {
                selectedLivingEntity = &entity; inspectionStack.clear(); inspectionStack.push_back(entity); entityClicked = true; break;
            }
        }
        if (!entityClicked) { selectedLivingEntity = nullptr; inspectionStack.clear(); }
    }
}

Simulation::SimUpdateStatus Simulation::update(int speedMultiplier, bool autoRestart) {
    updateLogic(speedMultiplier);
    updatePhysics(speedMultiplier);
    updateProjectiles();
    cleanupDead();
    if (entities.size() <= SURVIVOR_COUNT && !entities.empty()) {
        lastSurvivors = entities;
        for (const auto& winner : lastSurvivors) genealogyArchive.insert({winner.getName(), winner});
        if (autoRestart) { triggerReproduction(lastSurvivors); return SimUpdateStatus::RUNNING; }
        else return SimUpdateStatus::FINISHED;
    }
    if (!inspectionStack.empty()) panelTargetX = (float)(WINDOW_WIDTH - PANEL_WIDTH); else panelTargetX = (float)WINDOW_WIDTH;
    if(selectedLivingEntity != nullptr && !selectedLivingEntity->getIsAlive()) selectedLivingEntity = nullptr;
    float distance = panelTargetX - panelCurrentX;
    if (std::abs(distance) < 1.0f) panelCurrentX = panelTargetX; else panelCurrentX += distance * 0.1f;
    return SimUpdateStatus::RUNNING;
}

void Simulation::updateLogic(int speedMultiplier) {
    for (auto &entity : entities) {
        if (!entity.getIsAlive()) continue;
        Entity* closestEnemy = nullptr;
        auto closestDistance = (float)(WINDOW_WIDTH + WINDOW_HEIGHT);
        for (auto &other : entities) {
            if (&entity != &other && other.getIsAlive()) {
                int dx = entity.getX() - other.getX(); int dy = entity.getY() - other.getY();
                float distance = std::sqrt((float)dx * dx + (float)dy * dy);
                if (distance < closestDistance) { closestDistance = distance; closestEnemy = &other; }
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
                    entity.setIsCharging(false);
                    const int idealRange = attackRange * 2 / 3;
                    if (closestDistance < idealRange) {
                        if (entity.getStamina() > 0) {
                            int fleeTarget[2] = { entity.getX() + (entity.getX() - closestEnemy->getX()) * 2, entity.getY() + (entity.getY() - closestEnemy->getY()) * 2 };
                            entity.chooseDirection(fleeTarget); entity.setIsFleeing(true);
                        } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); }
                    } else if (closestDistance < attackRange) { entity.chooseDirection(nullptr); entity.setIsFleeing(false); }
                    else { entity.chooseDirection(target); entity.setIsFleeing(false); }
                } else {
                    entity.setIsFleeing(false);
                    if (closestDistance > attackRange) entity.setIsCharging(true);
                    else entity.setIsCharging(false);

                    if (closestDistance < attackRange) {
                        int stopTarget[2] = {entity.getX(), entity.getY()}; entity.chooseDirection(stopTarget);
                    } else entity.chooseDirection(target);
                }
            } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); entity.setIsCharging(false); }

            if (closestDistance < attackRange) {
                Uint32 currentTime = SDL_GetTicks();
                Uint32 effectiveCooldown = (speedMultiplier > 0) ? (cooldown / speedMultiplier) : cooldown;
                if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + effectiveCooldown) {
                    if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                        if (isRanged) {
                            Projectile newP(entity.getX(), entity.getY(), closestEnemy->getX(), closestEnemy->getY(), entity.getProjectileSpeed(), damage, attackRange, entity.getColor(), entity.getProjectileRadius(), entity.getName());
                            projectiles.push_back(newP);
                        } else {
                            // --- ATTAQUE MELEE ---
                            closestEnemy->takeDamage(damage);

                            // [NOUVEAU] KNOCKBACK !
                            // Repousse l'ennemi de 40 pixels pour briser le corps-à-corps statique
                            closestEnemy->knockBackFrom(entity.getX(), entity.getY(), 40);
                        }
                        lastShotTime[entity.getName()] = currentTime;
                    }
                }
            }
        } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); entity.setIsCharging(false); }
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
    const Entity* entityToDisplay = nullptr;
    if (inspectionStack.size() == 1 && selectedLivingEntity != nullptr) entityToDisplay = selectedLivingEntity;
    else entityToDisplay = &inspectionStack.back();
    if (entityToDisplay == nullptr) return;
    const Entity& entity = *entityToDisplay;

    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 240); SDL_RenderFillRect(renderer, &panelRect);
    int x = panelX + 15; int y = 20; const int lineHeight = 18;
    const SDL_Color titleColor = {255, 255, 0, 255}; const SDL_Color statColor = {255, 255, 255, 255};
    const SDL_Color geneColor = {150, 200, 255, 255}; const SDL_Color linkColor = {100, 100, 255, 255};
    const SDL_Color impactColor = {255, 100, 100, 255}; const SDL_Color bonusColor = {100, 255, 100, 255};

    if (inspectionStack.size() > 1) {
        std::string backText = "< Back"; stringRGBA(renderer, x + 10, y + 8, backText.c_str(), statColor.r, statColor.g, statColor.b, 255);
        panelBack_rect = {x, y, 80, 25}; rectangleRGBA(renderer, panelBack_rect.x, panelBack_rect.y, panelBack_rect.x + panelBack_rect.w, panelBack_rect.y + panelBack_rect.h, 255, 255, 255, 100);
        y += 40;
    } else panelBack_rect = {0, 0, 0, 0};

    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight * 1.5;
    if (entityToDisplay == selectedLivingEntity) stringRGBA(renderer, x, y, ("Health: " + std::to_string(entity.getHealth()) + "/" + std::to_string(entity.getMaxHealth())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    else stringRGBA(renderer, x, y, "Health: (Decede)", 150, 150, 150, 255);
    y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina()) + "/" + std::to_string(entity.getMaxStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Genealogie ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Generation: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Parent 1: " + entity.getParent1Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255); panelParent1_rect = {x, y - 2, 250, lineHeight}; y += lineHeight;
    stringRGBA(renderer, x, y, ("Parent 2: " + entity.getParent2Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255); panelParent2_rect = {x, y - 2, 250, lineHeight}; y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    std::string dominantTrait = "Inconnu"; int traitID = entity.getCurrentTraitID();
    if (traitID >= 0 && traitID < 12) dominantTrait = TRAIT_NAMES[traitID];
    stringRGBA(renderer, x, y, ("Dominant Trait: " + dominantTrait).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight * 1.5;
    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor() * 100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Sight Radius: " + std::to_string(entity.getSightRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Genetic Traits ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, (std::string("Type: ") + (entity.getIsRanged() ? "Ranged" : "Melee")).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Weapon Gene: " + float_to_string(entity.getWeaponGene())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Kite Ratio: " + float_to_string(entity.getKiteRatio(), 2)).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Bio-Traits ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    if (traitID == 1) { stringRGBA(renderer, x, y, "Obese: -20.0% Stamina Max", impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
    else if (traitID == 9) { stringRGBA(renderer, x, y, "Hyperactif: +20.0% Vitesse", bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; stringRGBA(renderer, x, y, "            x2.0 Cout Stamina Mouvement", impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
    else if (traitID == 10) { stringRGBA(renderer, x, y, "Sedentaire: -30.0% Vitesse", impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; stringRGBA(renderer, x, y, "            +15.0% Stamina Max", bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; }
    else if (traitID == 11) { stringRGBA(renderer, x, y, "Robuste: Defense Base augmentee", bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; }
    else if (traitID == 0) { stringRGBA(renderer, x, y, "Classique: Neutre", statColor.r, statColor.g, statColor.b, 255); y += lineHeight; }
    float fragPercent = entity.getDamageFragility() * 100.0f;
    if (fragPercent > 0.001f) { stringRGBA(renderer, x, y, ("Fragilite: +" + float_to_string(fragPercent, 1) + "% Degats Subis").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
    float effPercent = entity.getStaminaEfficiency() * 100.0f;
    if (effPercent > 0.001f) { stringRGBA(renderer, x, y, ("Stamina Eff: -" + float_to_string(effPercent, 1) + "% Cout").c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; }
    float regenRate = entity.getBaseHealthRegen();
    if (regenRate > 0.0001f) { stringRGBA(renderer, x, y, ("Regeneration: +" + float_to_string(regenRate, 3) + " HP/cycle").c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; }
    float myopiaPercent = entity.getMyopiaFactor() * 100.0f;
    if (myopiaPercent > 0.001f) { stringRGBA(renderer, x, y, ("Myopie: -" + float_to_string(myopiaPercent, 1) + "% Vision").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
    float aimPenalty = entity.getAimingPenalty();
    if (entity.getIsRanged() && aimPenalty > 0.001f) { stringRGBA(renderer, x, y, ("Maladresse: +" + float_to_string(aimPenalty, 1) + " deg Erreur").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
    int fertility = entity.getFertilityFactor();
    if (fertility > 0) { stringRGBA(renderer, x, y, ("Fecondite: +" + std::to_string(fertility) + " Enfants (Cout -" + float_to_string(fertility * 5.0f, 1) + "% PV)").c_str(), bonusColor.r, bonusColor.g, bonusColor.b, 255); y += lineHeight; }
    float agingRate = entity.getAgingRate();
    if (agingRate > 0.00001f) { stringRGBA(renderer, x, y, ("Vieillissement: -" + float_to_string(agingRate * 1000.0f, 3) + "%o Max PV/cycle").c_str(), impactColor.r, impactColor.g, impactColor.b, 255); y += lineHeight; }
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
