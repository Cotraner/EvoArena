#include "Projectile.h"
#include <iostream>
#include <utility>

Projectile::Projectile(int startX, int startY, float targetX, float targetY, int speed, int damage, int range, SDL_Color color, int radius, std::string shooterName)
        : x(startX), y(startY), speed(speed), damage(damage), maxRange(range), distanceTraveled(0), color(color), radius(radius), shooterName(std::move(shooterName)) {

    // Calculer la direction
    float distX = targetX - startX;
    float distY = targetY - startY;
    float distance = std::sqrt(distX * distX + distY * distY);

    if (distance > 0) {
        dx = distX / distance;
        dy = distY / distance;
    } else {
        dx = 0;
        dy = 0;
        alive = false;
    }
}

Projectile::~Projectile() = default;

void Projectile::update() {
    if (!alive) return;

    // Mise à jour de la position
    x += dx * speed;
    y += dy * speed;

    // Mise à jour de la distance parcourue
    distanceTraveled += speed;

    // Vérifier la portée maximale
    if (distanceTraveled >= maxRange) {
        alive = false;
    }

    // Vérifier si le projectile sort de l'écran
    if (x < 0 || x > WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT) { alive = false; }
}

void Projectile::draw(SDL_Renderer* renderer, const Camera& cam) {
    if (alive) {
        float sx = (x - cam.x) * cam.zoom;
        float sy = (y - cam.y) * cam.zoom;
        float sr = radius * cam.zoom;
        filledCircleRGBA(renderer, (int)sx, (int)sy, (int)sr, color.r, color.g, color.b, 255);
    }
}