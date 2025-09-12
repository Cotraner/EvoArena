#include <random>
#include <iostream>

#include "Entity.h"


Entity::Entity(int x, int y, int rad, int health, int speed, SDL_Color color):
    x(x), y(y), rad(rad), health(health), speed(speed), color(color) {
}

Entity::~Entity() {

}

void Entity::draw(SDL_Renderer* renderer) {
    filledCircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, 255);
    // Dessiner un contour noir autour
    circleRGBA(renderer, x, y, rad, 0, 0, 0, 255);
}
void Entity::update() {
    // Update entity state if needed
    //Choose a random point within a circle of radius 'speed'
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 2 * M_PI);
    float angle = dis(gen);
    x += speed * cos(angle);
    y += speed * sin(angle);
}



