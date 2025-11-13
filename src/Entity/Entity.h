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
    void draw(SDL_Renderer* renderer);
    void chooseDirection(int target[2] = nullptr);
    void knockBack();
    void clearTarget() { targetX = -1; targetY = -1; }


    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void attack(Entity &other); // Note: Cette fonction est toujours inutilisée
    void die();

    // *** NOUVEAU : Fonction centralisée pour prendre des dégâts ***
    void takeDamage(int amount);


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
    int getMaxStamina() { return maxStamina; }
    bool getIsAlive() const { return isAlive; }

    // GETTERS : Stats de combat fixes
    [[nodiscard]] bool getIsRanged() const { return isRanged; }
    [[nodiscard]] int getRangedDamage() const { return RANGED_DAMAGE; }
    [[nodiscard]] int getRangedRange() const { return RANGED_RANGE; }
    [[nodiscard]] Uint32 getRangedCooldownMS() const { return RANGED_COOLDOWN_MS; }

    [[nodiscard]] int getMeleeDamage() const { return MELEE_DAMAGE; }
    [[nodiscard]] int getMeleeRange() const { return MELEE_RANGE; }
    [[nodiscard]] Uint32 getMeleeCooldownMS() const { return MELEE_COOLDOWN_MS; }


    static SDL_Color generateRandomColor();

private:
    void calculateDerivedStats();

    // --- CORRECTION CLÉ : Les constantes sont STATIC CONSTEXPR ---
    // (Partagées par toutes les instances pour éviter les erreurs de copie/affectation)

    // Style Distance (Ranged)
    static constexpr int RANGED_DAMAGE = 10;
    static constexpr int RANGED_RANGE = 400;
    static constexpr Uint32 RANGED_COOLDOWN_MS = 1500;

    // Style Corps à Corps (Melee)
    static constexpr int MELEE_DAMAGE = 10;
    static constexpr int MELEE_RANGE = 70;
    static constexpr Uint32 MELEE_COOLDOWN_MS = 200;

    // *** NOUVEAU : Constante pour la régénération ***
    static constexpr Uint32 REGEN_COOLDOWN_MS = 2000; // 2 secondes


    std::string name;
    int x,y;
    int rad; // Gène principal
    int health;
    int speed;
    int direction[2]{};
    int sightRadius;
    int stamina;
    int maxHealth;
    int maxStamina;
    bool isAlive = true;
    SDL_Color color;

    // Gène de combat
    bool isRanged;

    // DEBUG VISUEL
    int targetX = -1;
    int targetY = -1;

    // Mémorise la direction pour l'errance naturelle
    float lastVelX = 1.0f;
    float lastVelY = 0.0f;

    // *** NOUVEAU : Stats dérivées (Étape 2) ***
    float armor = 0.0f; // Réduction des dégâts (0.0 = 0%, 1.0 = 100%)
    int regenAmount = 0; // PV soignés par tick
    Uint32 lastRegenTick = 0; // Timer pour la régénération
};


#endif //EVOARENA_ENTITY_H