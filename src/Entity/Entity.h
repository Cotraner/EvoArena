
#ifndef EVOARENA_ENTITY_H
#define EVOARENA_ENTITY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "constants.h"
#include <algorithm>

class Entity {
public:
    Entity(std::string name,int x, int y, int rad,int maxHealth, int speed, int maxStamina, SDL_Color color);
    ~Entity();

    void update();
    void draw(SDL_Renderer* renderer);
    void chooseDirection(int target[2] = nullptr);
    void knockBack();

    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void attack(Entity &other);
    void die();

    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; }
    int getDirectionX() const { return direction[0]; }
    int getDirectionY() const { return direction[1]; }
    int getSightRadius() const { return sightRadius; }
    [[nodiscard]] std::string getName() const { return name; }


    int getHealth() const { return health; }
    void setHealth(int h) { health = h; }
    int getStamina() const { return stamina; }
    void setStamina(int s) { stamina = s; }
    int getMaxHealth() const { return maxHealth; }
    int getMaxStamina() { return maxStamina; }
    bool getIsAlive() const { return isAlive; }




private:
    std::string name;
    int x,y;
    int rad;
    int health;
    int speed;
    int direction[2]{};
    int sightRadius = 150;
    int stamina;
    int maxHealth;
    int maxStamina;
    bool isAlive = true;
    SDL_Color color;

};


#endif //EVOARENA_ENTITY_H
