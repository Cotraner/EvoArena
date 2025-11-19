#include "Menu.h"
#include <iostream>
#include <SDL2/SDL_ttf.h>
#include <algorithm>

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
    font = TTF_OpenFont("../assets/font/arial_bold.ttf", 28);
    if (!font) {
        std::cerr << "Failed to load font! TTF Error: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

// --- CONSTRUCTEUR (Nettoyé) ---
Menu::Menu(SDL_Renderer* renderer, SDL_Texture* backgroundTexture)
        : renderer(renderer), font(nullptr), backgroundTexture(backgroundTexture)
{
    if (!initializeTTF()) {
        std::cerr << "TTF initialization failed. Graphics may not work." << std::endl;
    }

    // On définit juste le texte ici.
    // Les positions (.rect) seront définies dans updateLayout() appelé par draw().
    startButton.text = "Start Simulation";
    settingsButton.text = "Settings";
    quitButton.text = "Quit";
    saveButton.text = "Back (Save)";
    countUpButton.text = "+";
    countDownButton.text = "-";
}

Menu::~Menu() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

// --- NOUVEAU : Recalcule les positions en fonction de la taille actuelle de la fenêtre ---
void Menu::updateLayout() {
    int btnWidth = 300;
    int btnHeight = 60;

    // ON RÉCUPÈRE LA TAILLE RÉELLE DE LA FENÊTRE ICI
    int w, h;
    if (renderer) {
        SDL_GetRendererOutputSize(renderer, &w, &h);
    } else {
        w = 1280; // Valeur de secours
        h = 720;
    }

    int centerX = w / 2;
    int centerY = h / 2;

    // Pour le bouton Save en bas, on utilise 'h' (la hauteur réelle)
    int bottomY = h - btnHeight * 2;

    int spacing = 20;
    int smallBtnWidth = 60;

    // --- MAIN MENU ---
    startButton.rect = {centerX - btnWidth / 2, centerY - btnHeight * 2 - spacing * 2, btnWidth, btnHeight};
    settingsButton.rect = {centerX - btnWidth / 2, centerY - btnHeight, btnWidth, btnHeight};
    quitButton.rect = {centerX - btnWidth / 2, centerY + spacing, btnWidth, btnHeight};

    // --- SETTINGS SCREEN ---
    // On utilise bottomY calculé avec la vraie hauteur
    saveButton.rect = {centerX - btnWidth / 2, bottomY, btnWidth, btnHeight};
    countUpButton.rect = {centerX + 150, centerY - btnHeight, smallBtnWidth, btnHeight};
    countDownButton.rect = {centerX - 210, centerY - btnHeight, smallBtnWidth, btnHeight};
}

Menu::MenuAction Menu::handleEvents(const SDL_Event& event) {
    // IMPORTANT : On s'assure que les rectangles sont à jour avant de tester les clics
    // (Au cas où un événement arrive avant le premier draw)
    updateLayout();

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

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

// ... (drawText et drawButton restent inchangés) ...
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
    if (button.isHovered) {
        color = HOVER_COLOR;
    } else {
        color = BUTTON_COLOR;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &button.rect);
    int textX = button.rect.x + button.rect.w / 2;
    int textY = button.rect.y + button.rect.h / 2;
    drawText(button.text, textX, textY, TEXT_COLOR);
}
// ...

void Menu::drawMainMenu(int maxEntities) {
    // Récupérer la largeur pour centrer le texte
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    drawText("EvoArena - Genetic Simulation", w / 2, 100, TITLE_COLOR);

    std::string countStr = "Population: " + std::to_string(maxEntities);
    drawText(countStr, w / 2, 250, TEXT_COLOR);

    drawButton(startButton);
    drawButton(settingsButton);
    drawButton(quitButton);
}

void Menu::drawSettingsScreen(int maxEntities) {
    // Récupérer la largeur pour centrer le texte
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    drawText("SETTINGS", w / 2, 100, TITLE_COLOR);
    drawText("Initial Population Size:", w / 2, 250, TEXT_COLOR);
    std::string countStr = std::to_string(maxEntities);
    drawText(countStr, w / 2, 350, TEXT_COLOR);

    drawButton(countUpButton);
    drawButton(countDownButton);
    drawButton(saveButton);
}

void Menu::draw(int maxEntities) {
    // --- IMPORTANT : On recalcule les positions avant de dessiner ---
    updateLayout();

    // 1. Dessiner le fond du menu (étiré à la taille de l'écran)
    if (backgroundTexture) {
        // On passe NULL pour le rect de destination pour qu'il remplisse TOUT l'écran
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