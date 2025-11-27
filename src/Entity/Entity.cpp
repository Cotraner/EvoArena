#include <random>
#include <iostream>
#include <utility>
#include <cmath>
#include "Entity.h"
#include "TraitManager.h" // <--- INCLUDE IMPORTANT
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
SDL_Color Entity::generateRandomColor() {
    return { (Uint8)(std::rand() % 256), (Uint8)(std::rand() % 256), (Uint8)(std::rand() % 256), 255 };
}

// --- FONCTION DE CALCUL DES STATS ---
void Entity::calculateDerivedStats() {

    const int radBase = (int)geneticCode[0];
    const bool isRanged = geneticCode[10] > 0.5f;
    const int weaponGene = (int)geneticCode[1];
    const float staminaEfficiency = geneticCode[4];
    const float myopiaFactor = geneticCode[6];
    const int fertilityFactor = (int)geneticCode[9];

    // Récupération de l'ID du Trait Dominant et des données JSON
    const int traitID = (int)std::round(geneticCode[11]);
    const TraitStats& traitStats = TraitManager::get(traitID);

    // STATS DU CORPS DE BASE
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    this->maxHealth = (int)(HEALTH_MULTIPLIER * radBase);
    this->health = this->maxHealth;

    // Calcul initial vitesse
    float rawSpeed = SPEED_BASE + (SPEED_DIVISOR / (float)radBase);

    // Calcul initial stamina
    float rawMaxStamina = this->maxHealth * 1.5f;

    this->sightRadius = 150 + (radBase * 2);

    // Armure de base (taille)
    float sizeBias = std::clamp(((float)radBase - 10.0f) / (40.0f - 10.0f), 0.0f, 1.0f);
    this->armor = sizeBias * 0.60f;
    this->regenAmount = 0;


    // --- APPLICATION DES STATS DU JSON (TraitManager) ---

    // 1. Vitesse
    this->speed = (int)(rawSpeed * traitStats.speedMult);
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;

    // 2. Stamina Max
    this->maxStamina = (int)(rawMaxStamina * traitStats.maxStaminaMult);
    this->stamina = this->maxStamina;

    // 3. Armure (Bonus fixe)
    this->armor += traitStats.armorFlatBonus;
    this->armor = std::clamp(this->armor, 0.0f, 0.90f);

    // 4. Modification de la taille visuelle (rad)
    // Note : geneticCode[0] reste la référence génétique, mais 'rad' membre est modifié
    this->rad = (int)(radBase * traitStats.radMult);


    // --- APPLICATION DES EFFETS GÉNÉTIQUES FLOATS (Traits Individuels) ---

    // Myopie
    this->sightRadius = (int)(this->sightRadius * (1.0f - myopiaFactor));
    if (this->sightRadius < 50) this->sightRadius = 50;

    // Fécondité
    this->maxHealth -= (int)(this->maxHealth * 0.05f * fertilityFactor);
    if (this->maxHealth < 1) this->maxHealth = 1;
    this->health = this->maxHealth;

    // --- STATS D'ARME ---
    if (isRanged) {
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
        const int MIN_MELEE_DMG = 15;
        const int MAX_MELEE_DMG = 55;
        const int MIN_MELEE_COOLDOWN = 150;
        const int MAX_MELEE_COOLDOWN = 600;
        damage = MIN_MELEE_DMG + (int)(sizeBias * (MAX_MELEE_DMG - MIN_MELEE_DMG));
        attackCooldown = MIN_MELEE_COOLDOWN + (Uint32)(sizeBias * (MAX_MELEE_COOLDOWN - MIN_MELEE_COOLDOWN));
        attackRange = 50 + (int)(sizeBias * 40);
        projectileSpeed = 0;
        projectileRadius = 0;
        staminaAttackCost = 5 + (int)(sizeBias * 15);
        this->armor = std::clamp(this->armor + 0.25f, 0.0f, 0.90f);
        this->maxHealth += 25;
        this->health = this->maxHealth;
    }

    // Efficacité Stamina
    this->staminaAttackCost = (int)(this->staminaAttackCost * (1.0f - staminaEfficiency));
    if (this->staminaAttackCost < 1) this->staminaAttackCost = 1;
}

// --- CONSTRUCTEUR ---
Entity::Entity(std::string name, int x, int y, SDL_Color color,
               const float geneticCode[12], int generation,
               std::string p1_name, std::string p2_name) :
        x(x), y(y), color(color), name(std::move(name)),
        generation(generation),
        parent1_name(std::move(p1_name)),
        parent2_name(std::move(p2_name)){

    // Copie du code génétique
    for(int i = 0; i < 12; ++i) {
        this->geneticCode[i] = geneticCode[i];
    }

    // Initialisation basique
    this->rad = (int)geneticCode[0];
    direction[0] = 0;
    direction[1] = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
    float angle = dis_angle(gen);
    lastVelX = cos(angle); lastVelY = sin(angle);
    Uint32 startTime = SDL_GetTicks();
    lastRegenTick = startTime - (std::rand() % REGEN_COOLDOWN_MS);
    lastStaminaUseTick = startTime;
    isFleeing = false;
    isCharging = false;

    // Calcul final des stats (y compris TraitManager)
    calculateDerivedStats();
}

Entity::~Entity() = default;

void Entity::draw(SDL_Renderer* renderer, bool showDebug) {

    // --- 1. DESSIN DU CORPS (Différenciation Ranged vs Melee) ---
    bool isRangedUnit = getIsRanged();

    if (isRangedUnit) {
        // === RANGED : Motif "CIBLE / VISEUR" (Statique) ===
        // Concept : Un anneau extérieur + un vide + un point central

        // 1. Cercle extérieur (Couleur de l'entité)
        filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);

        // 2. Cercle noir/foncé pour évider (le "blanc" de l'œil) - 70% de la taille
        // On utilise un gris très foncé (20,20,20) pour simuler le vide
        filledCircleRGBA(renderer, x, y, (int)(rad * 0.65f), 20, 20, 20, 255);

        // 3. Point central (Pupille/Viseur) - 30% de la taille
        filledCircleRGBA(renderer, x, y, (int)(rad * 0.30f), color.r, color.g, color.b, 255);

        // Optionnel : Un cercle blanc fin autour du centre pour faire "briller" le viseur
        circleRGBA(renderer, x, y, (int)(rad * 0.35f), 255, 255, 255, 100);

    } else {
        // === MELEE : Motif "NOYAU / BLINDAGE" (Statique) ===
        // Concept : Un aspect dense et solide

        // 1. Bordure épaisse (Plus foncée que la couleur de base)
        // On assombrit la couleur de base pour faire le bord
        Uint8 r_dark = (Uint8)(color.r * 0.6f);
        Uint8 g_dark = (Uint8)(color.g * 0.6f);
        Uint8 b_dark = (Uint8)(color.b * 0.6f);

        filledCircleRGBA(renderer, x, y, rad, r_dark, g_dark, b_dark, 255);

        // 2. Cœur large (Couleur vive de l'entité) - 80% de la taille
        // Donne l'impression d'une cellule blindée
        filledCircleRGBA(renderer, x, y, (int)(rad * 0.75f), color.r, color.g, color.b, 255);

        // 3. Petit reflet pour le volume (optionnel)
        // circleRGBA(renderer, x, y, rad, 255, 255, 255, 50);
    }


    // --- 2. DEBUG VISUEL (Inchangé) ---
    if (showDebug) {
        circleRGBA(renderer, x, y, sightRadius, 255, 255, 255, 50);

        // Indicateur de Charge (Cercle rouge pulsant ou statique autour)
        if (isCharging) {
            circleRGBA(renderer, x, y, rad + 3, 255, 50, 50, 255);
            circleRGBA(renderer, x, y, rad + 4, 255, 50, 50, 255);
            circleRGBA(renderer, x, y, rad + 5, 255, 50, 50, 150);
        }

        // Affichage des pv
        stringRGBA(renderer, x - 0.1*rad  , y - 0.1*rad,
                   (std::to_string(health)).c_str(), 255, 255, 255, 255);
    }


    // --- 3. BARRES DE VIE (Inchangé) ---
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 6; // Légèrement écarté car certains motifs sont larges

    float healthPercent = (float)health / (float)maxHealth;
    healthPercent = std::clamp(healthPercent, 0.0f, 1.0f);
    float staminaPercent = (float)stamina / (float)maxStamina;
    staminaPercent = std::clamp(staminaPercent, 0.0f, 1.0f);

    // Barre de Vie (Rouge)
    SDL_Rect healthBarBg = {x - offset - barWidth, y - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);

    int healthFillHeight = static_cast<int>(barHeight * healthPercent + 0.5f);
    if (healthFillHeight > 0) {
        SDL_Rect healthBar = {x - offset - barWidth, y + barHeight / 2 - healthFillHeight, barWidth, healthFillHeight};
        SDL_SetRenderDrawColor(renderer, 220, 20, 20, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }

    // Barre de Stamina (Bleue)
    SDL_Rect staminaBarBg = {x + offset, y - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &staminaBarBg);

    int staminaFillHeight = static_cast<int>(barHeight * staminaPercent + 0.5f);
    if (staminaFillHeight > 0) {
        SDL_Rect staminaBar = {x + offset, y + barHeight / 2 - staminaFillHeight, barWidth, staminaFillHeight};
        SDL_SetRenderDrawColor(renderer, 20, 100, 255, 255);
        SDL_RenderFillRect(renderer, &staminaBar);
    }
}

void Entity::update(int speedMultiplier) {
    if (!isAlive) return;
    Uint32 currentTime = SDL_GetTicks();
    Uint32 effectiveRegenCooldown = (speedMultiplier > 0) ? (REGEN_COOLDOWN_MS / speedMultiplier) : REGEN_COOLDOWN_MS;
    Uint32 effectiveStaminaDelay = (speedMultiplier > 0) ? (STAMINA_REGEN_DELAY_MS / speedMultiplier) : STAMINA_REGEN_DELAY_MS;

    float agingRate = geneticCode[8];
    float baseHealthRegen = geneticCode[5];

    // Vieillissement
    if (agingRate > 0.0f) {
        this->maxHealth -= (int)(this->maxHealth * agingRate * 0.001f * speedMultiplier);
        if (this->maxHealth < 1) this->maxHealth = 1;
        if (this->health > this->maxHealth) this->health = this->maxHealth;
    }

    // Régénération
    if (baseHealthRegen > 0.0f && health < maxHealth) {
        if (currentTime > lastRegenTick + effectiveRegenCooldown) {
            health += (int)baseHealthRegen;
            if (health > maxHealth) health = maxHealth;
            lastRegenTick = currentTime;
        }
    }

    // Stamina (Récupération / Fuite)
    bool staminaConsumed = false;
    if (isFleeing) {
        // Coût de mouvement influencé par le Trait (ex: Hyperactif coûte 2x plus)
        int traitID = (int)std::round(geneticCode[11]);
        float moveCostMult = TraitManager::get(traitID).staminaMoveCostMult;
        int cost = (int)(STAMINA_FLEE_COST_PER_FRAME * moveCostMult);
        if (cost < 1) cost = 1;

        if (stamina > 0) {
            stamina -= cost;
            lastStaminaUseTick = currentTime;
            staminaConsumed = true;
            if (stamina < 0) stamina = 0;
        } else isFleeing = false;
    } else if (isCharging) {
        if (stamina > 0) {
            stamina -= STAMINA_CHARGE_COST_PER_FRAME;
            lastStaminaUseTick = currentTime;
            staminaConsumed = true;
            if (stamina < 0) stamina = 0;
        } else isCharging = false;
    }

    if (!staminaConsumed && stamina < maxStamina && currentTime > lastStaminaUseTick + effectiveStaminaDelay) {
        stamina += STAMINA_REGEN_RATE;
        if (stamina > maxStamina) stamina = maxStamina;
    }
    if (health <= 0) { die(); return; }

    float dynamicSpeed = (float)speed;
    if (isCharging) dynamicSpeed *= 1.8f;
    else if (isFleeing) dynamicSpeed *= 0.8f;
    int currentSpeed = (int)(dynamicSpeed * (float)speedMultiplier);
    if (currentSpeed < 1) currentSpeed = 1;

    if (direction[0] == 0 && direction[1] == 0) chooseDirection();

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);

    if (distance < currentSpeed && distance > 0.0f) {
        x += (int)distX; y += (int)distY;
        direction[0] = 0; direction[1] = 0;
        clearTarget();
    } else if (distance > 0.0f) {
        float normX = distX / distance;
        float normY = distY / distance;
        x += static_cast<int>(normX * currentSpeed);
        y += static_cast<int>(normY * currentSpeed);
        lastVelX = normX; lastVelY = normY;
    }

    // Collisions murs
    bool collided = false;
    if (x < rad) { x = rad; collided = true; }
    else if (x > WINDOW_WIDTH - rad) { x = WINDOW_WIDTH - rad; collided = true; }
    if (y < rad) { y = rad; collided = true; }
    else if (y > WINDOW_HEIGHT - rad) { y = WINDOW_HEIGHT - rad; collided = true; }

    if (collided) {
        direction[0] = 0; direction[1] = 0;
        clearTarget();
        lastVelX = -lastVelX; lastVelY = -lastVelY;
        isCharging = false; isFleeing = false;
    }
}

void Entity::chooseDirection(int target[2]) {
    if(target != nullptr){
        direction[0] = target[0]; direction[1] = target[1];
        targetX = target[0]; targetY = target[1];
    } else {
        targetX = -1; targetY = -1;
        direction[0] = target[0]; direction[1] = target[1];
        targetX = target[0]; targetY = target[1];
    } else {
        targetX = -1; targetY = -1;
        const float WANDER_DISTANCE = 90.0f;
        const float WANDER_JITTER_STRENGTH = 0.4f;
        std::random_device rd; std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis_jitter(-1.0, 1.0);
        float jitterX = dis_jitter(gen);
        float jitterY = dis_jitter(gen);

        float newDirX = (lastVelX * (1.0f - WANDER_JITTER_STRENGTH)) + jitterX * WANDER_JITTER_STRENGTH;
        float newDirY = (lastVelY * (1.0f - WANDER_JITTER_STRENGTH)) + jitterY * WANDER_JITTER_STRENGTH;

        float newMag = std::sqrt(newDirX * newDirX + newDirY * newDirY);
        if (newMag > 0.0f) { newDirX /= newMag; newDirY /= newMag; }
        else { std::uniform_real_distribution<> dis_angle(0, 2 * M_PI); float angle = dis_angle(gen); newDirX = cos(angle); newDirY = sin(angle); }
        direction[0] = x + static_cast<int>(newDirX * WANDER_DISTANCE);
        direction[1] = y + static_cast<int>(newDirY * WANDER_DISTANCE);
        lastVelX = newDirX; lastVelY = newDirY;
        lastVelX = newDirX; lastVelY = newDirY;
    }
}

// --- IMPLEMENTATION DU KNOCKBACK ---
void Entity::knockBackFrom(int sourceX, int sourceY, int force) {
    // Calcul du vecteur : Source -> Moi
    float dx = (float)(x - sourceX);
    float dy = (float)(y - sourceY);
    float dist = std::sqrt(dx*dx + dy*dy);

    // Si la distance est quasi nulle (superposition), on pousse arbitrairement
    if (dist < 0.1f) { dx = 1.0f; dy = 0.0f; dist = 1.0f; }

    float normX = dx / dist;
    float normY = dy / dist;

    // Application du déplacement
    x += (int)(normX * (float)force);
    y += (int)(normY * (float)force);

    // Clamping aux murs
    x = std::clamp(x, rad, WINDOW_WIDTH - rad);
    y = std::clamp(y, rad, WINDOW_HEIGHT - rad);

    // Effet "Stun" : on annule le mouvement en cours
    direction[0] = 0;
    direction[1] = 0;
    clearTarget();

    // Le choc interrompt la charge ou la fuite
    isCharging = false;
    isFleeing = false;
}

void Entity::takeDamage(int amount) {
    if (!isAlive) return;
    float damageFragility = geneticCode[3];
    float damageFragility = geneticCode[3];
    float totalDamageModifier = (1.0f - armor) + damageFragility;
    int damageTaken = static_cast<int>(amount * totalDamageModifier);
    if (damageTaken < 1 && amount > 0) damageTaken = 1;
    this->health -= damageTaken;
    if (this->health <= 0) { health = 0; die(); }
}


bool Entity::consumeStamina(int amount) {
    float staminaEfficiency = geneticCode[4];
    int actualCost = (int)(amount * (1.0f - staminaEfficiency));
    if (actualCost < 1) actualCost = 1;
    if (stamina >= actualCost) {
        stamina -= actualCost;
        lastStaminaUseTick = SDL_GetTicks();
        return true;
    }
    return false;
}

void Entity::die() {
    isAlive = false;
    this->color = {100,100,100,255};
}
