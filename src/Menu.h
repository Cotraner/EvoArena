#ifndef EVOARENA_MENU_H
#define EVOARENA_MENU_H

#include <SDL2/SDL.h>
#include "SDL_ttf.h"
#include <string>
#include "constants.h"

// Structure pour représenter un bouton
struct Button {
    SDL_Rect rect;
    std::string text;
    bool isHovered = false;
};

class Menu {
public:
    // Définit les actions que le menu peut renvoyer
    enum MenuAction {
        NONE,
        START_SIMULATION,
        QUIT
    };

    Menu(SDL_Renderer* renderer);
    ~Menu();

    MenuAction handleEvents(const SDL_Event& event);
    void draw();

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    Button startButton;
    Button quitButton;

    // Initialisation des éléments de la SDL_ttf et du menu
    bool initializeTTF();
    void drawButton(Button& button);

    // Fonction utilitaire pour dessiner le texte au centre du bouton
    void drawText(const std::string& text, int x, int y, SDL_Color color);
};

#endif //EVOARENA_MENU_H