#ifndef EVOARENA_CONSTANTS_H
#define EVOARENA_CONSTANTS_H

// Dimensions de la fenêtre (Viewport)
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

// Dimensions du monde simulé (La carte agrandie)
extern int WORLD_WIDTH;
extern int WORLD_HEIGHT;

struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.0f;
};

#endif //EVOARENA_CONSTANTS_H