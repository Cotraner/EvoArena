#include <random>
#include <iostream>
#include "Entity.h"


Entity::Entity(int x, int y, int rad, int health, int speed,int stamina, SDL_Color color):
    x(x), y(y), rad(rad), health(health), speed(speed), color(color) {
    direction[0] = 0;
    direction[1] = 0;
}

Entity::~Entity() {

}

void Entity::draw(SDL_Renderer* renderer) {

    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);
    // Dessiner un contour noir autour
    circleRGBA(renderer, x, y, rad, 0, 0, 0, 255);
    // Dimensions des barres (liées à la taille de l'entité)
    int barWidth = 6;
    int barHeight = 2 * rad;
    int offset = rad + 5;

    // Pourcentages clampés entre 0 et 1
    float healthPercent = (float)health / (float)maxHealth;
    if (healthPercent < 0) healthPercent = 0;
    if (healthPercent > 1) healthPercent = 1;

    float staminaPercent = (float)stamina / (float)maxStamina;
    if (staminaPercent < 0) staminaPercent = 0;
    if (staminaPercent > 1) staminaPercent = 1;

    // --- Barre de vie (rouge, gauche) ---
    SDL_Rect healthBarBg = {x - offset - barWidth, y - barHeight/2, barWidth, barHeight};

    int healthFillHeight = (int)(barHeight * healthPercent + 0.5f); // arrondi
    if (healthFillHeight > 0) {
        SDL_Rect healthBar = {
                x - offset - barWidth,
                y + barHeight/2 - healthFillHeight,
                barWidth,
                healthFillHeight
        };

        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderDrawRect(renderer, &healthBarBg); // contour au lieu de fond plein

    // --- Barre d’endurance (bleue, droite) ---
    SDL_Rect staminaBarBg = {x + offset, y - barHeight/2, barWidth, barHeight};

    int staminaFillHeight = (int)(barHeight * staminaPercent + 0.5f);
    if (staminaFillHeight > 0) {
        SDL_Rect staminaBar = {
                x + offset,
                y + barHeight/2 - staminaFillHeight,
                barWidth,
                staminaFillHeight
        };

        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        SDL_RenderFillRect(renderer, &staminaBar);
    }

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderDrawRect(renderer, &staminaBarBg);
}

void Entity::update() {
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
    if(target != nullptr){


    }
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



