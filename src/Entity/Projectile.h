#ifndef EVOARENA_PROJECTILE_H
#define EVOARENA_PROJECTILE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "../constants.h"
#include <cmath>
#include <string>

// Represents a projectile in the game, including its movement, rendering, and state.
class Projectile {
public:
    // Constructor and destructor
    Projectile(int startX, int startY, float targetX, float targetY, int speed, int damage, int range, SDL_Color color, int radius, std::string shooterName);
    ~Projectile();

    // Updates the projectile's position and state
    void update();

    // Renders the projectile on the screen
    void draw(SDL_Renderer* renderer, const Camera& cam);

    // Getters for projectile properties
    int getX() const { return (int)x; }
    int getY() const { return (int)y; }
    int getDamage() const { return damage; }
    int getRadius() const { return radius; }
    bool isAlive() const { return alive; }
    std::string getShooterName() const { return shooterName; }

    // Marks the projectile as dead
    void setDead() { alive = false; }

private:
    // Position and movement
    float x, y;       // Current position
    float dx, dy;     // Normalized direction vector
    int speed;        // Movement speed

    // Projectile properties
    int damage;       // Damage dealt by the projectile
    int maxRange;     // Maximum range the projectile can travel
    float distanceTraveled; // Distance traveled so far
    bool alive = true; // Whether the projectile is still active

    // Rendering properties
    SDL_Color color;  // Color of the projectile
    int radius;       // Radius of the projectile

    // Metadata
    std::string shooterName; // Name of the entity that fired the projectile
};

#endif //EVOARENA_PROJECTILE_H
