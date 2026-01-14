#include "Simulation.h"
#include "../constants.h"
#include "../Entity/TraitManager.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

namespace {
    // Constants for UI and genetic parameters
    const int PANEL_WIDTH = 300;
    const int SURVIVOR_COUNT = 20;
    const int MUTATION_CHANCE_PERCENT = 5;
    const int RAD_MUTATION_AMOUNT = 3;
    const int GENE_MUTATION_AMOUNT = 10;

    // Genetic limits
    const float FRAGILITY_MAX = 0.3f;
    const float EFFICIENCY_MAX = 0.5f;
    const float REGEN_MAX = 0.2f;
    const float MYOPIA_MAX = 0.5f;
    const float AIM_MAX = 15.0f;
    const int FERTILITY_MAX_INT = 2;
    const float AGING_MAX_FLOAT = 0.01f;
}

// Constructor: Initializes the simulation with the maximum number of entities
Simulation::Simulation(int maxEntities) :
        maxEntities(maxEntities),
        selectedLivingEntity(nullptr) {
    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;
    TraitManager::loadTraits("../assets/json/mutations.JSON");
    initialize(maxEntities);
}

// Destructor: Cleans up resources (automatic for STL containers)
Simulation::~Simulation() = default;

// Initializes the simulation with a given number of entities
void Simulation::initialize(int initialEntityCount) {
    currentGeneration = 0;
    entities.clear();
    projectiles.clear();
    lastShotTime.clear();
    genealogyArchive.clear();
    inspectionStack.clear();
    selectedLivingEntity = nullptr;
    foods.clear();

    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;

    std::srand(std::time(0));

    for (int i = 0; i < initialEntityCount; ++i) {
        float newGeneticCode[14];

        // Generate random genetic code for the entity
        newGeneticCode[0] = 10.0f + (float)(std::rand() % 31); // Size
        int randomRad = (int)newGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WORLD_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WORLD_HEIGHT - 2 * randomRad));
        std::string name = "G0-E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        // Weapon and role assignment
        newGeneticCode[1] = (float)(std::rand() % 101); // Weapon type
        newGeneticCode[2] = (10 + (std::rand() % 91)) / 100.0f; // Kite distance
        int roleRoll = std::rand() % 100;
        newGeneticCode[10] = (roleRoll < 33) ? 0.15f : (roleRoll < 66) ? 0.50f : 0.85f;
        newGeneticCode[10] += ((float)(std::rand() % 11) - 5.0f) / 100.0f;

        // Reset biological genes
        for (int j = 3; j <= 9; ++j) newGeneticCode[j] = 0.0f;

        // Assign a dominant trait
        int maxTraits = TraitManager::getCount();
        newGeneticCode[11] = (maxTraits > 1 && (std::rand() % 100) < 20) ? 
                             (float)(1 + (std::rand() % (maxTraits - 1))) : 0.0f;

        // Behavioral genes
        newGeneticCode[12] = (float)(std::rand() % 101) / 100.0f; // Bravery
        newGeneticCode[13] = (float)(std::rand() % 101) / 100.0f; // Greed

        entities.emplace_back(name, randomX, randomY, color, newGeneticCode, currentGeneration, "NONE", "NONE");
    }
}

// Handles reproduction and creates a new generation
void Simulation::triggerReproduction(const std::vector<Entity>& parents) {
    std::vector<Entity> newGeneration;
    int numParents = parents.size();

    if (parents.empty() || numParents < 2) {
        initialize(this->maxEntities);
        return;
    }

    int newGen = parents[0].getGeneration() + 1;
    this->currentGeneration = newGen;

    // Create a lottery for parent selection
    std::vector<int> parentLottery;
    parentLottery.reserve(numParents * 10);

    for (int i = 0; i < numParents; ++i) {
        int tickets = 1;
        tickets += parents[i].getFertilityFactor() * 3;
        if (parents[i].getCurrentTraitID() == 7) tickets += 15; // Trait Fertile

        for (int t = 0; t < tickets; ++t) parentLottery.push_back(i);
    }

    // Generate children
    for (int i = 0; i < maxEntities; ++i) {
        int p1_index = parentLottery[std::rand() % parentLottery.size()];
        int p2_index = parentLottery[std::rand() % parentLottery.size()];

        const Entity& parent1 = parents[p1_index];
        const Entity* parent2_ptr = &parents[p2_index];

        int attempts = 0;
        while (parent1.getName() == parent2_ptr->getName() && attempts < 15) {
            p2_index = parentLottery[std::rand() % parentLottery.size()];
            parent2_ptr = &parents[p2_index];
            attempts++;
        }
        const Entity& parent2 = *parent2_ptr;

        float childGeneticCode[14];

        for (int idx = 0; idx < 14; ++idx) {
            float geneP1 = parent1.getGeneticCode()[idx];
            float geneP2 = parent2.getGeneticCode()[idx];
            int crossoverStrategy = std::rand() % 3;

            if (crossoverStrategy == 0) {
                childGeneticCode[idx] = (geneP1 + geneP2) / 2.0f;
            } else if (crossoverStrategy == 1) {
                childGeneticCode[idx] = (std::rand() % 2 == 0) ? geneP1 : geneP2;
            } else  {
                float ratio = (float)(std::rand() % 101) / 100.0f;
                childGeneticCode[idx] = geneP1 * ratio + geneP2 * (1.0f - ratio);
            }

            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                if (idx == 10) { // Role
                    childGeneticCode[idx] += ((float)((std::rand() % 41) - 20) / 100.0f);
                } else if (idx == 0) { // Size
                    childGeneticCode[idx] += (float)((std::rand() % 7) - 3);
                } else {
                    childGeneticCode[idx] += ((float)((std::rand() % 21) - 10) / 100.0f);
                }
            }

            // Clamp values
            if (idx == 0) childGeneticCode[idx] = std::max(10.0f, std::min(50.0f, childGeneticCode[idx]));
            else if (idx == 10) childGeneticCode[idx] = std::clamp(childGeneticCode[idx], 0.0f, 1.0f);
            else if (idx != 11 && childGeneticCode[idx] < 0) childGeneticCode[idx] = 0.0f;
        }

        // Trait inheritance
        int chosenID = 0;
        int roll = std::rand() % 100;
        if (roll < 45) chosenID = parent1.getCurrentTraitID();
        else if (roll < 90) chosenID = parent2.getCurrentTraitID();
        else {
            int maxTraits = TraitManager::getCount();
            if (maxTraits > 1) chosenID = 1 + (std::rand() % (maxTraits - 1));
        }
        childGeneticCode[11] = (float)chosenID;

        // Color inheritance
        SDL_Color c1 = parent1.getColor();
        SDL_Color c2 = parent2.getColor();
        SDL_Color childColor;
        childColor.r = (Uint8)std::clamp(((int)c1.r + (int)c2.r) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.g = (Uint8)std::clamp(((int)c1.g + (int)c2.g) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.b = (Uint8)std::clamp(((int)c1.b + (int)c2.b) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.a = 255;

        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomRad = (int)childGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WORLD_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WORLD_HEIGHT - 2 * randomRad));
        newGeneration.emplace_back(newName, randomX, randomY, childColor, childGeneticCode, newGen, parent1.getName(), parent2.getName());
    }

    entities = std::move(newGeneration);
    selectedLivingEntity = nullptr;
    inspectionStack.clear();
}

// Restarts the simulation manually
void Simulation::triggerManualRestart() {
    triggerReproduction(lastSurvivors);
}

// Handles user input events
void Simulation::handleEvent(const SDL_Event &event, const Camera& cam) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        SDL_Point mousePoint = {mouseX, mouseY};

        // Handle clicks on the UI panel
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

        // Handle clicks in the world (entity selection)
        float worldMouseX = mouseX / cam.zoom + cam.x;
        float worldMouseY = mouseY / cam.zoom + cam.y;
        bool entityClicked = false;
        for (auto& entity : entities) {
            if (!entity.getIsAlive()) continue;
            int dx = worldMouseX - entity.getX();
            int dy = worldMouseY - entity.getY();
            if (std::sqrt((float)dx*dx + dy*dy) < entity.getRad()) {
                selectedLivingEntity = &entity;
                inspectionStack.clear();
                inspectionStack.push_back(entity);
                entityClicked = true;
                break;
            }
        }
        if (!entityClicked) { selectedLivingEntity = nullptr; inspectionStack.clear(); }
    }
}

// Updates the simulation state, including multithreaded logic and physics
Simulation::SimUpdateStatus Simulation::update(int speedMultiplier, bool autoRestart) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    std::vector<std::thread> threads;
    int totalEntities = entities.size();
    int chunkSize = totalEntities / numThreads;

    // Launch threads for logic and physics updates
    for (unsigned int i = 0; i < numThreads; ++i) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? totalEntities : (start + chunkSize);

        threads.emplace_back([this, start, end, speedMultiplier]() {
            this->updateLogicAndPhysicsRange(start, end, speedMultiplier);
        });
    }

    // Wait for threads to finish
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // Sequential updates
    spawnFood();
    updateFood(speedMultiplier);
    updateProjectiles();
    cleanupDead();

    // Handle end of generation
    if (entities.size() <= SURVIVOR_COUNT && !entities.empty()) {
        lastSurvivors = entities;
        for (const auto& winner : lastSurvivors) genealogyArchive.insert({winner.getName(), winner});
        if (autoRestart) { triggerReproduction(lastSurvivors); return SimUpdateStatus::RUNNING; }
        else return SimUpdateStatus::FINISHED;
    }

    // UI animation
    if (!inspectionStack.empty()) panelTargetX = (float)(WINDOW_WIDTH - PANEL_WIDTH); else panelTargetX = (float)WINDOW_WIDTH;
    if(selectedLivingEntity != nullptr && !selectedLivingEntity->getIsAlive()) selectedLivingEntity = nullptr;
    float distance = panelTargetX - panelCurrentX;
    if (std::abs(distance) < 1.0f) panelCurrentX = panelTargetX; else panelCurrentX += distance * 0.1f;

    return SimUpdateStatus::RUNNING;
}

// Updates a range of entities' logic and physics (used by threads)
void Simulation::updateLogicAndPhysicsRange(int startIdx, int endIdx, int speedMultiplier) {
    for (int i = startIdx; i < endIdx; ++i) {
        Entity& entity = entities[i];
        if (!entity.getIsAlive()) continue;

        // Perception and decision-making
        Entity* closestTarget = nullptr;
        float closestDist = 100000.0f;
        bool targetIsFriendly = false;

        for (auto &other : entities) {
            if (&entity != &other && other.getIsAlive()) {
                float dist = std::hypot(entity.getX() - other.getX(), entity.getY() - other.getY());

                bool isHealer = (entity.getEntityType() == 2);
                bool isAlly = entity.isAlliedWith(other);
                bool isValidTarget = false;
                bool isFriendlyInteraction = false;

                if (isHealer) {
                    if (isAlly && other.getHealth() < other.getMaxHealth() && other.getEntityType() != 2) {
                        isValidTarget = true;
                        isFriendlyInteraction = true;
                    } else if (!isAlly) {
                        isValidTarget = true;
                        isFriendlyInteraction = false;
                    }
                } else {
                    isValidTarget = true;
                    isFriendlyInteraction = false;
                }

                if (isValidTarget) {
                    if (targetIsFriendly && !isFriendlyInteraction) {
                        if (dist < 20.0f) {
                            closestDist = dist; closestTarget = &other; targetIsFriendly = false;
                        }
                    } else if (dist < closestDist) {
                        closestDist = dist; closestTarget = &other; targetIsFriendly = isFriendlyInteraction;
                    }
                }
            }
        }

        // Food perception
        int foodIndex = -1;
        float closestFoodDist = 100000.0f;
        for (size_t k = 0; k < foods.size(); ++k) {
            float d = std::hypot(entity.getX() - foods[k].x, entity.getY() - foods[k].y);
            if (d < closestFoodDist) { closestFoodDist = d; foodIndex = (int)k; }
        }

        // Decision-making
        float healthPct = (float)entity.getHealth() / (float)entity.getMaxHealth();
        float staminaPct = (float)entity.getStamina() / (float)entity.getMaxStamina();
        bool dangerClose = (closestTarget && closestDist < 150.0f);

        if (dangerClose && healthPct < entity.getBravery() && !entity.isAlliedWith(*closestTarget)) {
            bool stuck = (entity.getX() < 50 || entity.getX() > WORLD_WIDTH - 50 ||
                          entity.getY() < 50 || entity.getY() > WORLD_HEIGHT - 50);
            entity.setCurrentState(stuck ? Entity::COMBAT : Entity::FLEE);
        } else if (staminaPct < entity.getGreed() && foodIndex != -1) {
            entity.setCurrentState(Entity::FORAGE);
        } else if (closestTarget && closestDist < entity.getSightRadius()) {
            entity.setCurrentState(Entity::COMBAT);
        } else {
            entity.setCurrentState(Entity::WANDER);
        }

        // Action execution
        entity.setIsFleeing(false);
        entity.setIsCharging(false);

        switch (entity.getCurrentState()) {
            case Entity::FLEE: {
                if (closestTarget) {
                    int fleeTarget[2] = { entity.getX() + (entity.getX() - closestTarget->getX()),
                                          entity.getY() + (entity.getY() - closestTarget->getY()) };
                    entity.chooseDirection(fleeTarget);
                    entity.setIsFleeing(true);
                }
                break;
            }
            case Entity::FORAGE: {
                if (foodIndex != -1) {
                    int target[2] = {foods[foodIndex].x, foods[foodIndex].y};
                    entity.chooseDirection(target);
                }
                break;
            }
            case Entity::COMBAT: {
                if (!closestTarget) break;
                int targetPos[2] = {closestTarget->getX(), closestTarget->getY()};
                int attackRange = entity.getAttackRange();
                bool isRanged = (entity.getEntityType() == 1);
                bool isHealer = (entity.getEntityType() == 2);

                // Combat movement
                if (isHealer) {
                    if (targetIsFriendly) {
                        if (closestDist < entity.getRad() + closestTarget->getRad() + 10) entity.chooseDirection(nullptr);
                        else entity.chooseDirection(targetPos);
                    } else {
                        if (closestDist > attackRange) entity.chooseDirection(targetPos);
                        else entity.chooseDirection(nullptr);
                    }
                } else if (isRanged) {
                    float kiteDist = attackRange * entity.getKiteRatio();
                    if (closestDist < kiteDist && entity.getStamina() > 10) {
                        int back[2] = {entity.getX() + (entity.getX() - targetPos[0]), entity.getY() + (entity.getY() - targetPos[1])};
                        entity.chooseDirection(back);
                    } else if (closestDist > attackRange) {
                        entity.chooseDirection(targetPos);
                    } else {
                        entity.chooseDirection(nullptr);
                    }
                } else {
                    entity.setIsCharging(closestDist > attackRange);
                    if (closestDist <= attackRange) entity.chooseDirection(nullptr);
                    else entity.chooseDirection(targetPos);
                }

                // Attack or heal
                if (closestDist <= attackRange + 10) {
                    Uint32 currentTime = SDL_GetTicks();
                    Uint32 effectiveCooldown = (speedMultiplier > 0) ? (entity.getAttackCooldown() / speedMultiplier) : entity.getAttackCooldown();

                    bool canShoot = false;
                    {
                        std::lock_guard<std::mutex> lock(simMutex);
                        if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + effectiveCooldown) {
                            canShoot = true;
                        }
                    }

                    if (canShoot) {
                        if (entity.consumeStamina(entity.getStaminaAttackCost())) {
                            std::lock_guard<std::mutex> lock(simMutex);

                            lastShotTime[entity.getName()] = currentTime;

                            if (isHealer && targetIsFriendly) {
                                int healAmount = entity.getDamage();
                                if (entity.getHealth() > healAmount) {
                                    closestTarget->receiveHealing(healAmount);
                                    entity.takeDamage(healAmount);
                                }
                            } else if (isHealer && !targetIsFriendly) {
                                Projectile newP(entity.getX(), entity.getY(), closestTarget->getX(), closestTarget->getY(),
                                                entity.getProjectileSpeed(), entity.getDamage(), entity.getAttackRange(),
                                                entity.getColor(), entity.getProjectileRadius(), entity.getName());
                                projectiles.push_back(newP);
                            } else if (isRanged) {
                                Projectile newP(entity.getX(), entity.getY(), closestTarget->getX(), closestTarget->getY(),
                                                entity.getProjectileSpeed(), entity.getDamage(), attackRange,
                                                entity.getColor(), entity.getProjectileRadius(), entity.getName());
                                projectiles.push_back(newP);
                            } else {
                                closestTarget->takeDamage(entity.getDamage());
                                closestTarget->knockBackFrom(entity.getX(), entity.getY(), 40);
                            }
                        }
                    }
                }
                break;
            }
            case Entity::WANDER:
            default: {
                Entity* globalTarget = nullptr;
                float minGlobalDist = 1000000.0f;
                for (auto &other : entities) {
                    if (&entity != &other && other.getIsAlive()) {
                        bool isEnemy = !entity.isAlliedWith(other);
                        bool isEndGameTreason = (entity.getEntityType() == 2 && entities.size() < 5);
                        if (isEnemy || isEndGameTreason) {
                            float d = std::hypot(entity.getX() - other.getX(), entity.getY() - other.getY());
                            if (d < minGlobalDist) { minGlobalDist = d; globalTarget = &other; }
                        }
                    }
                }
                if (globalTarget) {
                    int targetPos[2] = {globalTarget->getX(), globalTarget->getY()};
                    entity.chooseDirection(targetPos);
                } else {
                    int center[2] = {WORLD_WIDTH / 2, WORLD_HEIGHT / 2};
                    entity.chooseDirection(center);
                }
                break;
            }
        }

        // Update physics
        entity.update(speedMultiplier);

        // Handle collisions
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

                    {
                        std::lock_guard<std::mutex> lock(simMutex);
                        entity.setX(entity.getX() + static_cast<int>(normX * separationDistance));
                        entity.setY(entity.getY() + static_cast<int>(normY * separationDistance));
                    }
                }
            }
        }
    }
}

// Draws the stats panel for the selected entity
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
    SDL_Color stateColor  = {255, 165, 0, 255};

    if (inspectionStack.size() > 1) {
        stringRGBA(renderer, x + 10, y + 8, "< Back", statColor.r, statColor.g, statColor.b, 255);
        panelBack_rect = {x, y, 80, 25};
        rectangleRGBA(renderer, panelBack_rect.x, panelBack_rect.y, panelBack_rect.x+panelBack_rect.w, panelBack_rect.y+panelBack_rect.h, 255,255,255,100);
        y += 40;
    } else {
        panelBack_rect = {0,0,0,0};
    }

    stringRGBA(renderer, x, y, ("ID: " + entity.getName()).c_str(), titleColor.r, titleColor.g, titleColor.b, 255); y += lineHeight*1.5;

    std::string hpStr = entityToDisplay == selectedLivingEntity ? std::to_string(entity.getHealth()) : "(Decede)";
    stringRGBA(renderer, x, y, ("Health: " + hpStr + " / " + std::to_string(entity.getMaxHealth())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Stamina: " + std::to_string(entity.getStamina())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;
    stringRGBA(renderer, x, y, ("CURRENT STATE: " + entity.getCurrentStateString()).c_str(), stateColor.r, stateColor.g, stateColor.b, 255);
    y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Psychology ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Bravery (Flee thresh): " + float_to_string(entity.getBravery() * 100, 0) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Greed (Eat thresh): " + float_to_string(entity.getGreed() * 100, 0) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight * 1.5;

    stringRGBA(renderer, x, y, "--- Genealogie ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Gen: " + std::to_string(entity.getGeneration())).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("P1: " + entity.getParent1Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent1_rect = {x, y - 2, 250, lineHeight}; y += lineHeight;
    stringRGBA(renderer, x, y, ("P2: " + entity.getParent2Name()).c_str(), linkColor.r, linkColor.g, linkColor.b, 255);
    panelParent2_rect = {x, y - 2, 250, lineHeight}; y += lineHeight*1.5;

    stringRGBA(renderer, x, y, "--- Body Stats ---", statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    std::string typeStr = "Melee";
    int eType = entity.getEntityType();
    if(eType == 1) typeStr = "Ranged"; else if(eType == 2) typeStr = "Healer";
    stringRGBA(renderer, x, y, ("Type: " + typeStr).c_str(), geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Rad (Size): " + std::to_string(entity.getRad())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Speed: " + std::to_string(entity.getSpeed())).c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Armor: " + float_to_string(entity.getArmor()*100) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight*1.5;

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
        stringRGBA(renderer, x, y, "(Fertile) Reproduction Bonus: ++", goodColor.r, goodColor.g, goodColor.b, 255);
        y += lineHeight;
        stringRGBA(renderer, x, y, "          Max HP Penalty: -", badColor.r, badColor.g, badColor.b, 255);
        y += lineHeight;
    }
    y += 10;

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

// Updates the state of all projectiles
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

// Removes dead entities from the simulation
void Simulation::cleanupDead() {
    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity &entity) { return !entity.getIsAlive(); }), entities.end());
}

// Renders the simulation, including entities, projectiles, and UI
void Simulation::render(SDL_Renderer* renderer, bool showDebug, const Camera& cam) {
    for (const auto& f : foods) {
        float sx = (f.x - cam.x) * cam.zoom;
        float sy = (f.y - cam.y) * cam.zoom;
        float sr = f.radius * cam.zoom;
        filledCircleRGBA(renderer, (int)sx, (int)sy, (int)sr, 34, 139, 34, 255);
        circleRGBA(renderer, (int)sx, (int)sy, (int)sr, 144, 238, 144, 200);
    }
    for (auto &entity : entities) entity.draw(renderer, cam, showDebug);
    for (auto &proj : projectiles) proj.draw(renderer, cam);

    if (selectedLivingEntity != nullptr) {
        float sx = (selectedLivingEntity->getX() - cam.x) * cam.zoom;
        float sy = (selectedLivingEntity->getY() - cam.y) * cam.zoom;
        float sr = (selectedLivingEntity->getRad() + 4) * cam.zoom;
        circleRGBA(renderer, (int)sx, (int)sy, (int)sr, 255, 255, 0, 200);
    }

    if (panelCurrentX < WINDOW_WIDTH) drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
}

// Spawns food items in the simulation
void Simulation::spawnFood() {
    if (foods.size() < MAX_FOOD_COUNT && (rand() % 100 < FOOD_SPAWN_RATE)) {
        Food f;
        f.x = 20 + (rand() % (WORLD_WIDTH - 40));
        f.y = 20 + (rand() % (WORLD_HEIGHT - 40));
        foods.push_back(f);
    }
}

// Updates the state of food items, including consumption by entities
void Simulation::updateFood(int speedMultiplier) {
    auto it = foods.begin();
    while (it != foods.end()) {
        bool eaten = false;
        for (auto &entity : entities) {
            if (!entity.getIsAlive()) continue;

            int dx = entity.getX() - it->x;
            int dy = entity.getY() - it->y;
            float dist = std::sqrt((float)(dx*dx + dy*dy));

            if (dist < (entity.getRad() + it->radius)) {
                entity.restoreStamina(50, speedMultiplier);
                eaten = true;
                break;
            }
        }
        if (eaten) {
            it = foods.erase(it);
        } else {
            ++it;
        }
    }
}
