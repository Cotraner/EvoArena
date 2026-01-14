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
#include <cmath>
#include <iostream>

class Projectile;

// Represents an entity in the game, including its stats, behavior, and rendering
class Entity {
public:
    // Possible states for the entity
    enum State {
        WANDER,
        COMBAT,
        FLEE,
        FORAGE
    };

    // Constructor and destructor
    Entity(std::string name, int x, int y, SDL_Color color,
           const float geneticCode[12], int generation,
           std::string parent1_name, std::string parent2_name);
    ~Entity();

    // Updates the entity's state
    void update(int speedMultiplier);

    // Renders the entity on the screen
    void draw(SDL_Renderer* renderer, const Camera& cam, bool showDebug = false);

    // Chooses a new direction for movement
    void chooseDirection(int target[2] = nullptr);

    // Applies a knockback effect
    void knockBackFrom(int sourceX, int sourceY, int force);

    // Determines the entity type (Melee, Ranged, or Healer)
    int getEntityType() const;

    // Heals the entity
    void receiveHealing(int amount);

    // Checks if another entity is an ally
    bool isAlliedWith(const Entity& other) const;

    // Clears the current movement target
    void clearTarget();

    // Setters for position and state
    void setX(int newX);
    void setY(int newY);
    void die();
    void takeDamage(int amount);
    bool consumeStamina(int amount);
    void setIsFleeing(bool fleeing);
    void setIsCharging(bool charging);
    void setCurrentState(State s);

    // Getters for basic properties
    [[nodiscard]] std::string getName() const;
    SDL_Color getColor() const;
    int getX() const;
    int getY() const;
    int getRad() const;
    int getSightRadius() const;
    State getCurrentState() const;
    std::string getCurrentStateString() const;

    // Getters for genetic code
    const float* getGeneticCode() const;
    float getKiteRatio() const;
    float getWeaponGene() const;
    bool getIsRanged() const;
    int getGeneration() const;
    std::string getParent1Name() const;
    std::string getParent2Name() const;
    int getCurrentTraitID() const;

    // Getters for derived stats
    int getHealth() const;
    void setHealth(int h);
    int getMaxHealth() const;
    int getStamina() const;
    int getMaxStamina() const;
    bool getIsAlive() const;
    int getSpeed() const;
    float getArmor() const;
    int getDamage() const;
    int getAttackRange() const;
    Uint32 getAttackCooldown() const;
    int getProjectileSpeed() const;
    int getProjectileRadius() const;
    int getStaminaAttackCost() const;

    // Getters for biological traits
    float getDamageFragility() const;
    float getStaminaEfficiency() const;
    float getBaseHealthRegen() const;
    float getMyopiaFactor() const;
    float getAimingPenalty() const;
    int getFertilityFactor() const;
    float getAgingRate() const;
    float getBravery() const;
    float getGreed() const;

    // Generates a random color
    static SDL_Color generateRandomColor();

    // Restores stamina and triggers a flash effect
    void restoreStamina(int amount, int speedMultiplier);

private:
    // Calculates derived stats based on genetic code and traits
    void calculateDerivedStats();

    // Constants for stamina and health regeneration
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000;
    static constexpr Uint32 STAMINA_REGEN_DELAY_MS = 3000;
    static constexpr int STAMINA_REGEN_RATE = 1;
    static constexpr int STAMINA_FLEE_COST_PER_FRAME = 4;
    static constexpr int STAMINA_CHARGE_COST_PER_FRAME = 3;

    // Entity properties
    std::string name;
    int x, y;
    bool isAlive = true;
    SDL_Color color;
    int rad;
    float geneticCode[14];

    // State and stats
    State currentState = WANDER;
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
    int regenAmount = 0;

    // Movement and behavior
    int direction[2]{};
    int targetX = -1;
    int targetY = -1;
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;
    Uint32 lastRegenTick = 0;
    Uint32 lastStaminaUseTick = 0;
    bool isFleeing = false;
    bool isCharging = false;

    // Metadata
    int generation;
    std::string parent1_name;
    std::string parent2_name;
};

#endif //EVOARENA_ENTITY_H
