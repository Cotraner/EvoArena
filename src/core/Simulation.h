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
    enum class SimUpdateStatus {
        RUNNING,
        FINISHED
    };

    explicit Simulation(int maxEntities);
    ~Simulation();

    void handleEvent(const SDL_Event& event);
    SimUpdateStatus update(int speedMultiplier, bool autoRestart);
    void render(SDL_Renderer* renderer, bool showDebug);
    int getCurrentGeneration() const { return currentGeneration; }

    void triggerManualRestart();

private:
    int currentGeneration = 0;
    int maxEntities;
    std::vector<Entity> entities;
    std::vector<Projectile> projectiles;
    std::map<std::string, Uint32> lastShotTime;
    std::map<std::string, Entity> genealogyArchive;
    Entity* selectedLivingEntity;
    std::vector<Entity> inspectionStack;
    std::vector<Entity> lastSurvivors;

    float panelTargetX;
    float panelCurrentX;

    // UI Hitboxes
    SDL_Rect panelParent1_rect{};
    SDL_Rect panelParent2_rect{};
    SDL_Rect panelBack_rect{};

    // Fonctions d'aide privées
    void initialize(int initialEntityCount);
    void triggerReproduction(const std::vector<Entity>& parents);
    void drawStatsPanel(SDL_Renderer* renderer, int panelX);
    void updateLogic(int speedMultiplier);
    void updatePhysics(int speedMultiplier);
    void updateProjectiles();
    void cleanupDead();

    // --- SYSTÈME DE NOURRITURE ---
    struct Food {
        int x, y;
        int radius = 4; // Taille visuelle
    };
    std::vector<Food> foods;

    // Paramètres de la nourriture
    const int MAX_FOOD_COUNT = 60;       // Limite max sur la carte
    const int FOOD_SPAWN_RATE = 5;       // % de chance de spawn par frame
    const int FOOD_STAMINA_GAIN = 50;    // Combien d'énergie ça rend
    const int FOOD_HEAL_GAIN = 10;       // (Optionnel) Un petit soin bonus

    // Nouvelles fonctions privées
    void spawnFood();
    void updateFood(int speedMultiplier);
};

#endif //EVOARENA_SIMULATION_H