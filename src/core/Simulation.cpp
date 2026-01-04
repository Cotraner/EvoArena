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
    foods.clear();

    panelCurrentX = (float)WINDOW_WIDTH;
    panelTargetX = (float)WINDOW_WIDTH;

    std::srand(std::time(0));

    for (int i = 0; i < initialEntityCount; ++i) {
        float newGeneticCode[14]; // Tableau de 14

        // --- 1. Physique & Position ---
        newGeneticCode[0] = 10.0f + (float)(std::rand() % 31);
        int randomRad = (int)newGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        std::string name = "G0-E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        // --- 2. Armement & Rôle ---
        newGeneticCode[1] = (float)(std::rand() % 101);
        newGeneticCode[2] = (10 + (std::rand() % 91)) / 100.0f;

        // Rôle
        int roleRoll = std::rand() % 100;
        if (roleRoll < 33) newGeneticCode[10] = 0.15f;      // Melee
        else if (roleRoll < 66) newGeneticCode[10] = 0.50f; // Ranged
        else newGeneticCode[10] = 0.85f;                    // Healer

        newGeneticCode[10] += ((float)(std::rand() % 11) - 5.0f) / 100.0f; // Variation

        // --- 3. Nettoyage Gènes Bio (Gen 0 Pure) ---
        newGeneticCode[3] = 0.0f;
        newGeneticCode[4] = 0.0f;
        newGeneticCode[5] = 0.0f;
        newGeneticCode[6] = 0.0f;
        newGeneticCode[7] = 0.0f;
        newGeneticCode[8] = 0.0f;
        newGeneticCode[9] = 0.0f;

        // --- 4. Choix du Trait (Index 11) ---
        int maxTraits = TraitManager::getCount();
        if (maxTraits > 1 && (std::rand() % 100) < 20) {
            int dominantTraitID = 1 + (std::rand() % (maxTraits - 1));
            newGeneticCode[11] = (float)dominantTraitID;
        } else {
            newGeneticCode[11] = 0.0f; // Classique
        }

        // --- 5. Gènes Comportementaux (Index 12 & 13) ---
        newGeneticCode[12] = (float)(std::rand() % 101) / 100.0f; // Bravoure
        newGeneticCode[13] = (float)(std::rand() % 101) / 100.0f; // Gourmandise

        // --- CRÉATION DE L'ENTITÉ (Une seule fois à la fin !) ---
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

    // Indices des gènes biologiques
    const std::vector<int> bioGeneIndices = {3, 4, 5, 6, 7, 8, 9};

    // --- CRÉATION DE LA LOTERIE (Sélection pondérée) ---
    std::vector<int> parentLottery;
    parentLottery.reserve(numParents * 10);

    for (int i = 0; i < numParents; ++i) {
        // Ticket de base pour avoir survécu
        int tickets = 1;
        // Bonus Fertilité
        tickets += parents[i].getFertilityFactor() * 3;
        // Bonus Trait "Fertile"
        if (parents[i].getCurrentTraitID() == 7) tickets += 15;

        for (int t = 0; t < tickets; ++t) parentLottery.push_back(i);
    }

    // --- GÉNÉRATION DES ENFANTS ---
    for (int i = 0; i < maxEntities; ++i) {
        // 1. Sélection des parents
        int p1_index = parentLottery[std::rand() % parentLottery.size()];
        int p2_index = parentLottery[std::rand() % parentLottery.size()];

        const Entity& parent1 = parents[p1_index];
        const Entity* parent2_ptr = &parents[p2_index];

        // Essayer de trouver un partenaire différent
        int attempts = 0;
        while (parent1.getName() == parent2_ptr->getName() && attempts < 15) {
            p2_index = parentLottery[std::rand() % parentLottery.size()];
            parent2_ptr = &parents[p2_index];
            attempts++;
        }
        const Entity& parent2 = *parent2_ptr;

        // --- 2. MÉLANGE GÉNÉTIQUE (MOYENNE) ---
        float childGeneticCode[14];

        // On parcourt TOUS les gènes (0 à 13)
        for (int idx = 0; idx < 14; ++idx) {
            // A. MOYENNE : L'enfant est la moyenne des deux parents
            float geneP1 = parent1.getGeneticCode()[idx];
            float geneP2 = parent2.getGeneticCode()[idx];
            childGeneticCode[idx] = (geneP1 + geneP2) / 2.0f;

            // B. MUTATION : Ajout d'un chaos aléatoire
            if ((std::rand() % 100) < MUTATION_CHANCE_PERCENT) {
                // Pour le Rôle (10), on mute plus fort pour permettre de changer de classe
                if (idx == 10) {
                    float change = (float)((std::rand() % 41) - 20) / 100.0f; // +/- 0.20
                    childGeneticCode[idx] += change;
                }
                    // Pour le rayon (0), on mute significativement
                else if (idx == 0) {
                    childGeneticCode[idx] += (float)((std::rand() % 7) - 3);
                }
                    // Pour les autres, petite variation
                else {
                    float mutation = (float)((std::rand() % 21) - 10) / 100.0f; // +/- 0.10
                    childGeneticCode[idx] += mutation;
                }
            }

            // C. LIMITES (Clamp)
            if (idx == 0) childGeneticCode[idx] = std::max(10.0f, std::min(50.0f, childGeneticCode[idx])); // Taille min/max
            else if (idx == 10) childGeneticCode[idx] = std::clamp(childGeneticCode[idx], 0.0f, 1.0f); // Rôle 0-1
            else if (idx != 11 && childGeneticCode[idx] < 0) childGeneticCode[idx] = 0.0f; // Pas de valeurs négatives (sauf Trait ID)
        }

        // --- 3. HÉRITAGE DU TRAIT (ID 11) ---
        // Les traits ne se "mélangent" pas (ce sont des ID entiers), on garde la logique dominante
        int chosenID = 0;
        int roll = std::rand() % 100;
        if (roll < 45) chosenID = parent1.getCurrentTraitID();
        else if (roll < 90) chosenID = parent2.getCurrentTraitID();
        else {
            int maxTraits = TraitManager::getCount();
            if (maxTraits > 1) chosenID = 1 + (std::rand() % (maxTraits - 1));
        }
        childGeneticCode[11] = (float)chosenID;

        // --- 4. MÉLANGE DES COULEURS (MOYENNE RGB) ---
        SDL_Color c1 = parent1.getColor();
        SDL_Color c2 = parent2.getColor();

        SDL_Color childColor;
        // La moyenne des composantes Rouge, Vert, Bleu
        childColor.r = (Uint8)std::clamp(((int)c1.r + (int)c2.r) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.g = (Uint8)std::clamp(((int)c1.g + (int)c2.g) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.b = (Uint8)std::clamp(((int)c1.b + (int)c2.b) / 2 + (std::rand()%21 - 10), 0, 255);
        childColor.a = 255;

        // --- Finalisation ---
        std::string newName = "G" + std::to_string(newGen) + "-E" + std::to_string(i + 1);
        int randomRad = (int)childGeneticCode[0];
        int randomX = randomRad + (std::rand() % (WINDOW_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_HEIGHT - 2 * randomRad));

        newGeneration.emplace_back(newName, randomX, randomY, childColor, childGeneticCode, newGen, parent1.getName(), parent2.getName());
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

    // --- GESTION NOURRITURE ---
    spawnFood();
    updateFood(speedMultiplier);
    // --------------------------

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

        // --- 1. PERCEPTION ---
        Entity* closestTarget = nullptr;
        float closestDist = 100000.0f;
        bool targetIsFriendly = false; // Pour savoir si on doit soigner ou taper

        for (auto &other : entities) {
            if (&entity != &other && other.getIsAlive()) {
                float dist = std::hypot(entity.getX() - other.getX(), entity.getY() - other.getY());

                bool isHealer = (entity.getEntityType() == 2);
                bool isAlly = entity.isAlliedWith(other); // Vrai si couleur proche

                bool isValidTarget = false;
                bool isFriendlyInteraction = false;

                if (isHealer) {
                    // Priorité Absolue : Allié blessé... MAIS PAS UN AUTRE HEALER !
                    // On ajoute : && other.getEntityType() != 2
                    if (isAlly && other.getHealth() < other.getMaxHealth() && other.getEntityType() != 2) {
                        isValidTarget = true;
                        isFriendlyInteraction = true;
                    }
                        // Sinon, on attaque (Self-defense ou Battle Medic)
                    else if (!isAlly) {
                        isValidTarget = true;
                        isFriendlyInteraction = false;
                    }
                }
                else {
                    // Pour les Melee/Ranged : On attaque tout le monde
                    isValidTarget = true;
                    isFriendlyInteraction = false;
                }

                if (isValidTarget) {
                    // On privilégie toujours le soin sur l'attaque si les distances sont proches
                    // Si on a déjà trouvé un patient (targetIsFriendly == true), on ne change pour un ennemi que s'il est VRAIMENT plus près
                    if (targetIsFriendly && !isFriendlyInteraction) {
                        // On garde le patient actuel sauf si l'ennemi est collé à nous (auto-défense)
                        if (dist < 20.0f) { // Danger immédiat
                            closestDist = dist;
                            closestTarget = &other;
                            targetIsFriendly = false;
                        }
                    }
                    else if (dist < closestDist) {
                        closestDist = dist;
                        closestTarget = &other;
                        targetIsFriendly = isFriendlyInteraction;
                    }
                }
            }
        }

        // Trouver la nourriture la plus proche
        int foodIndex = -1;
        float closestFoodDist = 100000.0f;
        for (size_t i = 0; i < foods.size(); ++i) {
            float d = std::hypot(entity.getX() - foods[i].x, entity.getY() - foods[i].y);
            if (d < closestFoodDist) {
                closestFoodDist = d;
                foodIndex = (int)i;
            }
        }

        // --- 2. PRISE DE DÉCISION (Changement d'état basé sur les gènes) ---
        float healthPct = (float)entity.getHealth() / (float)entity.getMaxHealth();
        float staminaPct = (float)entity.getStamina() / (float)entity.getMaxStamina();

        // Règle 1 : Fuite (Bravoure)
        // On ne fuit que si l'ennemi est VRAIMENT proche (< 150 pixels) ET qu'on est blessé.
        // Si l'ennemi est loin, fuir ne sert à rien, autant chercher à manger ou attendre.
        bool dangerClose = (closestTarget && closestDist < 150.0f);

        if (dangerClose && healthPct < entity.getBravery() && !entity.isAlliedWith(*closestTarget)) {
            // Si je suis coincé près d'un mur (marge de 50px), je ne peux plus fuir -> JE ME BATS !
            bool stuckAgainstWall = (entity.getX() < 50 || entity.getX() > WINDOW_WIDTH - 50 ||
                                     entity.getY() < 50 || entity.getY() > WINDOW_HEIGHT - 50);

            if (stuckAgainstWall) {
                entity.setCurrentState(Entity::COMBAT); // Dos au mur = Combat
            } else {
                entity.setCurrentState(Entity::FLEE);
            }
        }
            // Règle 2 : Manger (Gourmandise)
            // Si ma stamina est sous le seuil de gourmandise et qu'il y a de la nourriture
        else if (staminaPct < entity.getGreed() && foodIndex != -1) {
            entity.setCurrentState(Entity::FORAGE);
        }
            // Règle 3 : Combat / Soin
        else if (closestTarget && closestDist < entity.getSightRadius()) {
            entity.setCurrentState(Entity::COMBAT);
        }
            // Règle 4 : Par défaut
        else {
            entity.setCurrentState(Entity::WANDER);
        }

        // --- 3. ACTION (Exécution de l'état) ---
        entity.setIsFleeing(false);
        entity.setIsCharging(false);

        switch (entity.getCurrentState()) {
            case Entity::FLEE: {
                if (closestTarget) {
                    // Vecteur opposé à la menace
                    int fleeTarget[2] = {
                            entity.getX() + (entity.getX() - closestTarget->getX()),
                            entity.getY() + (entity.getY() - closestTarget->getY())
                    };
                    entity.chooseDirection(fleeTarget);
                    entity.setIsFleeing(true);
                }
                break;
            }

            case Entity::FORAGE: {
                if (foodIndex != -1) {
                    int target[2] = {foods[foodIndex].x, foods[foodIndex].y};
                    entity.chooseDirection(target);
                    // Pas de charge, mouvement normal vers la nourriture
                }
                break;
            }

            case Entity::COMBAT: {
                if (!closestTarget) break; // Sécurité

                int targetPos[2] = {closestTarget->getX(), closestTarget->getY()};
                int attackRange = entity.getAttackRange();
                bool isRanged = (entity.getEntityType() == 1);
                bool isHealer = (entity.getEntityType() == 2);

                // --- Logique de mouvement Combat ---
                if (isHealer) {
                    if (targetIsFriendly) {
                        // COMPORTEMENT DE SOIN (Classique)
                        if (closestDist < entity.getRad() + closestTarget->getRad() + 10)
                            entity.chooseDirection(nullptr);
                        else
                            entity.chooseDirection(targetPos);
                    } else {
                        // COMPORTEMENT D'ATTAQUE (Nouveau : Battle Medic)
                        // Le Healer se comporte comme un Ranged mais avec moins de portée
                        int attackRange = entity.getAttackRange(); // Assurez-vous que les Healers ont une portée > 0
                        if (closestDist > attackRange) entity.chooseDirection(targetPos);
                        else entity.chooseDirection(nullptr);
                    }
                }
                else if (isRanged) {
                    // Logique de Kiting (Similaire à avant)
                    float kiteDist = attackRange * entity.getKiteRatio();
                    if (closestDist < kiteDist && entity.getStamina() > 10) {
                        // Trop près -> Recule
                        int back[2] = {entity.getX() + (entity.getX() - targetPos[0]), entity.getY() + (entity.getY() - targetPos[1])};
                        entity.chooseDirection(back);
                    } else if (closestDist > attackRange) {
                        entity.chooseDirection(targetPos);
                    } else {
                        entity.chooseDirection(nullptr); // Position de tir idéale
                    }
                }
                else { // Melee
                    entity.setIsCharging(closestDist > attackRange);
                    if (closestDist <= attackRange) entity.chooseDirection(nullptr);
                    else entity.chooseDirection(targetPos);
                }

                // --- Logique de Tir / Frappe ---
                if (closestDist <= attackRange + 10) {
                    Uint32 currentTime = SDL_GetTicks();
                    Uint32 cooldown = entity.getAttackCooldown();
                    Uint32 effectiveCooldown = (speedMultiplier > 0) ? (cooldown / speedMultiplier) : cooldown;

                    if (lastShotTime.find(entity.getName()) == lastShotTime.end() || currentTime > lastShotTime[entity.getName()] + effectiveCooldown) {

                        // Coût en stamina standard
                        if (entity.consumeStamina(entity.getStaminaAttackCost())) {

                            if (isHealer && targetIsFriendly) {
                                // --- MECANIQUE DE SACRIFICE (Healer) ---
                                int healAmount = entity.getDamage();
                                int selfDamage = healAmount; // Le soigneur perd le soin donné

                                // Sécurité : Le Healer ne se suicide pas pour soigner
                                if (entity.getHealth() > selfDamage) {
                                    closestTarget->receiveHealing(healAmount);
                                    entity.takeDamage(selfDamage);
                                }
                            }
                            else if (isHealer && !targetIsFriendly) {
                                // ATTAQUE (Battle Medic) - Inchangé
                                Projectile newP(entity.getX(), entity.getY(), closestTarget->getX(), closestTarget->getY(),
                                                entity.getProjectileSpeed(), entity.getDamage(), entity.getAttackRange(),
                                                entity.getColor(), entity.getProjectileRadius(), entity.getName());
                                projectiles.push_back(newP);
                            }
                            else if (isRanged) {
                                // RANGED - Inchangé
                                Projectile newP(entity.getX(), entity.getY(), closestTarget->getX(), closestTarget->getY(),
                                                entity.getProjectileSpeed(), entity.getDamage(), attackRange,
                                                entity.getColor(), entity.getProjectileRadius(), entity.getName());
                                projectiles.push_back(newP);
                            }
                            else {
                                // MELEE - Inchangé
                                closestTarget->takeDamage(entity.getDamage());
                                closestTarget->knockBackFrom(entity.getX(), entity.getY(), 40);
                            }
                            lastShotTime[entity.getName()] = currentTime;
                        }
                    }
                }
                break;
            }

            case Entity::WANDER:
            default: {
                // --- LOGIQUE DE TRAQUE GLOBALE ---
                // Si je ne suis pas en combat ou en train de manger, je cherche une cible
                // n'importe où sur la carte pour me rapprocher.

                Entity* globalTarget = nullptr;
                float minGlobalDist = 1000000.0f;

                for (auto &other : entities) {
                    if (&entity != &other && other.getIsAlive()) {
                        // Je cible tout ce qui n'est pas mon allié
                        // (Ou même mes alliés si on est en fin de partie < 5 survivants)
                        bool isEnemy = !entity.isAlliedWith(other);
                        bool isEndGameTreaosn = (entity.getEntityType() == 2 && entities.size() < 5); // Les healers trahissent à la fin

                        if (isEnemy || isEndGameTreaosn) {
                            float d = std::hypot(entity.getX() - other.getX(), entity.getY() - other.getY());
                            if (d < minGlobalDist) {
                                minGlobalDist = d;
                                globalTarget = &other;
                            }
                        }
                    }
                }

                if (globalTarget) {
                    // Aim the closest target
                    int targetPos[2] = {globalTarget->getX(), globalTarget->getY()};
                    entity.chooseDirection(targetPos);
                } else {
                    // No targets found, wander towards center
                    int center[2] = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
                    entity.chooseDirection(center);
                }
                break;
            }
        }
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

    // Couleurs
    SDL_Color titleColor  = {255, 215, 0, 255};
    SDL_Color statColor   = {220, 220, 220, 255};
    SDL_Color geneColor   = {100, 200, 255, 255};
    SDL_Color linkColor   = {80, 80, 255, 255};
    SDL_Color badColor    = {255, 100, 100, 255};
    SDL_Color goodColor   = {100, 255, 100, 255};
    SDL_Color stateColor  = {255, 165, 0, 255}; // Orange pour l'état

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

    // --- ETAT ACTUEL (Votre ajout) ---
    // Je l'ai remonté ici car c'est une info dynamique importante
    stringRGBA(renderer, x, y, ("CURRENT STATE: " + entity.getCurrentStateString()).c_str(), stateColor.r, stateColor.g, stateColor.b, 255);
    y += lineHeight * 1.5;

    // --- COMPORTEMENT (Nouveau : Bravoure & Gourmandise) ---
    stringRGBA(renderer, x, y, "--- Psychology ---", geneColor.r, geneColor.g, geneColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Bravery (Flee thresh): " + float_to_string(entity.getBravery() * 100, 0) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight;
    stringRGBA(renderer, x, y, ("Greed (Eat thresh): " + float_to_string(entity.getGreed() * 100, 0) + "%").c_str(), statColor.r, statColor.g, statColor.b, 255); y += lineHeight * 1.5;


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
    // --- DESSIN NOURRITURE ---
    for (const auto& f : foods) {
        // Cercle plein vert foncé + contour vert clair
        filledCircleRGBA(renderer, f.x, f.y, f.radius, 34, 139, 34, 255);
        circleRGBA(renderer, f.x, f.y, f.radius, 144, 238, 144, 200);
    }
    // -------------------------

    for (auto &entity : entities) entity.draw(renderer, showDebug);
    for (auto &proj : projectiles) proj.draw(renderer);

    if (selectedLivingEntity != nullptr) circleRGBA(renderer, selectedLivingEntity->getX(), selectedLivingEntity->getY(), selectedLivingEntity->getRad() + 4, 255, 255, 0, 200);
    if (panelCurrentX < WINDOW_WIDTH) drawStatsPanel(renderer, static_cast<int>(panelCurrentX));
}

void Simulation::spawnFood() {
    // Si on n'a pas atteint la limite max, on a une chance de faire pousser une plante
    if (foods.size() < MAX_FOOD_COUNT && (rand() % 100 < FOOD_SPAWN_RATE)) {
        Food f;
        // On évite les bords de l'écran (marge de 20px)
        f.x = 20 + (rand() % (WINDOW_WIDTH - 40));
        f.y = 20 + (rand() % (WINDOW_HEIGHT - 40));
        foods.push_back(f);
    }
}

void Simulation::updateFood(int speedMultiplier) { // Vérifie que tu as bien int speedMultiplier ici
    auto it = foods.begin();
    while (it != foods.end()) {
        bool eaten = false;
        for (auto &entity : entities) {
            if (!entity.getIsAlive()) continue;

            int dx = entity.getX() - it->x;
            int dy = entity.getY() - it->y;
            float dist = std::sqrt((float)(dx*dx + dy*dy));

            if (dist < (entity.getRad() + it->radius)) {

                // --- CORRECTION ICI ---
                // Il fallait ajouter ", speedMultiplier"
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