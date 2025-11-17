#ifndef EVOARENA_PROJECTILE_H
#define EVOARENA_PROJECTILE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "../constants.h"
#include <cmath>
#include <string> // *** AJOUTÉ ***

class Projectile {
public:
    // *** SIGNATURE DU CONSTRUCTEUR MISE À JOUR ***
    Projectile(int startX, int startY, float targetX, float targetY, int speed, int damage, int range, SDL_Color color, int radius, std::string shooterName);
    ~Projectile();

    void update();
    void draw(SDL_Renderer* renderer);

    // Getters
    int getX() const { return (int)x; }
    int getY() const { return (int)y; }
    int getDamage() const { return damage; }
    int getRadius() const { return radius; }
    bool isAlive() const { return alive; }
    std::string getShooterName() const { return shooterName; } // *** AJOUTÉ ***

    // Setter
    void setDead() { alive = false; }


private:
    float x, y;
    float dx, dy; // Direction normalisée
    int speed;
    int damage;
    int maxRange;
    float distanceTraveled;
    bool alive = true;
    SDL_Color color;
    int radius;
    std::string shooterName;
};


#endif //EVOARENA_PROJECTILE_H