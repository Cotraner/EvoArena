#include <random>
#include <iostream>
#include <utility>
#include <cmath>
#include "Entity.h"
#include "TraitManager.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Generates a random color for the entity
SDL_Color Entity::generateRandomColor() {
    return { (Uint8)(std::rand() % 256), (Uint8)(std::rand() % 256), (Uint8)(std::rand() % 256), 255 };
}

// Calculates derived stats based on genetic code and traits
void Entity::calculateDerivedStats() {
    // Extract genetic code and trait data
    const int radBase = (int)geneticCode[0];
    const int entityType = getEntityType();
    const int weaponGene = (int)geneticCode[1];
    const float staminaEfficiency = geneticCode[4];
    const float myopiaFactor = geneticCode[6];
    const int fertilityFactor = (int)geneticCode[9];
    const int traitID = (int)std::round(geneticCode[11]);
    const TraitStats& traitStats = TraitManager::get(traitID);

    // Global balancing constants
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    // Base stat calculations
    this->maxHealth = (int)(HEALTH_MULTIPLIER * radBase);
    float rawSpeed = SPEED_BASE + (SPEED_DIVISOR / (float)radBase);
    float rawMaxStamina = this->maxHealth * 1.5f;
    this->sightRadius = 150 + (radBase * 2);
    float sizeBias = std::clamp(((float)radBase - 10.0f) / (40.0f - 10.0f), 0.0f, 1.0f);
    this->armor = sizeBias * 0.40f;

    // Apply trait modifiers
    this->speed = (int)(rawSpeed * traitStats.speedMult);
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;
    this->maxStamina = (int)(rawMaxStamina * traitStats.maxStaminaMult);
    this->stamina = this->maxStamina;
    this->rad = (int)(radBase * traitStats.radMult);
    this->armor += traitStats.armorFlatBonus;

    // Apply genetic modifiers
    this->sightRadius = (int)(this->sightRadius * (1.0f - myopiaFactor));
    if (this->sightRadius < 50) this->sightRadius = 50;
    this->maxHealth -= (int)(this->maxHealth * 0.10f * fertilityFactor);
    this->maxHealth = (int)(this->maxHealth * traitStats.maxHealthMult);
    if (this->maxHealth < 1) this->maxHealth = 1;
    this->health = this->maxHealth;

    // Combat stats based on entity type
    float traitDmgMult = traitStats.damageMult;
    if (entityType == 1 || entityType == 2) {
        // Ranged or Healer
        const int MIN_RANGED_DMG = 5, MAX_RANGED_DMG = 25;
        const int MIN_RANGED_RANGE = 180, MAX_RANGED_RANGE = 650;
        float powerBias = (float)weaponGene / 100.0f;
        float rangeBias = 1.0f - powerBias;

        damage = MIN_RANGED_DMG + (int)(powerBias * (MAX_RANGED_DMG - MIN_RANGED_DMG));
        damage = (int)(damage * traitDmgMult);
        attackRange = MIN_RANGED_RANGE + (int)(rangeBias * (MAX_RANGED_RANGE - MIN_RANGED_RANGE));
        attackCooldown = 500 + (Uint32)(powerBias * 2000);
        projectileSpeed = 14 - (int)(powerBias * 6);
        projectileRadius = 4 + (int)(powerBias * 8);
        staminaAttackCost = 5 + (int)(powerBias * 20);

        if (entityType == 2) { // Healer specialization
            this->maxHealth = (int)(this->maxHealth * 1.3f);
            this->health = this->maxHealth;
            this->armor += 0.10f;
            damage = (int)(damage * 0.5f);
            if (damage < 1) damage = 1;
        }
    } else {
        // Melee
        const int MIN_MELEE_DMG = 15, MAX_MELEE_DMG = 60;
        const int MIN_MELEE_COOLDOWN = 400, MAX_MELEE_COOLDOWN = 1200;

        damage = MIN_MELEE_DMG + (int)(sizeBias * (MAX_MELEE_DMG - MIN_MELEE_DMG));
        damage = (int)(damage * traitDmgMult);
        attackCooldown = MIN_MELEE_COOLDOWN + (Uint32)(sizeBias * (MAX_MELEE_COOLDOWN - MIN_MELEE_COOLDOWN));
        attackRange = 50 + (int)(sizeBias * 40);
        projectileSpeed = 0;
        projectileRadius = 0;
        staminaAttackCost = 10 + (int)(sizeBias * 20);

        this->armor += 0.15f;
        this->maxHealth += 20;
        this->health = this->maxHealth;
    }

    // Final adjustments
    this->armor = std::clamp(this->armor, 0.0f, 0.85f);
    this->staminaAttackCost = (int)(this->staminaAttackCost * (1.0f - staminaEfficiency));
    if (this->staminaAttackCost < 1) this->staminaAttackCost = 1;

    if (this->getEntityType() == 2) {
        this->maxHealth = (int)(this->maxHealth * 0.60f);
        this->damage = (int)(this->damage * 0.50f);
    }
}

// Constructor: Initializes the entity with its genetic code and other properties
Entity::Entity(std::string name, int x, int y, SDL_Color color,
               const float geneticCode[12], int generation,
               std::string p1_name, std::string p2_name) :
        x(x), y(y), color(color), name(std::move(name)),
        generation(generation), parent1_name(std::move(p1_name)), parent2_name(std::move(p2_name)) {
    for (int i = 0; i < 14; ++i) this->geneticCode[i] = geneticCode[i];
    this->rad = (int)geneticCode[0];
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
    isFleeing = false;
    isCharging = false;
    calculateDerivedStats();
}

// Destructor: Default behavior
Entity::~Entity() = default;

// Draws the entity on the screen, including debug visuals and health/stamina bars
void Entity::draw(SDL_Renderer* renderer, const Camera& cam, bool showDebug) {
    // Transform camera coordinates
    int screenX = (int)((x - cam.x) * cam.zoom);
    int screenY = (int)((y - cam.y) * cam.zoom);
    int screenRad = (int)(rad * cam.zoom);

    // Skip drawing if off-screen
    if (screenX + screenRad < 0 || screenX - screenRad > WINDOW_WIDTH ||
        screenY + screenRad < 0 || screenY - screenRad > WINDOW_HEIGHT) {
        return;
    }

    // Flash effect when eating
    if (flashTimer > 0) {
        filledCircleRGBA(renderer, screenX, screenY, screenRad, 255, 255, 255, 255);
        flashTimer--;
    } else {
        // Draw entity based on type
        int type = getEntityType();

        if (type == 1) {
            // Ranged
            filledCircleRGBA(renderer, screenX, screenY, screenRad, color.r, color.g, color.b, 255);
            filledCircleRGBA(renderer, screenX, screenY, (int)(screenRad * 0.65f), 20, 20, 20, 255);
            filledCircleRGBA(renderer, screenX, screenY, (int)(screenRad * 0.30f), color.r, color.g, color.b, 255);
            circleRGBA(renderer, screenX, screenY, (int)(screenRad * 0.35f), 255, 255, 255, 100);
        } else if (type == 2) {
            // Healer
            filledCircleRGBA(renderer, screenX, screenY, screenRad, color.r, color.g, color.b, 255);

            Uint8 r_dark = (Uint8)(color.r * 0.7f);
            Uint8 g_dark = (Uint8)(color.g * 0.7f);
            Uint8 b_dark = (Uint8)(color.b * 0.7f);
            circleRGBA(renderer, screenX, screenY, screenRad, r_dark, g_dark, b_dark, 255);

            int w = (int)(screenRad * 0.5f);
            int t = (int)(screenRad * 0.2f);
            boxRGBA(renderer, screenX - w, screenY - t, screenX + w, screenY + t, 40, 40, 40, 255);
            boxRGBA(renderer, screenX - t, screenY - w, screenX + t, screenY + w, 40, 40, 40, 255);

            rectangleRGBA(renderer, screenX - w, screenY - t, screenX + w, screenY + t, 255, 255, 255, 100);
            rectangleRGBA(renderer, screenX - t, screenY - w, screenX + t, screenY + w, 255, 255, 255, 100);
        } else {
            // Melee
            Uint8 r_dark = (Uint8)(color.r * 0.6f);
            Uint8 g_dark = (Uint8)(color.g * 0.6f);
            Uint8 b_dark = (Uint8)(color.b * 0.6f);
            filledCircleRGBA(renderer, screenX, screenY, screenRad, r_dark, g_dark, b_dark, 255);
            filledCircleRGBA(renderer, screenX, screenY, (int)(screenRad * 0.75f), color.r, color.g, color.b, 255);
        }
    }

    // Debug visuals
    if (showDebug) {
        int screenSight = (int)(sightRadius * cam.zoom);
        circleRGBA(renderer, screenX, screenY, screenSight, 255, 255, 255, 50);
        if (isCharging) {
            circleRGBA(renderer, screenX, screenY, screenRad + 3, 255, 50, 50, 255);
        }
        stringRGBA(renderer, screenX - (int)(0.1 * screenRad), screenY - (int)(0.1 * screenRad), std::to_string(health).c_str(), 255, 255, 255, 255);
    }

    // Health and stamina bars
    int barWidth = (int)(6 * cam.zoom);
    if (barWidth < 2) barWidth = 2;
    int barHeight = 2 * screenRad;
    int offset = screenRad + barWidth + 2;

    float healthPercent = std::clamp((float)health / (float)maxHealth, 0.0f, 1.0f);
    float staminaPercent = std::clamp((float)stamina / (float)maxStamina, 0.0f, 1.0f);

    // Health bar
    SDL_Rect healthBarBg = {screenX - offset - barWidth, screenY - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);

    int healthFillHeight = static_cast<int>(barHeight * healthPercent + 0.5f);
    if (healthFillHeight > 0) {
        SDL_Rect healthBar = {screenX - offset - barWidth, screenY + barHeight / 2 - healthFillHeight, barWidth, healthFillHeight};
        SDL_SetRenderDrawColor(renderer, 220, 20, 20, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }

    // Stamina bar
    SDL_Rect staminaBarBg = {screenX + offset, screenY - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &staminaBarBg);

    int staminaFillHeight = static_cast<int>(barHeight * staminaPercent + 0.5f);
    if (staminaFillHeight > 0) {
        SDL_Rect staminaBar = {screenX + offset, screenY + barHeight / 2 - staminaFillHeight, barWidth, staminaFillHeight};
        SDL_SetRenderDrawColor(renderer, 20, 100, 255, 255);
        SDL_RenderFillRect(renderer, &staminaBar);
    }
}

// Updates the entity's state, including movement, stamina, and health regeneration
void Entity::update(int speedMultiplier) {
    if (!isAlive) return;

    Uint32 currentTime = SDL_GetTicks();
    Uint32 effectiveRegenCooldown = (speedMultiplier > 0) ? (REGEN_COOLDOWN_MS / speedMultiplier) : REGEN_COOLDOWN_MS;
    Uint32 effectiveStaminaDelay = (speedMultiplier > 0) ? (STAMINA_REGEN_DELAY_MS / speedMultiplier) : STAMINA_REGEN_DELAY_MS;

    float agingRate = geneticCode[8];
    float baseHealthRegen = geneticCode[5];

    // Aging effect
    if (agingRate > 0.0f) {
        this->maxHealth -= (int)(this->maxHealth * agingRate * 0.001f * speedMultiplier);
        if (this->maxHealth < 1) this->maxHealth = 1;
        if (this->health > this->maxHealth) this->health = this->maxHealth;
    }

    // Health regeneration
    if (baseHealthRegen > 0.0f && health < maxHealth) {
        if (currentTime > lastRegenTick + effectiveRegenCooldown) {
            health += (int)baseHealthRegen;
            if (health > maxHealth) health = maxHealth;
            lastRegenTick = currentTime;
        }
    }

    // Stamina consumption
    bool staminaConsumed = false;
    if (isFleeing) {
        int traitID = (int)std::round(geneticCode[11]);
        float moveCostMult = TraitManager::get(traitID).staminaMoveCostMult;
        int cost = (int)(STAMINA_FLEE_COST_PER_FRAME * moveCostMult);
        if (cost < 1) cost = 1;

        if (stamina > 0) {
            stamina -= cost;
            lastStaminaUseTick = currentTime;
            staminaConsumed = true;
            if (stamina < 0) stamina = 0;
        } else {
            isFleeing = false;
        }
    } else if (isCharging) {
        if (stamina > 0) {
            stamina -= STAMINA_CHARGE_COST_PER_FRAME;
            lastStaminaUseTick = currentTime;
            staminaConsumed = true;
            if (stamina < 0) stamina = 0;
        } else {
            isCharging = false;
        }
    }

    // Stamina regeneration
    int traitID = (int)std::round(geneticCode[11]);
    float regenBonus = TraitManager::get(traitID).staminaRegenBonus;
    int netRegen = STAMINA_REGEN_RATE + (int)regenBonus;

    if (netRegen < 0) {
        int tickRate = 2 * (speedMultiplier > 0 ? speedMultiplier : 1);
        if (currentTime % tickRate == 0) {
            stamina += netRegen;
            if (stamina < 0) stamina = 0;
        }
    } else if (!staminaConsumed && stamina < maxStamina && currentTime > lastStaminaUseTick + effectiveStaminaDelay) {
        stamina += netRegen;
        if (stamina > maxStamina) stamina = maxStamina;
    }

    // Death check
    if (health <= 0) {
        die();
        return;
    }

    // Movement and physics
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

    // World boundary collision
    bool collided = false;
    if (x < rad) {
        x = rad;
        collided = true;
    } else if (x > WORLD_WIDTH - rad) {
        x = WORLD_WIDTH - rad;
        collided = true;
    }

    if (y < rad) {
        y = rad;
        collided = true;
    } else if (y > WORLD_HEIGHT - rad) {
        y = WORLD_HEIGHT - rad;
        collided = true;
    }

    if (collided) {
        direction[0] = 0;
        direction[1] = 0;
        clearTarget();
        lastVelX = -lastVelX;
        lastVelY = -lastVelY;
        isCharging = false;
        isFleeing = false;
    }
}

// Chooses a new direction for the entity to move toward
void Entity::chooseDirection(int target[2]) {
    if (target != nullptr) {
        direction[0] = target[0];
        direction[1] = target[1];
        targetX = target[0];
        targetY = target[1];
    } else {
        targetX = -1;
        targetY = -1;
        const float WANDER_DISTANCE = 90.0f;
        const float WANDER_JITTER_STRENGTH = 0.4f;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis_jitter(-1.0, 1.0);
        float jitterX = dis_jitter(gen);
        float jitterY = dis_jitter(gen);
        float newDirX = (lastVelX * (1.0f - WANDER_JITTER_STRENGTH)) + jitterX * WANDER_JITTER_STRENGTH;
        float newDirY = (lastVelY * (1.0f - WANDER_JITTER_STRENGTH)) + jitterY * WANDER_JITTER_STRENGTH;
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

// Applies a knockback effect to the entity
void Entity::knockBackFrom(int sourceX, int sourceY, int force) {
    float dx = (float)(x - sourceX);
    float dy = (float)(y - sourceY);
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.1f) {
        dx = 1.0f;
        dy = 0.0f;
        dist = 1.0f;
    }
    float normX = dx / dist;
    float normY = dy / dist;
    x += (int)(normX * (float)force);
    y += (int)(normY * (float)force);

    x = std::clamp(x, rad, WORLD_WIDTH - rad);
    y = std::clamp(y, rad, WORLD_HEIGHT - rad);

    direction[0] = 0;
    direction[1] = 0;
    clearTarget();
    isCharging = false;
    isFleeing = false;
}

// Reduces the entity's health when taking damage
void Entity::takeDamage(int amount) {
    if (!isAlive) return;
    float damageFragility = geneticCode[3];
    float totalDamageModifier = (1.0f - armor) + damageFragility;
    int damageTaken = static_cast<int>(amount * totalDamageModifier);
    if (damageTaken < 1 && amount > 0) damageTaken = 1;
    this->health -= damageTaken;
    if (this->health <= 0) {
        health = 0;
        die();
    }
}

// Consumes stamina for an action, returning whether the action is possible
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

// Marks the entity as dead and changes its color
void Entity::die() {
    isAlive = false;
    this->color = {100, 100, 100, 255};
}
