#ifndef EVOARENA_MENU_H
#define EVOARENA_MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "constants.h"

// Represents a button
struct Button {
    SDL_Rect rect;
    std::string text;
    bool isHovered = false;
};

class Menu {
public:
    // Menu actions
    enum MenuAction {
        NONE,
        START_SIMULATION,
        QUIT,
        OPEN_SETTINGS,
        SAVE_SETTINGS,
        CHANGE_CELL_COUNT
    };

    Menu(SDL_Renderer *renderer, SDL_Texture *backgroundTexture);

    ~Menu();

    MenuAction handleEvents(const SDL_Event &event); // Handle user input
    void draw(int maxEntities); // Draw the menu
    void updateLayout(); // Update button layout

    // Menu screen states
    enum ScreenState {
        MAIN_MENU,
        SETTINGS_SCREEN
    };

    void setScreenState(ScreenState state); // Set current screen
    ScreenState getCurrentScreenState() const; // Get current screen

    // Buttons for settings screen
    Button countUpButton;
    Button countDownButton;

private:
    SDL_Renderer *renderer;
    TTF_Font *font;
    SDL_Texture *backgroundTexture;

    ScreenState currentScreen = MAIN_MENU;

    // Buttons for main menu
    Button startButton;
    Button quitButton;
    Button settingsButton;

    // Button for settings screen
    Button saveButton;

    bool initializeTTF(); // Initialize font system
    void drawButton(Button &button); // Draw a button
    void drawText(const std::string &text, int x, int y, SDL_Color color); // Draw text

    // Specific drawing functions
    void drawMainMenu(int maxEntities); // Draw main menu
    void drawSettingsScreen(int maxEntities); // Draw settings screen
};

#endif //EVOARENA_MENU_H