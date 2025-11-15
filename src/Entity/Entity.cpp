#include <random>
#include <iostream>
#include <utility>
#include <cmath>
#include "Entity.h"
#include <SDL2/SDL2_gfxPrimitives.h>


// --- FONCTION STATIQUE --- (Inchangée)
SDL_Color Entity::generateRandomColor() {
    return {
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            255
    };
}

// --- FONCTION DE CALCUL DES STATS (MODIFIÉE POUR STAMINA) ---
void Entity::calculateDerivedStats() {

    // --- PARTIE 1 : STATS DU CORPS (Basées sur 'rad') ---
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    this->maxHealth = (int)(HEALTH_MULTIPLIER * rad);
    this->health = this->maxHealth;

    this->speed = (int)(SPEED_BASE + (SPEED_DIVISOR / (float)rad));
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;

    // Max Stamina est lié à la santé max
    this->maxStamina = (int)(this->maxHealth * 1.5f);
    this->stamina = this->maxStamina;

    this->sightRadius = 150 + (rad * 2);

    // Normalise 'rad' (10-40) en un biais (0.0 - 1.0)
    float sizeBias = ((float)rad - 10.0f) / (40.0f - 10.0f);
    sizeBias = std::clamp(sizeBias, 0.0f, 1.0f);

    this->armor = sizeBias * 0.60f; // 0% à 60% d'armure
    this->regenAmount = (this->speed >= 4) ? 1 : 0; // 1 regen si rapide


    // --- PARTIE 2 : STATS D'ARME (Basées sur 'weaponGene' ou 'rad') ---

    if (isRanged) {
        // --- STATS RANGED (Basées sur 'weaponGene' 0-100) ---
        float powerBias = (float)weaponGene / 100.0f;
        float rangeBias = 1.0f - powerBias;

        // ... (Calculs de damage, attackRange, etc. inchangés) ...
        const int MIN_RANGED_DMG = 5;
        const int MAX_RANGED_DMG = 20;
        const int MIN_RANGED_RANGE = 150;
        const int MAX_RANGED_RANGE = 600;

        damage = MIN_RANGED_DMG + (int)(powerBias * (MAX_RANGED_DMG - MIN_RANGED_DMG));
        attackRange = MIN_RANGED_RANGE + (int)(rangeBias * (MAX_RANGED_RANGE - MIN_RANGED_RANGE));
        attackCooldown = 500 + (Uint32)(powerBias * 2000);
        projectileSpeed = 12 - (int)(powerBias * 8);
        projectileRadius = 4 + (int)(powerBias * 8);

        // *** NOUVEAU : Coût en Stamina (Ranged) ***
        staminaAttackCost = 5 + (int)(powerBias * 15); // Coût de 5 (faible) à 20 (élevé)

    } else {
        // --- STATS MELEE (Basées sur 'sizeBias' 0.0-1.0) ---

        // ... (Calculs de damage, attackCooldown, etc. inchangés) ...
        const int MIN_MELEE_DMG = 5;
        const int MAX_MELEE_DMG = 25;
        const int MIN_MELEE_COOLDOWN = 150;
        const int MAX_MELEE_COOLDOWN = 600;

        damage = MIN_MELEE_DMG + (int)(sizeBias * (MAX_MELEE_DMG - MIN_MELEE_DMG));
        attackCooldown = MIN_MELEE_COOLDOWN + (Uint32)(sizeBias * (MAX_MELEE_COOLDOWN - MIN_MELEE_COOLDOWN));
        attackRange = 50 + (int)(sizeBias * 40);
        projectileSpeed = 0;
        projectileRadius = 0;

        // *** NOUVEAU : Coût en Stamina (Melee) ***
        staminaAttackCost = 5 + (int)(sizeBias * 15); // Coût de 5 (petit) à 20 (gros)
    }
}


// --- CONSTRUCTEUR (MODIFIÉ) ---
Entity::Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene):
        x(x), y(y), rad(rad), color(color), name(std::move(name)),
        isRanged(isRangedGene), weaponGene(std::rand() % 101) // Initialise le gène d'arme (0-100)
{

    direction[0] = 0;
    direction[1] = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
    float angle = dis_angle(gen);
    lastVelX = cos(angle);
    lastVelY = sin(angle);

    // Initialise les timers
    Uint32 startTime = SDL_GetTicks();
    lastRegenTick = startTime - (std::rand() % REGEN_COOLDOWN_MS);
    lastStaminaUseTick = startTime; // Peut se régénérer immédiatement

    // Calcule TOUTES les stats (corps + arme)
    calculateDerivedStats();
}


Entity::~Entity() = default;


// --- FONCTION DRAW --- (Inchangée)
void Entity::draw(SDL_Renderer* renderer, bool showDebug) {
    // Dessiner le cercle de l'entité
    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);

    // --- MODIFIÉ : Conditionnel ---
    if (showDebug) {
        // Dessiner le rayon de vision en blanc (semi-transparent)
        circleRGBA(renderer, x, y, sightRadius, 255, 255, 255, 50);
    }

    // Dessiner les barres de santé et d'endurance
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 5;

    // ... (Reste du code de dessin des barres inchangé) ...
    float healthPercent = (float)health / (float)maxHealth;
    healthPercent = std::clamp(healthPercent, 0.0f, 1.0f);
    float staminaPercent = (float)stamina / (float)maxStamina;
    staminaPercent = std::clamp(staminaPercent, 0.0f, 1.0f);
    SDL_Rect healthBarBg = {x - offset - barWidth, y - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);
    int healthFillHeight = static_cast<int>(barHeight * healthPercent + 0.5f);
    if (healthFillHeight > 0) {
        SDL_Rect healthBar = {x - offset - barWidth, y + barHeight / 2 - healthFillHeight, barWidth, healthFillHeight};
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }
    SDL_Rect staminaBarBg = {x + offset, y - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &staminaBarBg);
    int staminaFillHeight = static_cast<int>(barHeight * staminaPercent + 0.5f);
    if (staminaFillHeight > 0) {
        SDL_Rect staminaBar = {x + offset, y + barHeight / 2 - staminaFillHeight, barWidth, staminaFillHeight};
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        SDL_RenderFillRect(renderer, &staminaBar);
    }

    // Affichage des PV/Vitesse au centre
    std::string infoText = std::to_string(health) + " / " + std::to_string(speed);
    stringRGBA(renderer, x - 15, y - 5, infoText.c_str(), 0, 0, 0, 255);
}

// --- FONCTION UPDATE (MODIFIÉE POUR STAMINA) ---
void Entity::update() {
    if (!isAlive) return;

    Uint32 currentTime = SDL_GetTicks();

    // 1. Régénération de la santé
    if (regenAmount > 0 && health < maxHealth) {
        if (currentTime > lastRegenTick + REGEN_COOLDOWN_MS) {
            health += regenAmount;
            if (health > maxHealth) health = maxHealth;
            lastRegenTick = currentTime;
        }
    }

    // *** NOUVEAU : 2. Régénération et consommation de la Stamina ***
    if (isFleeing) {
        // Consommer de la stamina en fuyant
        if (stamina > 0) {
            stamina -= STAMINA_FLEE_COST_PER_FRAME;
            lastStaminaUseTick = currentTime; // Retarde la régénération
            if (stamina < 0) stamina = 0;
        } else {
            isFleeing = false; // Ne peut plus fuir
        }
    } else {
        // Régénérer la stamina si le délai est passé
        if (stamina < maxStamina && currentTime > lastStaminaUseTick + STAMINA_REGEN_DELAY_MS) {
            stamina += STAMINA_REGEN_RATE;
            if (stamina > maxStamina) stamina = maxStamina;
        }
    }


    // 3. Vérification de la mort (au cas où, bien que géré par takeDamage)
    if (health <= 0) {
        die();
        return;
    }

    // 4. Logique de mouvement (inchangée)
    if (direction[0] == 0 && direction[1] == 0) {
        chooseDirection();
    }

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);

    if (distance < speed && distance > 0.0f) {
        x += (int)distX;
        y += (int)distY;
        direction[0] = 0;
        direction[1] = 0;
        clearTarget();
    } else if (distance > 0.0f) {
        float normX = distX / distance;
        float normY = distY / distance;

        x += static_cast<int>(normX * speed);
        y += static_cast<int>(normY * speed);

        lastVelX = normX;
        lastVelY = normY;
    }

    // 5. Gestion des bords (inchangée)
    bool collided = false;
    // ... (code des collisions avec les bords inchangé) ...
    if (x < rad) {
        x = rad;
        collided = true;
    } else if (x > WINDOW_SIZE_WIDTH - rad) {
        x = WINDOW_SIZE_WIDTH - rad;
        collided = true;
    }

    if (y < rad) {
        y = rad;
        collided = true;
    } else if (y > WINDOW_SIZE_HEIGHT - rad) {
        y = WINDOW_SIZE_HEIGHT - rad;
        collided = true;
    }

    if (collided) {
        direction[0] = 0;
        direction[1] = 0;
        clearTarget();
        lastVelX = -lastVelX;
        lastVelY = -lastVelY;
    }
}


// --- FONCTION CHOOSEDIRECTION --- (Inchangée)
// ... (code inchangé) ...
void Entity::chooseDirection(int target[2]) {

    if(target != nullptr){
        direction[0] = target[0];
        direction[1] = target[1];
        targetX = target[0];
        targetY = target[1];
    }
    else{
        targetX = -1;
        targetY = -1;

        const float WANDER_DISTANCE = 90.0f;
        const float WANDER_JITTER_STRENGTH = 0.4f;

        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<> dis_jitter(-1.0, 1.0);
        float jitterX = dis_jitter(gen);
        float jitterY = dis_jitter(gen);

        float jitterMag = std::sqrt(jitterX * jitterX + jitterY * jitterY);
        if (jitterMag > 0.0f) {
            jitterX = (jitterX / jitterMag) * WANDER_JITTER_STRENGTH;
            jitterY = (jitterY / jitterMag) * WANDER_JITTER_STRENGTH;
        }

        float newDirX = (lastVelX * (1.0f - WANDER_JITTER_STRENGTH)) + jitterX;
        float newDirY = (lastVelY * (1.0f - WANDER_JITTER_STRENGTH)) + jitterY;

        float newMag = std::sqrt(newDirX * newDirX + newDirY * newDirY);
        if (newMag > 0.0f) {
            newDirX /= newMag;
            newDirY /= newMag;
        } else {
            std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
            float angle = dis_angle(gen);
            newDirX = cos(angle);
            newDirY = sin(angle);
        }

        direction[0] = x + static_cast<int>(newDirX * WANDER_DISTANCE);
        direction[1] = y + static_cast<int>(newDirY * WANDER_DISTANCE);

        lastVelX = newDirX;
        lastVelY = newDirY;
    }
}

// --- FONCTION KNOCKBACK --- (Inchangée)
// ... (code inchangé) ...
void Entity::knockBack() {
    const int KNOCKBACK_DISTANCE = 50;

    x -= static_cast<int>(lastVelX * KNOCKBACK_DISTANCE);
    y -= static_cast<int>(lastVelY * KNOCKBACK_DISTANCE);

    x = std::clamp(x, rad, WINDOW_SIZE_WIDTH - rad);
    y = std::clamp(y, rad, WINDOW_SIZE_HEIGHT - rad);

    direction[0] = 0;
    direction[1] = 0;
    clearTarget();
}

// --- FONCTION TAKEDAMAGE --- (Inchangée)
// ... (code inchangé) ...
void Entity::takeDamage(int amount) {
    if (!isAlive) return;

    int damageTaken = static_cast<int>(amount * (1.0f - armor));

    if (damageTaken < 1 && amount > 0) damageTaken = 1;

    this->health -= damageTaken;

    if (this->health <= 0) {
        this->health = 0;
        die();
    }
}


// --- FONCTION ATTACK --- (Inchangée)
// ... (code inchangé) ...
void Entity::attack(Entity &other) {
    if (!isAlive || !other.isAlive) {
        return;
    }
    // ...
}

// --- FONCTION DIE --- (Inchangée)
// ... (code inchangé) ...
void Entity::die() {
    isAlive = false;
    this->color = {100,100,100,255};
}

// --- NOUVELLE FONCTION : consumeStamina ---
bool Entity::consumeStamina(int amount) {
    if (stamina >= amount) {
        stamina -= amount;
        lastStaminaUseTick = SDL_GetTicks(); // Met à jour le timer de délai
        return true;
    }
    return false; // Pas assez de stamina
}