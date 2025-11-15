#ifndef EVOARENA_MENU_H
#define EVOARENA_MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
        QUIT,
        OPEN_SETTINGS,      // Ouvrir les paramètres
        SAVE_SETTINGS,      // Sauvegarder et revenir
        CHANGE_CELL_COUNT   // Changer le nombre de cellules
    };

    // CONSTRUCTEUR MIS À JOUR
    Menu(SDL_Renderer* renderer, SDL_Texture* backgroundTexture);
    ~Menu();

    MenuAction handleEvents(const SDL_Event& event);
    void draw(int maxEntities); // Affiche le nombre de cellules

    // État du menu
    enum ScreenState {
        MAIN_MENU,
        SETTINGS_SCREEN
    };

    void setScreenState(ScreenState state) { currentScreen = state; }
    ScreenState getCurrentScreenState() const { return currentScreen; }

    // Boutons de l'écran Settings pour le Main.cpp
    Button countUpButton;
    Button countDownButton;

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* backgroundTexture;

    ScreenState currentScreen = MAIN_MENU;

    // Boutons du Main Menu
    Button startButton;
    Button quitButton;
    Button settingsButton;

    // Boutons de l'écran Settings
    Button saveButton;

    bool initializeTTF();
    void drawButton(Button& button);
    void drawText(const std::string& text, int x, int y, SDL_Color color);

    // Fonctions de dessin spécifiques
    void drawMainMenu(int maxEntities);
    void drawSettingsScreen(int maxEntities);
};

#endif //EVOARENA_MENU_H