#include <random>
#include <iostream>
#include <utility>
#include "Entity.h"


Entity::Entity(std::string name, int x, int y, int rad,int maxHealth, int speed,int maxStamina, SDL_Color color):
    x(x), y(y), rad(rad), health(maxHealth),maxHealth(maxHealth), speed(speed),stamina(maxStamina),maxStamina(maxStamina), color(color), name(std::move(name)) {
    direction[0] = 0;
    direction[1] = 0;

}


Entity::~Entity() {

}


void Entity::draw(SDL_Renderer* renderer) {
    // Draw the entity circle
    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);

    // Draw health and stamina bars (unchanged)
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 5;

    //draw sight radius in white
    circleRGBA(renderer, x, y, sightRadius, 255, 255, 255, 50);

    // Pourcentages clampés
    // Pourcentages
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

    // Draw health value as text inside the entity
    std::string healthText = std::to_string(health);
    stringRGBA(renderer, x - 10, y - 5, healthText.c_str(), 0, 0, 0, 255);
}

void Entity::update() {
    if (health <= 0 && isAlive) {
        die();
        return;
    }
    if (direction[0] == 0 && direction[1] == 0) {
        chooseDirection();
        return ;
    }

    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);


    if (distance < speed) {  // tolérance
        x+= distX;
        y+= distY;
        direction[0] = 0;
        direction[1] = 0;
        return;
    }
    float normX = distX / distance;
    float normY = distY / distance;

    x += static_cast<int>(normX * speed);
    y += static_cast<int>(normY * speed);
}


void Entity::chooseDirection(int target[2]) {
    //Un ennemi a été détecté
    if(target != nullptr){
        direction[0] = target[0];
        direction[1] = target[1];
    }
    //Aucun ennemi détécté, choisi une direction aléatoire
    else{
        bool validDirection = false;

        while(!validDirection){
            // Choisir un point aléatoire dans un cercle de rayon de 8 à 12 fois la vitesse de l'entité
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis_angle(0, 2 * M_PI);
            std::uniform_real_distribution<> dis_radius(8 * speed, 12 * speed);
            float radius = dis_radius(gen);
            float angle = dis_angle(gen);

            // Calculer le déplacement
            direction[0] = x + static_cast<int>(radius * cos(angle));
            direction[1] = y + static_cast<int>(radius * sin(angle));

            //Vérifie si la direction est valide et ne sort pas de l'écran
            if(direction[0] > rad && direction[0] < WINDOW_SIZE_WIDTH - rad && direction[1] > rad && direction[1] < WINDOW_SIZE_HEIGHT  - rad){
                validDirection = true;
            }
        }
    }

}

void Entity::knockBack() {
    // Reculer de 10 pixels dans la direction opposée
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
    // Vérifie si l'entité attaquante est vivante
    if (!isAlive) {
        std::cout << "Dead entities cannot attack!" << std::endl;
        return;
    }
    // Vérifie si la cible est vivante
    if (!other.isAlive) {
        std::cout << "Cannot attack a dead entity!" << std::endl;
        return;
    }

    int damage = 10; // Dégâts infligés
    int staminaCost = 5; // Coût en endurance

    // Réduction de la santé et de l'endurance des deux entités
    this->health -= damage;
    this->stamina -= staminaCost;

    other.health -= damage;
    other.stamina -= staminaCost;

    // Empêcher les valeurs négatives
    if (this->health < 0) this->health = 0;
    if (this->stamina < 0) this->stamina = 0;

    if (other.health < 0) other.health = 0;
    if (other.stamina < 0) other.stamina = 0;
}

void Entity::die() {
    isAlive = false;
    this->color = {100,100,100,255};
}



