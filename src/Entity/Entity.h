#ifndef EVOARENA_ENTITY_H
#define EVOARENA_ENTITY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "../constants.h" // WINDOW_SIZE_WIDTH et WINDOW_SIZE_HEIGHT
#include <algorithm>
#include <string>
#include <cstdlib>
#include <map>

// Définition simple pour le projectile (juste une forward declaration)
class Projectile;

class Entity {
public:
    // CONSTRUCTEUR MIS À JOUR : Prend uniquement les gènes (rad) et les attributs de base
    Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene);
    ~Entity();

    void update();
    void draw(SDL_Renderer* renderer, bool showDebug = false);
    void chooseDirection(int target[2] = nullptr);
    void knockBack();
    void clearTarget() { targetX = -1; targetY = -1; }


    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void attack(Entity &other); // Note: Cette fonction est toujours inutilisée
    void die();

    void takeDamage(int amount);

    // --- NOUVEAU : Gestion de la Stamina ---
    bool consumeStamina(int amount);
    void setIsFleeing(bool fleeing) { isFleeing = fleeing; }


    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; }
    int getSightRadius() const { return sightRadius; }
    [[nodiscard]] std::string getName() const { return name; }
    SDL_Color getColor() const { return color; }

    int getHealth() const { return health; }
    void setHealth(int h) { health = h; } // Toujours utile pour le debug ou des soins
    int getStamina() const { return stamina; }
    void setStamina(int s) { stamina = s; }
    int getMaxHealth() const { return maxHealth; }
    int getMaxStamina() const { return maxStamina; }
    bool getIsAlive() const { return isAlive; }
    int getSpeed() const { return speed; }
    float getArmor() const { return armor; }
    int getRegenAmount() const { return regenAmount; }
    int getWeaponGene() const { return weaponGene; }

    // --- MODIFIÉ : GETTERS POUR LES GÈNES D'ARMES ---
    [[is_ranged]] [[nodiscard]] bool getIsRanged() const { return isRanged; }
    [[nodiscard]] int getDamage() const { return damage; }
    [[nodiscard]] int getAttackRange() const { return attackRange; }
    [[nodiscard]] Uint32 getAttackCooldown() const { return attackCooldown; }
    [[nodiscard]] int getProjectileSpeed() const { return projectileSpeed; }
    [[nodiscard]] int getProjectileRadius() const { return projectileRadius; }
    [[nodiscard]] int getStaminaAttackCost() const { return staminaAttackCost; } // *** NOUVEAU ***


    static SDL_Color generateRandomColor();

private:
    void calculateDerivedStats();

    // Constantes de régénération
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000; // 2 secondes

    // --- NOUVEAU : Constantes de Stamina ---
    static constexpr Uint32 STAMINA_REGEN_DELAY_MS = 2000; // Délai avant de regagner
    static constexpr int STAMINA_REGEN_RATE = 2; // Points de stamina par tick (dans update)
    static constexpr int STAMINA_FLEE_COST_PER_FRAME = 1; // Coût de la fuite par frame


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

    // Mémorise la direction pour l'errance naturelle
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;

    // Stats de régénération
    int regenAmount = 0;
    Uint32 lastRegenTick = 0;

    // --- NOUVEAU : GÈNES (Étape 2 & 3) ---
    int rad; // Gène principal (corps)
    bool isRanged; // Gène de type de combat
    int weaponGene; // Gène principal (arme, 0-100)
    float armor = 0.0f;

    // --- NOUVEAU : STATS DÉRIVÉES DES GÈNES (Arme) ---
    int damage;
    int attackRange;
    Uint32 attackCooldown;
    int projectileSpeed;
    int projectileRadius;
    int staminaAttackCost; // *** NOUVEAU ***

    // --- NOUVEAU : États de Stamina ---
    Uint32 lastStaminaUseTick = 0;
    bool isFleeing = false;
};


#endif //EVOARENA_ENTITY_H