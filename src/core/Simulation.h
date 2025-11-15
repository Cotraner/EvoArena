#ifndef EVOARENA_SIMULATION_H
#define EVOARENA_SIMULATION_H

#include <vector>
#include <map>
#include <string>
#include <SDL2/SDL.h>
#include "../Entity/Entity.h"
#include "../Entity/Projectile.h"

class Simulation {
public:
    explicit Simulation(int maxEntities);
    ~Simulation();

    void handleEvent(const SDL_Event& event);
    void update();
    void render(SDL_Renderer* renderer, bool showDebug);

private:
    // État de la simulation
    std::vector<Entity> entities;
    std::vector<Projectile> projectiles;
    std::map<std::string, Uint32> lastShotTime;
    Entity* selectedEntity;

    // --- NOUVEAU : État de l'animation du panneau ---
    float panelTargetX;
    float panelCurrentX;

    // Fonctions d'aide privées
    void initialize(int maxEntities);
    void drawStatsPanel(SDL_Renderer* renderer, int panelX);
    void updateLogic();
    void updatePhysics();
    void updateProjectiles();
    void cleanupDead();
};

#endif //EVOARENA_SIMULATION_H