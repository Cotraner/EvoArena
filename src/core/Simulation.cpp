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

        int roleRoll = std::rand() % 100;
        if (roleRoll < 33) {
            newGeneticCode[10] = 0.15f; // Melee (0.0 - 0.33)
        } else if (roleRoll < 66) {
            newGeneticCode[10] = 0.50f; // Ranged (0.33 - 0.66)
        } else {
            newGeneticCode[10] = 0.85f; // Healer (0.66 - 1.0)
        }
        // Petit bruit aléatoire pour éviter des clones parfaits
        newGeneticCode[10] += ((float)(std::rand() % 11) - 5.0f) / 100.0f;


        // Genes BIO
        newGeneticCode[3] = (std::rand() % 10 < 2) ? ((float)(std::rand() % 101) / 100.0f * FRAGILITY_MAX) : 0.0f;
        newGeneticCode[4] = (std::rand() % 10 < 2) ? ((float)(std::rand() % 101) / 100.0f * EFFICIENCY_MAX) : 0.0f;
        newGeneticCode[5] = (std::rand() % 10 < 2) ? ((float)(std::rand() % 101) / 100.0f * REGEN_MAX) : 0.0f;
        newGeneticCode[6] = (std::rand() % 10 < 2) ? ((float)(std::rand() % 101) / 100.0f * MYOPIA_MAX) : 0.0f;
        newGeneticCode[7] = (std::rand() % 10 < 2) ? ((float)(std::rand() % 101) / 100.0f * AIM_MAX) : 0.0f;
        newGeneticCode[8] = (std::rand() % 20 < 1) ? ((float)(std::rand() % 101) / 100.0f * AGING_MAX_FLOAT) : 0.0f;
        newGeneticCode[9] = (std::rand() % 20 < 1) ? (float)(1 + std::rand() % FERTILITY_MAX_INT) : 0.0f;

        int maxTraits = TraitManager::getCount();
        int dominantTraitID = 0;
        if (maxTraits > 1 && (std::rand() % 100) >= 80) {
            dominantTraitID = 1 + (std::rand() % (maxTraits - 1));
        }
        newGeneticCode[11] = (float)dominantTraitID;

        entities.emplace_back(name, randomX, randomY, color, newGeneticCode, currentGeneration, "NONE", "NONE");
    }
}

void Simulation::triggerReproduction(const std::vector<Entity>& parents) {
    std::vector<Entity> newGeneration;
    int numParents = parents.size();

    // Sécurité : S'il n'y a pas assez de survivants, on redémarre tout
    if (parents.empty() || numParents < 2) {
        initialize(this->maxEntities);
        return;
    }

    int newGen = parents[0].getGeneration() + 1;
    this->currentGeneration = newGen;
    const std::vector<int> bioGeneIndices = {3, 4, 5, 6, 7, 8, 9};

    // --- CRÉATION DE LA LOTERIE (La partie clé pour l'équilibrage) ---
    std::vector<int> parentLottery;
    parentLottery.reserve(numParents * 10); // Pré-allocation pour la perf

    for (int i = 0; i < numParents; ++i) {
        // Ticket de base pour avoir survécu
        int tickets = 1;

        // Bonus via le Gène de Fertilité (valeur 0, 1 ou 2)
        // Un gène fertilité de 2 donne +6 tickets !
        tickets += parents[i].getFertilityFactor() * 3;

        // Bonus via le Trait "Fertile" (ID 7 dans ton JSON)
        // Cela rend le trait Fertile extrêmement puissant pour la survie de l'espèce
        if (parents[i].getCurrentTraitID() == 7) {
            tickets += 15;
        }

        // On ajoute les tickets dans l'urne
        for (int t = 0; t < tickets; ++t) {
            parentLottery.push_back(i);
        }
    }

    // --- GÉNÉRATION DES ENFANTS ---
    for (int i = 0; i < maxEntities; ++i) {
        // Sélection des parents via la loterie
        int p1_index = parentLottery[std::rand() % parentLottery.size()];
        int p2_index = parentLottery[std::rand() % parentLottery.size()];

        const Entity& parent1 = parents[p1_index];
        const Entity* parent2_ptr = &parents[p2_index];

        // On essaie d'éviter l'auto-fécondation si possible (sauf s'il ne reste qu'un survivant dominant)
        int attempts = 0;
        while (parent1.getName() == parent2_ptr->getName() && attempts < 10) {
            p2_index = parentLottery[std::rand() % parentLottery.size()];
            parent2_ptr = &parents[p2_index];
            attempts++;
        }
        const Entity& parent2 = *parent2_ptr;

        // --- Crossover & Mutation (Code existant conservé et optimisé) ---
        float childGeneticCode[12];
        int structuralGenes[] = {0, 1, 2, 10};

        // 1. Gènes Structurels
        for (int idx : structuralGenes) {
            childGeneticCode[idx] = (std::rand() % 2 == 0) ? parent1.getGeneticCode()[idx] : parent2.getGeneticCode()[idx];

            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                if (idx == 0) { // Rayon (Taille)
                    childGeneticCode[idx] += (float)((std::rand() % (2 * RAD_MUTATION_AMOUNT + 1)) - RAD_MUTATION_AMOUNT);
                    childGeneticCode[idx] = std::max(10.0f, std::min(40.0f, childGeneticCode[idx]));
                } else if (idx == 10) { // Rôle (Melee/Ranged/Healer)
                    float change = (float)((std::rand() % 21) - 10) / 100.0f;
                    childGeneticCode[idx] = std::clamp(childGeneticCode[idx] + change, 0.0f, 1.0f);
                } else {
                    childGeneticCode[idx] += (float)((std::rand() % (2 * GENE_MUTATION_AMOUNT + 1)) - GENE_MUTATION_AMOUNT) / 100.0f;
                }
            }
        }

        // 2. Gènes Biologiques
        for (int idx : bioGeneIndices) {
            childGeneticCode[idx] = (std::rand() % 2 == 0) ? parent1.getGeneticCode()[idx] : parent2.getGeneticCode()[idx];
            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                float mutation = (float)((std::rand() % (2 * GENE_MUTATION_AMOUNT + 1)) - GENE_MUTATION_AMOUNT) / 100.0f;
                childGeneticCode[idx] += mutation;
                if(childGeneticCode[idx] < 0) childGeneticCode[idx] = 0;
            }
        }

        // 3. Héritage du Trait (ID 11)
        int chosenID = 0;
        int roll = std::rand() % 100;
        if (roll < 45) chosenID = parent1.getCurrentTraitID();
        else if (roll < 90) chosenID = parent2.getCurrentTraitID();
        else {
            int maxTraits = TraitManager::getCount();
            if (maxTraits > 1) chosenID = 1 + (std::rand() % (maxTraits - 1)); // Mutation de trait aléatoire
        }
        childGeneticCode[11] = (float)chosenID;

        // Création de l'entité
        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomRad = (int)childGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        newGeneration.emplace_back(newName, randomX, randomY, parent1.getColor(), childGeneticCode, newGen, parent1.getName(), parent2.getName());
    }

    entities = std::move(newGeneration);
    selectedLivingEntity = nullptr;
    inspectionStack.clear();
}

void Simulation::triggerManualRestart() { triggerReproduction(lastSurvivors); }

void Simulation::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        SDL_Point mousePoint = {mouseX, mouseY};
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
            if (std::sqrt((float)dx*dx + dy*dy) < entity.getRad()) {
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

        int myType = entity.getEntityType();

        // --- HEALER (2) ---
        if (myType == 2) {
            Entity* targetAlly = nullptr;
            Entity* closestThreat = nullptr;
            float closestAllyDist = (float)(WINDOW_WIDTH + WINDOW_HEIGHT);
            float closestThreatDist = (float)(WINDOW_WIDTH + WINDOW_HEIGHT);

            for (auto &other : entities) {
                if (&entity != &other && other.getIsAlive()) {
                    float dist = std::hypot(entity.getX() - other.getX(), entity.getY() - other.getY());
                    if (entity.isAlliedWith(other)) {
                        if (other.getHealth() < other.getMaxHealth() && dist < closestAllyDist) {
                            closestAllyDist = dist; targetAlly = &other;
                        }
                    } else {
                        if (dist < closestThreatDist) { closestThreatDist = dist; closestThreat = &other; }
                    }
                }
            }

            bool actionTaken = false;
            // 1. DANGER IMMEDIAT (Auto-Défense)
            if (closestThreat && closestThreatDist < 150) {
                int fleeTarget[2] = { entity.getX() + (entity.getX() - closestThreat->getX()), entity.getY() + (entity.getY() - closestThreat->getY()) };
                entity.chooseDirection(fleeTarget); entity.setIsFleeing(true);
                Uint32 currentTime = SDL_GetTicks();
                if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + entity.getAttackCooldown()) {
                    if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                        Projectile newP(entity.getX(), entity.getY(), closestThreat->getX(), closestThreat->getY(), entity.getProjectileSpeed(), entity.getDamage(), entity.getAttackRange(), entity.getColor(), entity.getProjectileRadius(), entity.getName());
                        projectiles.push_back(newP); lastShotTime[entity.getName()] = currentTime;
                    }
                }
                actionTaken = true;
            }
                // 2. SOIN
            else if (targetAlly && closestAllyDist < entity.getSightRadius()) {
                entity.setIsFleeing(false);
                int targetPos[2] = {targetAlly->getX(), targetAlly->getY()};
                if (closestAllyDist < entity.getRad() + targetAlly->getRad() + 40) {
                    entity.chooseDirection(nullptr);
                    Uint32 currentTime = SDL_GetTicks();
                    Uint32 healCooldown = entity.getAttackCooldown() / 2;
                    if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + healCooldown) {
                        if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                            int healAmount = entity.getDamage();
                            targetAlly->receiveHealing(healAmount);
                            entity.receiveHealing(healAmount / 2); // Symbiose
                            lastShotTime[entity.getName()] = currentTime;
                        }
                    }
                } else entity.chooseDirection(targetPos);
                actionTaken = true;
            }
            if (!actionTaken) { entity.chooseDirection(nullptr); entity.setIsFleeing(false); }
            continue;
        }

        // --- MELEE / RANGED ---
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
            bool isRanged = (myType == 1);
            int attackRange = entity.getAttackRange();
            int target[2] = {closestEnemy->getX(), closestEnemy->getY()};

            if (closestDistance < entity.getSightRadius()){
                if (isRanged) {
                    entity.setIsCharging(false);
                    float kiteRatio = entity.getKiteRatio();
                    int idealRange = (int)(attackRange * kiteRatio);

                    if (closestDistance < idealRange) {
                        if (entity.getStamina() > 0) {
                            int fleeTarget[2] = { entity.getX() + (entity.getX() - closestEnemy->getX()) * 2, entity.getY() + (entity.getY() - closestEnemy->getY()) * 2 };
                            entity.chooseDirection(fleeTarget); entity.setIsFleeing(true);
                        } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); }
                    } else if (closestDistance < attackRange) { entity.chooseDirection(nullptr); entity.setIsFleeing(false); }
                    else { entity.chooseDirection(target); entity.setIsFleeing(false); }
                } else {
                    entity.setIsFleeing(false);
                    if (closestDistance > attackRange) entity.setIsCharging(true); else entity.setIsCharging(false);
                    if (closestDistance < attackRange) {
                        int stopTarget[2] = {entity.getX(), entity.getY()}; entity.chooseDirection(stopTarget);
                    } else entity.chooseDirection(target);
                }
            } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); entity.setIsCharging(false); }

            if (closestDistance < attackRange) {
                Uint32 currentTime = SDL_GetTicks();
                Uint32 cooldown = entity.getAttackCooldown();
                int damage = entity.getDamage();
                Uint32 effectiveCooldown = (speedMultiplier > 0) ? (cooldown / speedMultiplier) : cooldown;
                if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + effectiveCooldown) {
                    if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                        if (isRanged) {
                            Projectile newP(entity.getX(), entity.getY(), closestEnemy->getX(), closestEnemy->getY(), entity.getProjectileSpeed(), damage, attackRange, entity.getColor(), entity.getProjectileRadius(), entity.getName());
                            projectiles.push_back(newP);
                        } else {
                            closestEnemy->takeDamage(damage);
                            closestEnemy->knockBackFrom(entity.getX(), entity.getY(), 40);
                        }
                        lastShotTime[entity.getName()] = currentTime;
                    }
                }
            }
        } else { entity.chooseDirection(nullptr); entity.setIsFleeing(false); entity.setIsCharging(false); }
    }
}

void Simulation::drawStatsPanel(SDL_Renderer* renderer, int panelX) {
    auto float_to_string = [](float val, int precision = 1) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    };

    if (inspectionStack.empty()) return;

    const Entity* entityToDisplay = (inspectionStack.size() == 1 && selectedLivingEntity) ? selectedLivingEntity : &inspectionStack.back();
    if (!entityToDisplay) return;
    const Entity& entity = *entityToDisplay;

    SDL_Rect panelRect = {panelX, 0, PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 240);
    SDL_RenderFillRect(renderer, &panelRect);

    int x = panelX + 15;
    int y = 20;
    const int lineHeight = 18;

    SDL_Color titleColor  = {255, 215, 0, 255};
    SDL_Color statColor   = {220, 220, 220, 255};
    SDL_Color geneColor   = {100, 200, 255, 255};
    SDL_Color linkColor   = {80, 80, 255, 255};
    SDL_Color badColor    = {255, 100, 100, 255};
    SDL_Color goodColor   = {100, 255, 100, 255};

    if (inspectionStack.size() > 1) {
        stringRGBA(renderer, x + 10, y + 8, "< Back", statColor.r, statColor.g, statColor.b, 255);
        panelBack_rect = {x, y, 80, 25};
        rectangleRGBA(renderer, panelBack_rect.x, panelBack_rect.y, panelBack_rect.x+panelBack_rect.w, panelBack_rect.y+panelBack_rect.h, 255,255,255,100);
        y += 40;
    } else {
        panelBack_rect = {0,0,0,0};
    }

    // --- EN-TÊTE ---
    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight*1.5;

    std::string hpStr = entityToDisplay == selectedLivingEntity ? std::to_string(entity.getHealth()) : "(Decede)";
    stringRGBA(renderer, x, y, ("Health: " + hpStr + " / " + std::to_string(entity.getMaxHealth())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;

    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;

    // --- GÉNÉALOGIE ---
    stringRGBA(renderer, x, y, "--- Genealogie ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Gen: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;

    stringRGBA(renderer, x, y, ("P1: " + entity.getParent1Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent1_rect = {x, y - 2, 250, lineHeight}; y += lineHeight;

    stringRGBA(renderer, x, y, ("P2: " + entity.getParent2Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent2_rect = {x, y - 2, 250, lineHeight}; y += lineHeight*1.5;

    // --- BODY STATS ---
    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;

    std::string typeStr = "Melee";
    int eType = entity.getEntityType();
    if(eType == 1) typeStr = "Ranged"; else if(eType == 2) typeStr = "Healer";

    stringRGBA(renderer, x, y, ("Type: " + typeStr).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor()*100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;

    // --- BIO-TRAITS ---
    stringRGBA(renderer, x, y, "--- Bio-Traits ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;

    int traitID = entity.getCurrentTraitID();
    const TraitStats& stats = TraitManager::get(traitID);

    std::string fullTraitTitle = stats.name;
    if (entity.getDamageFragility() > 0.10f) fullTraitTitle += " / Weak";
    if (entity.getMyopiaFactor() > 0.15f)    fullTraitTitle += " / Myopic";
    if (entity.getFertilityFactor() > 0)     fullTraitTitle += " / Fertile";
    if (entity.getAgingRate() > 0.0001f)     fullTraitTitle += " / Decaying";
    if (entity.getAimingPenalty() > 2.0f)    fullTraitTitle += " / Clumsy";

    stringRGBA(renderer, x, y, ("Trait: " + fullTraitTitle).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight;

    if (traitID != 0 && !stats.description.empty()) {
        stringRGBA(renderer, x, y, stats.description.c_str(), statColor.r, statColor.g, statColor.b, 255);
        y += lineHeight;
    }

    y += 5;

    float frag = entity.getDamageFragility() * 100.0f;
    if(frag > 1.0f) {
        stringRGBA(renderer, x, y, ("(Weak) Dmg Taken: +" + float_to_string(frag,1) + "%").c_str(), badColor.r, badColor.g, badColor.b, 255);
        y += lineHeight;
    }

    float myop = entity.getMyopiaFactor() * 100.0f;
    if(myop > 1.0f) {
        stringRGBA(renderer, x, y, ("(Myopic) Vision: -" + float_to_string(myop,1) + "%").c_str(), badColor.r, badColor.g, badColor.b, 255);
        y += lineHeight;
    }

    int fert = entity.getFertilityFactor();
    if(fert > 0) {
        // --- CORRECTION : PLUS DE .c_str() ICI ---
        stringRGBA(renderer, x, y, "(Fertile) Reproduction Bonus: ++", goodColor.r, goodColor.g, goodColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, "          Max HP Penalty: -", badColor.r, badColor.g, badColor.b, 255);
        y += lineHeight;
    }

    y += 10;

    // --- WEAPON STATS ---
    stringRGBA(renderer, x, y, "--- Weapon Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Damage: " + std::to_string(entity.getDamage())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Range: " + std::to_string(entity.getAttackRange())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Cooldown: " + std::to_string(entity.getAttackCooldown()) + " ms").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina Cost: " + std::to_string(entity.getStaminaAttackCost())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;

    if (entity.getProjectileSpeed() > 0) {
        stringRGBA(renderer, x, y, ("Proj Speed: " + std::to_string(entity.getProjectileSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
        stringRGBA(renderer, x, y, ("Proj Radius: " + std::to_string(entity.getProjectileRadius())).c_str(), statColor.r, statColor.g, statColor.b, 255);
    }
}
void Simulation::updatePhysics(int speedMultiplier) {
    // Mise à jour individuelle (mouvement)
    for (auto &entity : entities) {
        entity.update(speedMultiplier);
    }

    // Gestion des collisions entre entités (pour qu'elles ne se chevauchent pas)
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