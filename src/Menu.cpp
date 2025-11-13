#include "Menu.h"
#include <iostream>
#include <sstream>
#include <SDL2/SDL_ttf.h>
#include <algorithm> // Pour std::clamp


// Constantes de couleur
const SDL_Color BUTTON_COLOR = {50, 50, 50, 255};
const SDL_Color HOVER_COLOR = {80, 80, 80, 255};
const SDL_Color TEXT_COLOR = {255, 255, 255, 255};
const SDL_Color TITLE_COLOR = {100, 255, 100, 255};


// --- INITIALISATION TTF ---
bool Menu::initializeTTF() {
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    // Assurez-vous d'avoir une police accessible (ex: arial.ttf)
    font = TTF_OpenFont("../assets/font/arial_bold.ttf", 28);
    if (!font) {
        std::cerr << "Failed to load font! TTF Error: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

// --- CONSTRUCTEUR COMPLET ---
Menu::Menu(SDL_Renderer* renderer, SDL_Texture* backgroundTexture)
        : renderer(renderer), font(nullptr), backgroundTexture(backgroundTexture)
{
    if (!initializeTTF()) {
        std::cerr << "TTF initialization failed. Graphics may not work." << std::endl;
    }

    int btnWidth = 300;
    int btnHeight = 60;
    int centerX = WINDOW_SIZE_WIDTH / 2;
    int centerY = WINDOW_SIZE_HEIGHT / 2;
    int spacing = 20;
    int smallBtnWidth = 60;

    // --- MAIN MENU ---
    startButton.text = "Start Simulation";
    startButton.rect = {centerX - btnWidth / 2, centerY - btnHeight * 2 - spacing * 2, btnWidth, btnHeight};

    settingsButton.text = "Settings";
    settingsButton.rect = {centerX - btnWidth / 2, centerY - btnHeight, btnWidth, btnHeight};

    quitButton.text = "Quit";
    quitButton.rect = {centerX - btnWidth / 2, centerY + spacing, btnWidth, btnHeight};

    // --- SETTINGS SCREEN ---
    saveButton.text = "Back (Save)";
    saveButton.rect = {centerX - btnWidth / 2, WINDOW_SIZE_HEIGHT - btnHeight * 2, btnWidth, btnHeight};

    countUpButton.text = "+";
    countUpButton.rect = {centerX + 150, centerY - btnHeight, smallBtnWidth, btnHeight};

    countDownButton.text = "-";
    countDownButton.rect = {centerX - 210, centerY - btnHeight, smallBtnWidth, btnHeight};
}

Menu::~Menu() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

Menu::MenuAction Menu::handleEvents(const SDL_Event& event) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Détection du survol
    if (currentScreen == MAIN_MENU) {
        startButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &startButton.rect);
        quitButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &quitButton.rect);
        settingsButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &settingsButton.rect);

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (startButton.isHovered) return START_SIMULATION;
            if (quitButton.isHovered) return QUIT;
            if (settingsButton.isHovered) return OPEN_SETTINGS;
        }
    }
    else if (currentScreen == SETTINGS_SCREEN) {
        saveButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &saveButton.rect);
        countUpButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &countUpButton.rect);
        countDownButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &countDownButton.rect);

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (saveButton.isHovered) return SAVE_SETTINGS;
            if (countUpButton.isHovered) return CHANGE_CELL_COUNT;
            if (countDownButton.isHovered) return CHANGE_CELL_COUNT;
        }
    }
    return NONE;
}

void Menu::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface) return;

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textW = textSurface->w;
    int textH = textSurface->h;

    SDL_FreeSurface(textSurface);

    SDL_Rect renderQuad = {x - textW / 2, y - textH / 2, textW, textH};
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

void Menu::drawButton(Button& button) {
    SDL_Color color;

    // Utiliser if/else pour l'affectation de SDL_Color
    if (button.isHovered) {
        color = HOVER_COLOR;
    } else {
        color = BUTTON_COLOR;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &button.rect);

    // Dessiner le texte au centre du bouton
    int textX = button.rect.x + button.rect.w / 2;
    int textY = button.rect.y + button.rect.h / 2;
    drawText(button.text, textX, textY, TEXT_COLOR);
}

void Menu::drawMainMenu(int maxEntities) {
    // Dessiner le titre
    drawText("EvoArena - Genetic Simulation", WINDOW_SIZE_WIDTH / 2, 100, TITLE_COLOR);

    // Afficher le nombre de cellules actuel
    std::string countStr = "Population: " + std::to_string(maxEntities);
    drawText(countStr, WINDOW_SIZE_WIDTH / 2, 250, TEXT_COLOR);

    // Dessiner les boutons
    drawButton(startButton);
    drawButton(settingsButton);
    drawButton(quitButton);
}

void Menu::drawSettingsScreen(int maxEntities) {
    drawText("SETTINGS", WINDOW_SIZE_WIDTH / 2, 100, TITLE_COLOR);

    // Texte de l'option
    drawText("Initial Population Size:", WINDOW_SIZE_WIDTH / 2, 250, TEXT_COLOR);

    // Afficher la valeur actuelle
    std::string countStr = std::to_string(maxEntities);
    drawText(countStr, WINDOW_SIZE_WIDTH / 2, 350, TEXT_COLOR);

    // Boutons de contrôle
    drawButton(countUpButton);
    drawButton(countDownButton);

    // Bouton de sauvegarde/retour
    drawButton(saveButton);
}


void Menu::draw(int maxEntities) {
    // 1. Dessiner le fond du menu
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    if (currentScreen == MAIN_MENU) {
        drawMainMenu(maxEntities);
    } else if (currentScreen == SETTINGS_SCREEN) {
        drawSettingsScreen(maxEntities);
    }

    SDL_RenderPresent(renderer);
}