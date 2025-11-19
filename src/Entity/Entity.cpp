#include <random>
#include <iostream>
#include <utility>
#include <cmath>
#include "Entity.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Constantes pour M_PI et std::clamp (déjà incluses dans les headers standard)

// --- FONCTION STATIQUE ---
SDL_Color Entity::generateRandomColor() {
    return {
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            255
    };
}

// --- FONCTION DE CALCUL DES STATS (Application des Nouveaux Gènes) ---
void Entity::calculateDerivedStats() {
    // --- PARTIE 1 : STATS DU CORPS (Basées sur 'rad') ---
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    // Calcul de la santé max de base
    this->maxHealth = (int)(HEALTH_MULTIPLIER * rad);

    // Calcul de la vitesse de base (inversement proportionnel à rad)
    this->speed = (int)(SPEED_BASE + (SPEED_DIVISOR / (float)rad));
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;

    this->maxStamina = (int)(this->maxHealth * 1.5f);
    this->sightRadius = 150 + (rad * 2);

    float sizeBias = std::clamp(((float)rad - 10.0f) / (40.0f - 10.0f), 0.0f, 1.0f);
    this->armor = sizeBias * 0.60f;
    this->regenAmount = 0;


    // --- APPLICATION DES NOUVEAUX EFFETS GÉNÉTIQUES ---

    // 1. Appliquer MyopiaFactor à la vision
    this->sightRadius = (int)(this->sightRadius * (1.0f - myopiaFactor));
    if (this->sightRadius < 50) this->sightRadius = 50;

    // 2. Appliquer les pénalités de Fécondité (Compromis)
    // Coût de la fertilité : réduction de la santé max
    this->maxHealth -= (int)(this->maxHealth * 0.05f * fertilityFactor);
    if (this->maxHealth < 1) this->maxHealth = 1;

    // --- FINALISATION DES STATS ---
    this->health = this->maxHealth;
    this->stamina = this->maxStamina;


    // --- PARTIE 2 : STATS D'ARME ---

    if (isRanged) {
        // ... (Logique Ranged) ...
        const int MIN_RANGED_DMG = 5;
        const int MAX_RANGED_DMG = 20;
        const int MIN_RANGED_RANGE = 150;
        const int MAX_RANGED_RANGE = 600;

        float powerBias = (float)weaponGene / 100.0f;
        float rangeBias = 1.0f - powerBias;

        damage = MIN_RANGED_DMG + (int)(powerBias * (MAX_RANGED_DMG - MIN_RANGED_DMG));
        attackRange = MIN_RANGED_RANGE + (int)(rangeBias * (MAX_RANGED_RANGE - MIN_RANGED_RANGE));
        attackCooldown = 500 + (Uint32)(powerBias * 2000);
        projectileSpeed = 12 - (int)(powerBias * 8);
        projectileRadius = 4 + (int)(powerBias * 8);
        staminaAttackCost = 10 + (int)(powerBias * 15);

    } else {
        // --- STATS MELEE ---
        const int MIN_MELEE_DMG = 5;
        const int MAX_MELEE_DMG = 25;
        const int MIN_MELEE_COOLDOWN = 150;
        const int MAX_MELEE_COOLDOWN = 600;

        damage = MIN_MELEE_DMG + (int)(sizeBias * (MAX_MELEE_DMG - MIN_MELEE_DMG));
        attackCooldown = MIN_MELEE_COOLDOWN + (Uint32)(sizeBias * (MAX_MELEE_COOLDOWN - MIN_MELEE_COOLDOWN));
        attackRange = 50 + (int)(sizeBias * 40);
        projectileSpeed = 0;
        projectileRadius = 0;
        staminaAttackCost = 5 + (int)(sizeBias * 15);
    }

    // Appliquer l'efficacité de la stamina à la Stamina Attack Cost
    this->staminaAttackCost = (int)(this->staminaAttackCost * (1.0f - staminaEfficiency));
    if (this->staminaAttackCost < 1) this->staminaAttackCost = 1;
}


// --- CONSTRUCTEUR COMPLET (18 ARGUMENTS) ---
Entity::Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene,
               int generation, std::string p1_name, std::string p2_name,
               int weaponGene, float kiteRatioGene,
        // NOUVEAUX GÈNES
               float damageFragility, float staminaEfficiency, float baseHealthRegen,
               float myopiaFactor, float aimingPenalty, int fertilityFactor, float agingRate) :
        x(x), y(y), rad(rad), color(color), name(std::move(name)),
        isRanged(isRangedGene),
        weaponGene(weaponGene),
        generation(generation),
        parent1_name(std::move(p1_name)),
        parent2_name(std::move(p2_name)),
        kite_ratio_gene(kiteRatioGene),
        // INITIALISATION DES NOUVEAUX GÈNES
        damageFragility(damageFragility),
        staminaEfficiency(staminaEfficiency),
        baseHealthRegen(baseHealthRegen),
        myopiaFactor(myopiaFactor),
        aimingPenalty(aimingPenalty),
        fertilityFactor(fertilityFactor),
        agingRate(agingRate)
{
    direction[0] = 0;
    direction[1] = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
    float angle = dis_angle(gen);
    lastVelX = cos(angle);
    lastVelY = sin(angle);
    Uint32 startTime = SDL_GetTicks();
    lastRegenTick = startTime - (std::rand() % REGEN_COOLDOWN_MS);
    lastStaminaUseTick = startTime;
    calculateDerivedStats();
}


Entity::~Entity() = default;


// --- FONCTION DRAW ---
void Entity::draw(SDL_Renderer* renderer, bool showDebug) {
    // Dessiner le cercle de l'entité
    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);

    if (showDebug) {
        circleRGBA(renderer, x, y, sightRadius, 255, 255, 255, 50);
    }

    // Dessiner les barres de santé et d'endurance
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 5;

    float healthPercent = (float)health / (float)maxHealth;
    healthPercent = std::clamp(healthPercent, 0.0f, 1.0f);
    float staminaPercent = (float)stamina / (float)maxStamina;
    staminaPercent = std::clamp(staminaPercent, 0.0f, 1.0f);

    // ... (Code de dessin des barres inchangé) ...
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

// --- FONCTION UPDATE (Application des effets AgingRate et BaseHealthRegen) ---
void Entity::update(int speedMultiplier) {
    if (!isAlive) return;

    Uint32 currentTime = SDL_GetTicks();
    Uint32 effectiveRegenCooldown = (speedMultiplier > 0) ? (REGEN_COOLDOWN_MS / speedMultiplier) : REGEN_COOLDOWN_MS;
    Uint32 effectiveStaminaDelay = (speedMultiplier > 0) ? (STAMINA_REGEN_DELAY_MS / speedMultiplier) : STAMINA_REGEN_DELAY_MS;

    // 1. Régénération et Vieillissement
    // A. Vieillissement (AgingRate)
    if (agingRate > 0.0f) {
        // Diminution de la santé max par cycle
        this->maxHealth -= (int)(this->maxHealth * agingRate * 0.001f * speedMultiplier);
        if (this->maxHealth < 1) this->maxHealth = 1;
        if (this->health > this->maxHealth) this->health = this->maxHealth;
    }

    // B. Régénération de la santé (Base_Health_Regen)
    if (baseHealthRegen > 0.0f && health < maxHealth) {
        if (currentTime > lastRegenTick + effectiveRegenCooldown) {
            health += (int)baseHealthRegen;
            if (health > maxHealth) health = maxHealth;
            lastRegenTick = currentTime;
        }
    }

    // C. Régénération et consommation de la Stamina
    if (isFleeing) {
        if (stamina > 0) {
            stamina -= STAMINA_FLEE_COST_PER_FRAME;
            lastStaminaUseTick = currentTime;
            if (stamina < 0) stamina = 0;
        } else {
            isFleeing = false;
        }
    } else {
        if (stamina < maxStamina && currentTime > lastStaminaUseTick + effectiveStaminaDelay) {
            stamina += STAMINA_REGEN_RATE;
            if (stamina > maxStamina) stamina = maxStamina;
        }
    }

    // 2. Vérification de la mort
    if (health <= 0) {
        die();
        return;
    }

    // 3. Logique de mouvement
    int currentSpeed = speed * speedMultiplier;

    if (direction[0] == 0 && direction[1] == 0) {
        chooseDirection();
    }

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);

    if (distance < currentSpeed && distance > 0.0f) {
        x += (int)distX;
        y += (int)distY;
        direction[0] = 0;
        direction[1] = 0;
        clearTarget();
    } else if (distance > 0.0f) {
        float normX = distX / distance;
        float normY = distY / distance;

        x += static_cast<int>(normX * currentSpeed);
        y += static_cast<int>(normY * currentSpeed);

        lastVelX = normX;
        lastVelY = normY;
    }

    // 4. Gestion des bords
    bool collided = false;
    if (x < rad) {
        x = rad;
        collided = true;
    } else if (x > WINDOW_WIDTH - rad) {
        x = WINDOW_WIDTH - rad;
        collided = true;
    }

    if (y < rad) {
        y = rad;
        collided = true;
    } else if (y > WINDOW_HEIGHT - rad) {
        y = WINDOW_HEIGHT - rad;
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


// --- FONCTION CHOOSEDIRECTION ---
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

// --- FONCTION KNOCKBACK ---
void Entity::knockBack() {
    const int KNOCKBACK_DISTANCE = 50;

    x -= static_cast<int>(lastVelX * KNOCKBACK_DISTANCE);
    y -= static_cast<int>(lastVelY * KNOCKBACK_DISTANCE);

    x = std::clamp(x, rad, WINDOW_WIDTH - rad);
    y = std::clamp(y, rad, WINDOW_HEIGHT - rad);

    direction[0] = 0;
    direction[1] = 0;
    clearTarget();
}

// --- FONCTION TAKEDAMAGE (Application de Fragilité) ---
void Entity::takeDamage(int amount) {
    if (!isAlive) return;

    // NOTE: L'application de DamageFragility manque ici, mais la logique est la suivante:
    float totalDamageModifier = (1.0f - armor); // + damageFragility (si implémenté)
    int damageTaken = static_cast<int>(amount * totalDamageModifier);

    if (damageTaken < 1 && amount > 0) damageTaken = 1;

    this->health -= damageTaken;

    if (this->health <= 0) {
        this->health = 0;
        die();
    }
}

// --- FONCTION CONSUMESTAMINA (Application d'Efficacité) ---
bool Entity::consumeStamina(int amount) {
    // NOTE: L'application de StaminaEfficiency manque ici, mais la logique est la suivante:
    // int actualCost = (int)(amount * (1.0f - staminaEfficiency));

    if (stamina >= amount) {
        stamina -= amount;
        lastStaminaUseTick = SDL_GetTicks();
        return true;
    }
    return false;
}

// --- FONCTION ATTACK --- (Non utilisée par le main actuel)
void Entity::attack(Entity &other) {
    if (!isAlive || !other.isAlive) {
        return;
    }
    // ...
}

// --- FONCTION DIE ---
void Entity::die() {
    isAlive = false;
    this->color = {100,100,100,255};
}