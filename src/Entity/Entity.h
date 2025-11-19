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

class Projectile; // Forward declaration

class Entity {
public:
    // --- NOUVELLE SIGNATURE DU CONSTRUCTEUR (16 arguments déduits du CPP) ---
    Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene,
           int generation, std::string parent1_name, std::string parent2_name,
           int weaponGene, float kiteRatioGene,
            // NOUVEAUX GÈNES (7 arguments flottants/entiers)
           float damageFragility, float staminaEfficiency, float baseHealthRegen,
           float myopiaFactor, float aimingPenalty, int fertilityFactor, float agingRate);
    ~Entity(); // Le destructeur doit être déclaré

    // --- Fonctions de Base ---
    void update(int speedMultiplier);
    void draw(SDL_Renderer* renderer, bool showDebug = false);
    void chooseDirection(int target[2] = nullptr);
    void knockBack();
    void clearTarget() { targetX = -1; targetY = -1; }

    // --- Fonctions de Logique / Accesseurs Mutables ---
    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void attack(Entity &other); // Déclarée mais non utilisée par le main actuel
    void die();
    void takeDamage(int amount); // Utilisée pour les projectiles et les attaques Melee
    bool consumeStamina(int amount); // Utilisée pour le coût d'attaque
    void setHealth(int h) { health = h; }
    void setStamina(int s) { stamina = s; }
    void setIsFleeing(bool fleeing) { isFleeing = fleeing; }


    // --- Getters ---
    [[nodiscard]] std::string getName() const { return name; }
    SDL_Color getColor() const { return color; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; }
    int getSightRadius() const { return sightRadius; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getStamina() const { return stamina; }
    int getMaxStamina() const { return maxStamina; }
    bool getIsAlive() const { return isAlive; }

    // --- Getters de Gènes et Stats Dérivées ---
    int getSpeed() const { return speed; }
    float getArmor() const { return armor; }
    int getWeaponGene() const { return weaponGene; }
    float getKiteRatio() const { return kite_ratio_gene; }
    int getGeneration() const { return generation; }
    std::string getParent1Name() const { return parent1_name; }
    std::string getParent2Name() const { return parent2_name; }

    // --- Getters d'Armes/Combat ---
    [[nodiscard]] bool getIsRanged() const { return isRanged; }
    [[nodiscard]] int getDamage() const { return damage; }
    [[nodiscard]] int getAttackRange() const { return attackRange; }
    [[nodiscard]] Uint32 getAttackCooldown() const { return attackCooldown; }
    [[nodiscard]] int getProjectileSpeed() const { return projectileSpeed; }
    [[nodiscard]] int getProjectileRadius() const { return projectileRadius; }
    [[nodiscard]] int getStaminaAttackCost() const { return staminaAttackCost; }

    // --- Getters des Nouveaux Gènes (JSON) ---
    float getDamageFragility() const { return damageFragility; }
    float getStaminaEfficiency() const { return staminaEfficiency; }
    float getBaseHealthRegen() const { return baseHealthRegen; }
    float getMyopiaFactor() const { return myopiaFactor; }
    float getAimingPenalty() const { return aimingPenalty; }
    int getFertilityFactor() const { return fertilityFactor; }
    float getAgingRate() const { return agingRate; }


    static SDL_Color generateRandomColor();

private:
    void calculateDerivedStats(); // Utilisée dans le constructeur

    // --- Constantes de Régénération (Utilisées dans update()) ---
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000;
    static constexpr Uint32 STAMINA_REGEN_DELAY_MS = 2000;
    static constexpr int STAMINA_REGEN_RATE = 2;
    static constexpr int STAMINA_FLEE_COST_PER_FRAME = 1;


    // --- MEMBRES EXISTANTS (Stockage des états) ---
    std::string name;
    int x,y;
    int health;
    int speed;
    int direction[2]{};
    int sightRadius;
    int stamina;
    int maxHealth;
    int maxStamina;
    bool isAlive = true;
    SDL_Color color;

    // --- GÈNES ET PARAMÈTRES (11 membres) ---
    int rad;
    bool isRanged;
    int weaponGene;
    float kite_ratio_gene;
    int generation;
    std::string parent1_name;
    std::string parent2_name;

    // --- NOUVEAUX GÈNES (7 membres) ---
    float damageFragility;
    float staminaEfficiency;
    float baseHealthRegen;
    float myopiaFactor;
    float aimingPenalty;
    int fertilityFactor;
    float agingRate;

    // --- STATS DÉRIVÉES (Arme/Défense) ---
    int damage;
    int attackRange;
    Uint32 attackCooldown;
    int projectileSpeed;
    int projectileRadius;
    int staminaAttackCost;
    float armor = 0.0f;
    int regenAmount = 0; // Utilisé dans le CPP

    // --- Variables de Déplacement/Temps ---
    int targetX = -1;
    int targetY = -1;
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;
    Uint32 lastRegenTick = 0;
    Uint32 lastStaminaUseTick = 0;
    bool isFleeing = false;
};


#endif //EVOARENA_ENTITY_H