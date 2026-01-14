#ifndef EVOARENA_CONSTANTS_H
#define EVOARENA_CONSTANTS_H

// Window dimensions
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

// World dimensions
extern int WORLD_WIDTH;
extern int WORLD_HEIGHT;

// Camera structure
struct Camera {
    float x = 0.0f;   // Camera X position
    float y = 0.0f;   // Camera Y position
    float zoom = 1.0f; // Camera zoom level
};

#endif //EVOARENA_CONSTANTS_H