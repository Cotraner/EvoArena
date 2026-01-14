#include "Projectile.h"
#include <iostream>
#include <utility>

// Constructor: Initializes the projectile's properties and calculates its direction
Projectile::Projectile(int startX, int startY, float targetX, float targetY, int speed, int damage, int range, SDL_Color color, int radius, std::string shooterName)
        : x(startX), y(startY), speed(speed), damage(damage), maxRange(range), distanceTraveled(0), color(color), radius(radius), shooterName(std::move(shooterName)) {

    // Calculate normalized direction vector
    float distX = targetX - startX;
    float distY = targetY - startY;
    float distance = std::sqrt(distX * distX + distY * distY);

    if (distance > 0) {
        dx = distX / distance;
        dy = distY / distance;
    } else {
        dx = 0;
        dy = 0;
        alive = false; // Mark as dead if no valid direction
    }
}

// Destructor: Default behavior
Projectile::~Projectile() = default;

// Updates the projectile's position and checks its state
void Projectile::update() {
    if (!alive) return;

    // Update position based on direction and speed
    x += dx * speed;
    y += dy * speed;

    // Update the distance traveled
    distanceTraveled += speed;

    // Check if the projectile exceeds its maximum range
    if (distanceTraveled >= maxRange) {
        alive = false;
    }

    // Check if the projectile goes out of bounds
    if (x < 0 || x > WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT) {
        alive = false;
    }
}

// Renders the projectile on the screen if it is still active
void Projectile::draw(SDL_Renderer* renderer, const Camera& cam) {
    if (alive) {
        float sx = (x - cam.x) * cam.zoom; // Adjust position based on camera
        float sy = (y - cam.y) * cam.zoom;
        float sr = radius * cam.zoom;     // Adjust radius based on camera zoom
        filledCircleRGBA(renderer, (int)sx, (int)sy, (int)sr, color.r, color.g, color.b, 255);
    }
}
