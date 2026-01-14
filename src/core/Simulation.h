#ifndef EVOARENA_SIMULATION_H
#define EVOARENA_SIMULATION_H

#include <vector>
#include <map>
#include <string>
#include <SDL2/SDL.h>
#include <thread>
#include <mutex>
#include "../Entity/Entity.h"
#include "../Entity/Projectile.h"

// Manages the simulation, including entities, projectiles, and game logic
class Simulation {
public:
    // Status of the simulation update
    enum class SimUpdateStatus {
        RUNNING,
        FINISHED
    };

    // Constructor and destructor
    explicit Simulation(int maxEntities);
    ~Simulation();

    // Updates the simulation state
    SimUpdateStatus update(int speedMultiplier, bool autoRestart);

    // Handles user input events
    void handleEvent(const SDL_Event& event, const Camera& cam);

    // Renders the simulation
    void render(SDL_Renderer* renderer, bool showDebug, const Camera& cam);

    // Restarts the simulation manually
    void triggerManualRestart();

    // Returns the current generation number
    int getCurrentGeneration() const { return currentGeneration; }

private:
    // Simulation state
    int currentGeneration = 0;
    int maxEntities;
    std::vector<Entity> entities;
    std::vector<Projectile> projectiles;
    std::map<std::string, Uint32> lastShotTime;
    std::map<std::string, Entity> genealogyArchive;
    Entity* selectedLivingEntity;
    std::vector<Entity> inspectionStack;
    std::vector<Entity> lastSurvivors;

    // Mutex for thread safety
    std::mutex simMutex;

    // UI panel state
    float panelTargetX;
    float panelCurrentX;
    SDL_Rect panelParent1_rect{};
    SDL_Rect panelParent2_rect{};
    SDL_Rect panelBack_rect{};

    // Food system
    struct Food {
        int x, y;
        int radius = 4;
    };
    std::vector<Food> foods;

    // Food parameters
    const int MAX_FOOD_COUNT = 60;
    const int FOOD_SPAWN_RATE = 5;
    const int FOOD_STAMINA_GAIN = 50;

    // Private helper functions
    void initialize(int initialEntityCount);
    void triggerReproduction(const std::vector<Entity>& parents);
    void drawStatsPanel(SDL_Renderer* renderer, int panelX);
    void updateLogicAndPhysicsRange(int startIdx, int endIdx, int speedMultiplier);
    void updateProjectiles();
    void cleanupDead();
    void spawnFood();
    void updateFood(int speedMultiplier);
};

#endif //EVOARENA_SIMULATION_H
