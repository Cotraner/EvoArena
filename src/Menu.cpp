#include "Menu.h"
#include <iostream>
#include <SDL2/SDL_ttf.h>
#include <algorithm>

// Define button and text colors
const SDL_Color BUTTON_COLOR = {50, 50, 50, 255};
const SDL_Color HOVER_COLOR = {80, 80, 80, 255};
const SDL_Color TEXT_COLOR = {255, 255, 255, 255};
const SDL_Color TITLE_COLOR = {100, 255, 100, 255};

// Initialize the TTF font system
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

// Constructor: Initialize menu buttons and load font
Menu::Menu(SDL_Renderer *renderer, SDL_Texture *backgroundTexture)
        : renderer(renderer), font(nullptr), backgroundTexture(backgroundTexture) {
    if (!initializeTTF()) {
        std::cerr << "TTF initialization failed. Graphics may not work." << std::endl;
    }

    // Define button texts
    startButton.text = "Start Simulation";
    settingsButton.text = "Settings";
    quitButton.text = "Quit";
    saveButton.text = "Back (Save)";
    countUpButton.text = "+";
    countDownButton.text = "-";
}

// Destructor: Clean up font resources
Menu::~Menu() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

// Update button positions based on the current window size
void Menu::updateLayout() {
    int btnWidth = 300;
    int btnHeight = 60;

    int w, h;
    if (renderer) {
        SDL_GetRendererOutputSize(renderer, &w, &h);
    } else {
        w = 1280; // Default width
        h = 720;  // Default height
    }

    int centerX = w / 2;
    int centerY = h / 2;

    int bottomY = h - btnHeight * 2;
    int spacing = 20;
    int smallBtnWidth = 60;

    // Main menu button positions
    startButton.rect = {centerX - btnWidth / 2, centerY - btnHeight * 2 - spacing * 2, btnWidth, btnHeight};
    settingsButton.rect = {centerX - btnWidth / 2, centerY - btnHeight, btnWidth, btnHeight};
    quitButton.rect = {centerX - btnWidth / 2, centerY + spacing, btnWidth, btnHeight};

    // Settings screen button positions
    saveButton.rect = {centerX - btnWidth / 2, bottomY, btnWidth, btnHeight};
    countUpButton.rect = {centerX + 150, centerY - btnHeight, smallBtnWidth, btnHeight};
    countDownButton.rect = {centerX - 210, centerY - btnHeight, smallBtnWidth, btnHeight};
}

// Handle user input events for the menu
Menu::MenuAction Menu::handleEvents(const SDL_Event &event) {
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
    } else if (currentScreen == SETTINGS_SCREEN) {
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

// Draw text on the screen
void Menu::drawText(const std::string &text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface) return;
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textW = textSurface->w;
    int textH = textSurface->h;
    SDL_FreeSurface(textSurface);
    SDL_Rect renderQuad = {x - textW / 2, y - textH / 2, textW, textH};
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

// Draw a button with its text
void Menu::drawButton(Button &button) {
    SDL_Color color = button.isHovered ? HOVER_COLOR : BUTTON_COLOR;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &button.rect);
    int textX = button.rect.x + button.rect.w / 2;
    int textY = button.rect.y + button.rect.h / 2;
    drawText(button.text, textX, textY, TEXT_COLOR);
}

// Draw the main menu screen
void Menu::drawMainMenu(int maxEntities) {
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    drawText("EvoArena - Genetic Simulation", w / 2, 100, TITLE_COLOR);

    std::string countStr = "Population: " + std::to_string(maxEntities);
    drawText(countStr, w / 2, 250, TEXT_COLOR);

    drawButton(startButton);
    drawButton(settingsButton);
    drawButton(quitButton);
}

// Draw the settings screen
void Menu::drawSettingsScreen(int maxEntities) {
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

// Draw the current menu screen
void Menu::draw(int maxEntities) {
    updateLayout();

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