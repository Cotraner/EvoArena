
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

    void randomMove();

    int getHealth() const { return health; }
    void setHealth(int h) { health = h; }
    int getStamina() const { return stamina; }
    void setStamina(int s) { stamina = s; }



private:
    int x,y;
    int rad;
    int health;
    int speed;
    int stamina;
    SDL_Color color;

};


#endif //EVOARENA_ENTITY_H
