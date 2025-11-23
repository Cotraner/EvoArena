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

    // --- PARTIE 0 : Récupération des gènes ---
    const int rad = (int)geneticCode[0];
    const bool isRanged = geneticCode[10] > 0.5f;
    const int weaponGene = (int)geneticCode[1];
    const float staminaEfficiency = geneticCode[4];
    const float myopiaFactor = geneticCode[6];
    const int fertilityFactor = (int)geneticCode[9];

    // NOUVEAU : Récupération de l'ID du Trait Dominant
    const int traitID = (int)std::round(geneticCode[11]);


    // --- PARTIE 1 : STATS DU CORPS (Basées sur 'rad' et valeurs de base) ---
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    // Calculs initiaux basés sur 'rad'
    this->maxHealth = (int)(HEALTH_MULTIPLIER * rad);
    this->health = this->maxHealth;

    // Calcul initial de la vitesse
    this->speed = (int)(SPEED_BASE + (SPEED_DIVISOR / (float)rad));
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;

    // Calcul initial de la stamina Max (avant modificateurs de trait)
    this->maxStamina = (int)(this->maxHealth * 1.5f);
    this->stamina = this->maxStamina;

    this->sightRadius = 150 + (rad * 2);

    float sizeBias = std::clamp(((float)rad - 10.0f) / (40.0f - 10.0f), 0.0f, 1.0f);
    this->armor = sizeBias * 0.60f; // Armor basée sur la taille par défaut
    this->regenAmount = 0;


    // --- PARTIE 2 : APPLICATION DES EFFETS DU TRAIT DOMINANT (geneticCode[11]) ---

    // Valeurs par défaut qui peuvent être modifiées par le switch
    float finalSpeedMultiplier = 1.0f;
    float finalMaxStaminaMultiplier = 1.0f;
    float finalRadMultiplier = 1.0f;

    switch (traitID) {
        case 1: // ID 1 : Obèse
            finalMaxStaminaMultiplier = 0.8f; // Stamina * (-0.2)
            finalRadMultiplier = 1.2f;        // rad * 0.2 (Bonus de taille physique)
            break;
        case 9: // ID 9 : Hyperactif
            finalSpeedMultiplier = 1.2f;      // Vitesse * 1.2
            // Le drain de stamina est géré dans update()
            break;
        case 10: // ID 10 : Sédentaire
            finalSpeedMultiplier = 0.7f;      // Vitesse * 0.7
            finalMaxStaminaMultiplier = 1.15f; // Max Stamina * 1.15
            break;
        case 11: // ID 11 : Robuste
            // Réduit les dégâts reçus (ici, on applique une meilleure armure)
            this->armor = std::clamp(this->armor + 0.30f, 0.0f, 0.90f);
            break;
        default:
            // ID 0 (Classique) et les autres IDs (2 à 8) qui gèrent leur effet via les floats [3] à [9]
            break;
    }

    // Application des multiplicateurs
    // Note: Le rad ne peut pas être modifié car il est tiré de geneticCode[0]
    // Cependant, le trait Obèse est censé le modifier.
    // OPTION 1: On ignore la modification de rad ici et on suppose que geneticCode[0] est muté.
    // OPTION 2: On réinterprète le gène Obèse/Robust en modifiant directement les STATS dérivées:

    // Appliquer le multiplicateur de vitesse (Hyperactif / Sédentaire)
    this->speed = (int)(this->speed * finalSpeedMultiplier);
    this->speed = std::max(this->speed, (int)MIN_SPEED);

    // Appliquer le multiplicateur de stamina max (Obèse / Sédentaire)
    this->maxStamina = (int)(this->maxStamina * finalMaxStaminaMultiplier);
    this->stamina = this->maxStamina;


    // --- PARTIE 3 : APPLICATION DES EFFETS GÉNÉTIQUES FLOATS (Traits Individuels) ---

    // 1. Appliquer MyopiaFactor à la vision
    this->sightRadius = (int)(this->sightRadius * (1.0f - myopiaFactor));
    if (this->sightRadius < 50) this->sightRadius = 50;

    // 2. Appliquer les pénalités de Fécondité (Compromis)
    this->maxHealth -= (int)(this->maxHealth * 0.05f * fertilityFactor);
    if (this->maxHealth < 1) this->maxHealth = 1;
    this->health = this->maxHealth; // Finalisation de la santé


    // --- PARTIE 4 : STATS D'ARME ---

    if (isRanged) {
        // ... (Logique Ranged utilisant weaponGene) ...
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
        staminaAttackCost = 5 + (int)(powerBias * 15);

    } else {
        // --- STATS MELEE ---
        // ... (Logique Melee utilisant sizeBias) ...
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
Entity::Entity(std::string name, int x, int y, SDL_Color color,
               const float geneticCode[12], int generation,
               std::string p1_name, std::string p2_name) :
        x(x), y(y), color(color), name(std::move(name)),
        generation(generation),
        parent1_name(std::move(p1_name)),
        parent2_name(std::move(p2_name)){

    this->rad = (int)geneticCode[0]; // FIX: Initialise le nouveau membre rad

    for(int i = 0; i < 12; ++i) {
        this->geneticCode[i] = geneticCode[i];
    }

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

    // 1. Régénération et Vieillissement (Utilise les indices 8 et 5)
    float agingRate = geneticCode[8];
    float baseHealthRegen = geneticCode[5];

    if (agingRate > 0.0f) {
        this->maxHealth -= (int)(this->maxHealth * agingRate * 0.001f * speedMultiplier);
        if (this->maxHealth < 1) this->maxHealth = 1;
        if (this->health > this->maxHealth) this->health = this->maxHealth;
    }

    if (baseHealthRegen > 0.0f && health < maxHealth) {
        if (currentTime > lastRegenTick + effectiveRegenCooldown) {
            health += (int)baseHealthRegen;
            if (health > maxHealth) health = maxHealth;
            lastRegenTick = currentTime;
        }
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

    float damageFragility = geneticCode[3]; // Gène de fragilité

    // Application de l'armure et de la fragilité
    float totalDamageModifier = (1.0f - armor) + damageFragility;
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
    float staminaEfficiency = geneticCode[4];

    // Appliquer l'efficacité de la stamina
    int actualCost = (int)(amount * (1.0f - staminaEfficiency));
    if (actualCost < 1) actualCost = 1;

    if (stamina >= actualCost) {
        stamina -= actualCost;
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