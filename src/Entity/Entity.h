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
    // --- CONSTRUCTEUR MODIFIÉ ---
    Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene,
           int generation, std::string parent1_name, std::string parent2_name);
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


    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; }
    int getSightRadius() const { return sightRadius; }
    [[nodiscard]] std::string getName() const { return name; }
    SDL_Color getColor() const { return color; }

    int getHealth() const { return health; }
    void setHealth(int h) { health = h; }
    int getStamina() const { return stamina; }
    void setStamina(int s) { stamina = s; }
    int getMaxHealth() const { return maxHealth; }
    int getMaxStamina() const { return maxStamina; }
    bool getIsAlive() const { return isAlive; }
    int getSpeed() const { return speed; }
    float getArmor() const { return armor; }
    int getRegenAmount() const { return regenAmount; }
    int getWeaponGene() const { return weaponGene; }

    //Générations getters
    int getGeneration() const { return generation; }
    std::string getParent1Name() const { return parent1_name; }
    std::string getParent2Name() const { return parent2_name; }

    // --- GETTERS D'ARMES ---
    [[nodiscard]] bool getIsRanged() const { return isRanged; } // Corrigé (c'était [[is_ranged]])
    [[nodiscard]] int getDamage() const { return damage; }
    [[nodiscard]] int getAttackRange() const { return attackRange; }
    [[nodiscard]] Uint32 getAttackCooldown() const { return attackCooldown; }
    [[nodiscard]] int getProjectileSpeed() const { return projectileSpeed; }
    [[nodiscard]] int getProjectileRadius() const { return projectileRadius; }
    [[nodiscard]] int getStaminaAttackCost() const { return staminaAttackCost; }



    static SDL_Color generateRandomColor();

private:
    void calculateDerivedStats();

    // Constantes de régénération
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000;
    static constexpr Uint32 STAMINA_REGEN_DELAY_MS = 2000;
    static constexpr int STAMINA_REGEN_RATE = 2;
    static constexpr int STAMINA_FLEE_COST_PER_FRAME = 1;

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

    // DEBUG VISUEL
    int targetX = -1;
    int targetY = -1;

    // Mémorise la direction
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;

    // Stats de régénération
    int regenAmount = 0;
    Uint32 lastRegenTick = 0;

    // --- GÈNES ---
    int rad;
    bool isRanged;
    int weaponGene;

    // --- STATS DÉRIVÉES (Arme) ---
    int damage;
    int attackRange;
    Uint32 attackCooldown;
    int projectileSpeed;
    int projectileRadius;
    int staminaAttackCost;
    float armor = 0.0f;

    // --- États de Stamina ---
    Uint32 lastStaminaUseTick = 0;
    bool isFleeing = false;

    // --- NOUVEAU : Données de Généalogie ---
    int generation;
    std::string parent1_name;
    std::string parent2_name;
};


#endif //EVOARENA_ENTITY_H