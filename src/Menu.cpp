#include "Menu.h"
#include <iostream>

// Constantes de couleur pour le menu
#define BUTTON_COLOR {50, 50, 50, 255}
#define HOVER_COLOR {80, 80, 80, 255}
#define TEXT_COLOR {255, 255, 255, 255}

Menu::Menu(SDL_Renderer* renderer) : renderer(renderer), font(nullptr) {
    if (!initializeTTF()) {
        std::cerr << "TTF initialization failed. Graphics may not work." << std::endl;
    }

    int btnWidth = 300;
    int btnHeight = 60;
    int centerX = WINDOW_SIZE_WIDTH / 2;
    int centerY = WINDOW_SIZE_HEIGHT / 2;
    int spacing = 20;

    // Bouton START
    startButton.text = "Start Simulation";
    startButton.rect = {centerX - btnWidth / 2, centerY - btnHeight - spacing, btnWidth, btnHeight};

    // Bouton QUIT
    quitButton.text = "Quit";
    quitButton.rect = {centerX - btnWidth / 2, centerY + spacing, btnWidth, btnHeight};
}

Menu::~Menu() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

bool Menu::initializeTTF() {
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    // Chargez une police. Assurez-vous d'avoir un fichier de police (ex: Arial.ttf) dans le chemin d'accès.
    // REMPLACER "path/to/your/font.ttf" par un chemin valide ou inclure une police dans le projet.
    // Pour l'exemple, nous allons essayer une police par défaut ou simple si elle existe.
    font = TTF_OpenFont("arial.ttf", 28);
    if (!font) {
        std::cerr << "Failed to load font! TTF Error: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

Menu::MenuAction Menu::handleEvents(const SDL_Event& event) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    startButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &startButton.rect);
    quitButton.isHovered = SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &quitButton.rect);

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            if (startButton.isHovered) {
                return START_SIMULATION;
            }
            if (quitButton.isHovered) {
                return QUIT;
            }
        }
    }
    return NONE;
}

void Menu::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! TTF Error: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
    }

    int textW = textSurface->w;
    int textH = textSurface->h;
    SDL_FreeSurface(textSurface);

    // Positionner le texte
    SDL_Rect renderQuad = {x - textW / 2, y - textH / 2, textW, textH};
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

void Menu::drawButton(Button& button) {
    SDL_Color color;

    // Remplacer l'opérateur ternaire par un if/else standard pour l'affectation de la structure SDL_Color
    if (button.isHovered) {
        color = HOVER_COLOR;
    } else {
        color = BUTTON_COLOR;
    }

    // Choisir la couleur du bouton
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &button.rect);

    // Dessiner le texte au centre du bouton
    int textX = button.rect.x + button.rect.w / 2;
    int textY = button.rect.y + button.rect.h / 2;
    drawText(button.text, textX, textY, TEXT_COLOR);
}

void Menu::draw() {
    // Nettoyer l'écran (avec un fond sombre pour le menu)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Dessiner le titre
    drawText("EvoArena - Genetic Simulation", WINDOW_SIZE_WIDTH / 2, 100, TEXT_COLOR);

    // Dessiner les boutons
    drawButton(startButton);
    drawButton(quitButton);

    SDL_RenderPresent(renderer);
}