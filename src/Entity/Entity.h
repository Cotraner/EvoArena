#ifndef EVOARENA_ENTITY_H
#define EVOARENA_ENTITY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "../constants.h"
#include <algorithm>
#include <string>
#include <cstdlib>
#include <map>

class Projectile;

class Entity {
public:
    // --- NOUVELLE SIGNATURE DU CONSTRUCTEUR (8 arguments) ---
    Entity(std::string name, int x, int y, SDL_Color color,
           const float geneticCode[12], int generation,
           std::string parent1_name, std::string parent2_name);
    ~Entity();

    void update(int speedMultiplier);
    void draw(SDL_Renderer* renderer, bool showDebug = false);
    void chooseDirection(int target[2] = nullptr);
    void knockBack();
    void clearTarget() { targetX = -1; targetY = -1; }


    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void attack(Entity &other);
    void die();
    void takeDamage(int amount);
    bool consumeStamina(int amount);
    void setIsFleeing(bool fleeing) { isFleeing = fleeing; }


    // --- Getters de Base (utilisent maintenant le nouveau membre rad) ---
    [[nodiscard]] std::string getName() const { return name; }
    SDL_Color getColor() const { return color; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; } // UTILISE LE MEMBRE DE CLASSE
    int getSightRadius() const { return sightRadius; }

    // --- Getters du Code Génétique (pour la Reproduction) ---
    const float* getGeneticCode() const { return geneticCode; }
    float getKiteRatio() const { return geneticCode[2]; }
    float getWeaponGene() const { return geneticCode[1]; } // FIX: Ajout du getter manquant
    bool getIsRanged() const { return geneticCode[10] > 0.5f; } // Tiré du code
    int getGeneration() const { return generation; }
    std::string getParent1Name() const { return parent1_name; }
    std::string getParent2Name() const { return parent2_name; }
    int getCurrentTraitID() const { return (int)geneticCode[11]; }

    // --- Getters des Stats Dérivées et Armes ---
    // ... (inchangé)
    int getHealth() const { return health; }
    void setHealth(int h) { health = h; }
    int getMaxHealth() const { return maxHealth; }
    int getStamina() const { return stamina; }
    int getMaxStamina() const { return maxStamina; }
    bool getIsAlive() const { return isAlive; }
    int getSpeed() const { return speed; }
    float getArmor() const { return armor; }
    int getDamage() const { return damage; }
    int getAttackRange() const { return attackRange; }
    Uint32 getAttackCooldown() const { return attackCooldown; }
    int getProjectileSpeed() const { return projectileSpeed; }
    int getProjectileRadius() const { return projectileRadius; }
    int getStaminaAttackCost() const { return staminaAttackCost; }

    // --- Getters des Gènes Bio-Inspirés (JSON) ---
    float getDamageFragility() const { return geneticCode[3]; }
    float getStaminaEfficiency() const { return geneticCode[4]; }
    float getBaseHealthRegen() const { return geneticCode[5]; }
    float getMyopiaFactor() const { return geneticCode[6]; }
    float getAimingPenalty() const { return geneticCode[7]; }
    int getFertilityFactor() const { return (int)geneticCode[9]; }
    float getAgingRate() const { return geneticCode[8]; }


    static SDL_Color generateRandomColor();

private:
    void calculateDerivedStats();

    // Constantes de Régénération
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000;
    static constexpr Uint32 STAMINA_REGEN_DELAY_MS = 2000;
    static constexpr int STAMINA_REGEN_RATE = 2;
    static constexpr int STAMINA_FLEE_COST_PER_FRAME = 1;

    // --- MEMBRES FONDAMENTAUX ---
    std::string name;
    int x,y;
    bool isAlive = true;
    SDL_Color color;
    int rad; // FIX: Réintroduction du membre de classe rad

    // --- CHROMOSOME (Le seul stockage génétique) ---
    float geneticCode[12];

    // --- STATS DÉRIVÉES (Stockage en mémoire) ---
    int health;
    int maxHealth;
    int speed;
    int sightRadius;
    int stamina;
    int maxStamina;
    int damage;
    int attackRange;
    Uint32 attackCooldown;
    int projectileSpeed;
    int projectileRadius;
    int staminaAttackCost;
    float armor = 0.0f;
    int regenAmount = 0; // Utilisé dans le CPP

    // --- Variables de Logique/Temps ---
    int direction[2]{};
    int targetX = -1;
    int targetY = -1;
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;
    Uint32 lastRegenTick = 0;
    Uint32 lastStaminaUseTick = 0;
    bool isFleeing = false;

    // --- Données de Généalogie ---
    int generation;
    std::string parent1_name;
    std::string parent2_name;
};

#endif //EVOARENA_ENTITY_H