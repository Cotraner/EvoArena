
#ifndef EVOARENA_ENTITY_H
#define EVOARENA_ENTITY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

class Entity {
public:
    Entity(int x, int y, int rad, int health, int speed, int stamina, SDL_Color color);
    ~Entity();

    void update();
    void draw(SDL_Renderer* renderer);
    void chooseDirection(int target[2] = nullptr);

    int getX() const { return x; }
    int getY() const { return y; }
    int getRad() const { return rad; }
    int getDirectionX() const { return direction[0]; }
    int getDirectionY() const { return direction[1]; }
    int getSightRadius() const { return sightRadius; }


    int getHealth() const { return health; }
    void setHealth(int h) { health = h; }
    int getStamina() const { return stamina; }
    void setStamina(int s) { stamina = s; }
    int getMaxHealth() const { return maxHealth; }
    int getMaxStamina() const { return maxStamina; }



private:
    int x,y;
    int rad;
    int health;
    int speed;
    int direction[2];
    int sightRadius = 50;
    int stamina;
    int maxHealth;
    static const int maxStamina = 100;
    SDL_Color color;

};


#endif //EVOARENA_ENTITY_H
