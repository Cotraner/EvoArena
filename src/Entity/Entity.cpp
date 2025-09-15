#include <random>
#include <iostream>

#include "Entity.h"


Entity::Entity(int x, int y, int rad, int health, int speed, SDL_Color color):
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
}

void Entity::update() {
    // 1. Si pas encore de direction -> en choisir une
    if (direction[0] == 0 && direction[1] == 0) {
        chooseDirection();
        return ;
    }

    // 2. Calculer le vecteur vers la cible
    float distX = direction[0] - x;
    float distY = direction[1] - y;
    float distance = std::sqrt(distX * distX + distY * distY);


    // 3. Vérifier si on est assez proche de la cible
    if (distance < speed) {  // tolérance
        x+= distX;
        y+= distY;
        direction[0] = 0;
        direction[1] = 0;
        return;
    }

    // 4. Normaliser puis avancer
    float normX = distX / distance;
    float normY = distY / distance;

    x += static_cast<int>(normX * speed);
    y += static_cast<int>(normY * speed);
}


void Entity::chooseDirection(int target[2]) {
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
        if(direction[0] > rad && direction[0] < 640 - rad && direction[1] > rad && direction[1] < 480 - rad){
            validDirection = true;
        }
    }
}



