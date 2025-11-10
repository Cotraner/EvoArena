#include <random>
#include <iostream>
#include <utility>
#include <cmath>
#include "Entity.h"
#include <SDL2/SDL2_gfxPrimitives.h>


// --- FONCTION STATIQUE ---
SDL_Color Entity::generateRandomColor() {
    return {
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            (Uint8)(std::rand() % 256),
            255
    };
}

// --- FONCTION DE CALCUL DES STATS (Étape 2) ---
void Entity::calculateDerivedStats() {
    const float HEALTH_MULTIPLIER = 4.0f;
    const float SPEED_BASE = 5.0f;
    const float SPEED_DIVISOR = 50.0f;
    const float MIN_SPEED = 2.0f;

    this->maxHealth = (int)(HEALTH_MULTIPLIER * rad);
    this->health = this->maxHealth;

    this->speed = (int)(SPEED_BASE + (SPEED_DIVISOR / (float)rad));
    if (this->speed < MIN_SPEED) this->speed = (int)MIN_SPEED;

    this->maxStamina = this->maxHealth * 2;
    this->stamina = this->maxStamina;

    this->sightRadius = 150 + (rad * 2);
}


// --- CONSTRUCTEUR ---
Entity::Entity(std::string name, int x, int y, int rad, SDL_Color color, bool isRangedGene):
        x(x), y(y), rad(rad), color(color), name(std::move(name)), isRanged(isRangedGene) { // <-- Initialisation directe de isRanged

    direction[0] = 0;
    direction[1] = 0;

    calculateDerivedStats();

    // Initialisation aléatoire du type de combat (50% de chance)
    isRanged = (std::rand() % 2 == 0);
}


Entity::~Entity() = default;


// --- FONCTION DRAW (Affichage Debug) ---
void Entity::draw(SDL_Renderer* renderer) {
    // Dessiner le cercle de l'entité
    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);

    // Dessiner le rayon de vision en blanc (semi-transparent)
    circleRGBA(renderer, x, y, sightRadius, 255, 255, 255, 50);

    // Dessiner les barres de santé et d'endurance
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 5;

    // Pourcentages clampés
    float healthPercent = (float)health / (float)maxHealth;
    healthPercent = std::clamp(healthPercent, 0.0f, 1.0f);

    float staminaPercent = (float)stamina / (float)maxStamina;
    staminaPercent = std::clamp(staminaPercent, 0.0f, 1.0f);

    // Barre de Santé (rouge)
    SDL_Rect healthBarBg = {x - offset - barWidth, y - barHeight / 2, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);
    int healthFillHeight = static_cast<int>(barHeight * healthPercent + 0.5f);
    if (healthFillHeight > 0) {
        SDL_Rect healthBar = {x - offset - barWidth, y + barHeight / 2 - healthFillHeight, barWidth, healthFillHeight};
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }

    // Barre d'Endurance (bleue)
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

    // DEBUG VISUEL : Afficher le type de combat et la portée
    std::string type = isRanged ? "RNG" : "MLY";
    std::string rangeStr = isRanged ? std::to_string(RANGED_RANGE) : std::to_string(MELEE_RANGE);
    std::string debugStr = type + " Rng:" + rangeStr;
    stringRGBA(renderer, x - 30, y + rad + 15, debugStr.c_str(), color.r, color.g, color.b, 255);


    // Dessin de la ligne de visée vers la cible (si une cible est définie)
    if (targetX != -1 && targetY != -1) {
        lineRGBA(renderer, x, y, targetX, targetY, color.r, color.g, color.b, 200);
    }
}

// --- FONCTION UPDATE (Correction Convulsion + Gestion des Bords) ---
void Entity::update() {
    if (health <= 0 && isAlive) {
        die();
        return;
    }

    if (direction[0] == 0 && direction[1] == 0) {
        chooseDirection(); // Choisir une direction aléatoire
    }

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY); // float pour plus de précision

    // 1. Vérification de l'arrivée à la destination
    if (distance < speed && distance > 0.0f) {
        x += (int)distX;
        y += (int)distY;
        direction[0] = 0; // Réinitialiser la direction
        direction[1] = 0;
        clearTarget(); // Réinitialiser la cible de debug
        // Ne retourne pas ici, permet la vérification des bords après le dernier mouvement
    } else if (distance > 0.0f) {
        // 2. Mouvement normal
        float normX = distX / distance;
        float normY = distY / distance;

        x += static_cast<int>(normX * speed);
        y += static_cast<int>(normY * speed);
    }

    // 3. GESTION DES BORDS (Bloque l'entité à l'intérieur de la fenêtre)
    bool collided = false;

    // Bord Gauche/Droit
    if (x < rad) {
        x = rad;
        collided = true;
    } else if (x > WINDOW_SIZE_WIDTH - rad) {
        x = WINDOW_SIZE_WIDTH - rad;
        collided = true;
    }

    // Bord Haut/Bas
    if (y < rad) {
        y = rad;
        collided = true;
    } else if (y > WINDOW_SIZE_HEIGHT - rad) {
        y = WINDOW_SIZE_HEIGHT - rad;
        collided = true;
    }

    // Si l'entité a touché un mur, elle doit choisir une nouvelle direction aléatoire
    if (collided) {
        direction[0] = 0;
        direction[1] = 0;
        clearTarget();
    }
}


// --- FONCTION CHOOSEDIRECTION (Stockage de la cible de debug) ---
void Entity::chooseDirection(int target[2]) {
    //Un ennemi a été détecté
    if(target != nullptr){
        direction[0] = target[0];
        direction[1] = target[1];
        targetX = target[0]; // DEBUG
        targetY = target[1]; // DEBUG
    }
        //Aucun ennemi détécté, choisi une direction aléatoire
    else{
        targetX = -1; // DEBUG
        targetY = -1; // DEBUG

        bool validDirection = false;

        while(!validDirection){
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
            std::uniform_real_distribution<> dis_radius(8 * speed, 12 * speed);
            float radius = dis_radius(gen);
            float angle = dis_angle(gen);

            direction[0] = x + static_cast<int>(radius * cos(angle));
            direction[1] = y + static_cast<int>(radius * sin(angle));

            // La vérification des limites est faite dans update(), mais on vérifie
            // ici pour s'assurer que la destination aléatoire n'est pas complètement folle.
            if(direction[0] > rad && direction[0] < WINDOW_SIZE_WIDTH - rad &&
               direction[1] > rad && direction[1] < WINDOW_SIZE_HEIGHT  - rad){
                validDirection = true;
            }
        }
    }
}

void Entity::knockBack() {
    // ... (Logique inchangée)
    if (direction[0] == 0 && direction[1] == 0) {
        return;
    }

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);

    float normX = distX / distance;
    float normY = distY / distance;

    x -= static_cast<int>(normX * 50);
    y -= static_cast<int>(normY * 50);
}

void Entity::attack(Entity &other) {
    // ... (Logique inchangée)
    if (!isAlive || !other.isAlive) {
        return;
    }

    int damage = 10;
    int staminaCost = 5;

    this->health -= damage;
    this->stamina -= staminaCost;

    other.health -= damage;
    other.stamina -= staminaCost;

    if (this->health < 0) this->health = 0;
    if (this->stamina < 0) this->stamina = 0;

    if (other.health < 0) other.health = 0;
    if (other.stamina < 0) other.stamina = 0;
}

void Entity::die() {
    isAlive = false;
    this->color = {100,100,100,255}; // Devient gris
}