#include "Graphics.h"
#include "constants.h"
#include "Menu.h"
#include "Entity/Entity.h"
#include "Entity/Projectile.h"
#include "core/Simulation.h"
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>
#include <ctime>
#include <memory>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <string>


int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

// --- DÉFINITION DE L'ÉTAT DU JEU ---
enum GameState {
    MENU,
    SIMULATION
};

enum SimRunState {
    RUNNING, // La simulation est active
    POST_COMBAT // La simulation est terminée, en attente
};

// --- Structures et Variables Globales (Panneau de Contrôle) ---
struct ControlButton {
    SDL_Rect rect;
    std::string text;
};

namespace {
    bool isPaused = false;
    int simulationSpeed = 1;
    bool showDebug = false;

    SimRunState currentSimRunState = RUNNING;
    bool autoRestart = false;

    bool isControlPanelVisible = false;
    const int CONTROL_PANEL_WIDTH = 220;
    float controlPanelCurrentX = (float)-CONTROL_PANEL_WIDTH;
    float controlPanelTargetX = (float)-CONTROL_PANEL_WIDTH;

    SDL_Texture* settingsIconTexture = nullptr;
    SDL_Rect settingsIconRect = {10, 10, 40, 40};

    ControlButton pauseButton;
    ControlButton speedButton;
    ControlButton restartButton;
    ControlButton debugButton;
    ControlButton autoRestartButton;
    ControlButton manualRestartButton;

    const SDL_Color disabledButtonColor = {40, 40, 40, 255};
    const SDL_Color disabledTextColor = {100, 100, 100, 255};

    void drawControlPanel(SDL_Renderer* renderer, int panelX, int currentGen);
}

// --- DÉCLARATION DE LA FONCTION D'INITIALISATION DE LA SIMULATION ---
std::vector<Entity> initializeSimulation(int maxEntities);

// ----------------------------------------------------------------------

int main() {
    Graphics graphics;
    if (graphics.getRenderer()) {
        SDL_GetRendererOutputSize(graphics.getRenderer(), &WINDOW_WIDTH, &WINDOW_HEIGHT);
    }
    if (WINDOW_WIDTH == 0 || WINDOW_HEIGHT == 0) {
        WINDOW_WIDTH = 1280;
        WINDOW_HEIGHT = 720;
        SDL_SetWindowSize(graphics.getWindow(), 1280, 720); // Force une taille physique
    }

    std::unique_ptr<Simulation> simulation = nullptr;

    // --- PARAMÈTRES DE JEU AJUSTABLES ---
    int maxEntities = 15;
    const int MIN_CELLS = 5;
    const int MAX_CELLS = 50;

    // --- PARAMÈTRES FIXES POUR LE PROJECTILE (RNG) ---
    const int PROJECTILE_SPEED = 8;
    const int PROJECTILE_RADIUS = 8;

    // --- INITIALISATION DU MENU ET DE L'ÉTAT ---
    Menu menu(graphics.getRenderer(), graphics.getMenuBackgroundTexture());
    GameState currentState = MENU;

    settingsIconTexture = graphics.getSettingsIconTexture();

    SDL_Event event;
    bool running = true;

    while (running) {
        if (graphics.getRenderer()) {
            SDL_GetRendererOutputSize(graphics.getRenderer(), &WINDOW_WIDTH, &WINDOW_HEIGHT);
        }

        // --- NOUVEAU : Animation du panneau de contrôle (se produit toujours) ---
        // Animation du panneau de contrôle
        controlPanelTargetX = isControlPanelVisible ? 0.0f : (float)-CONTROL_PANEL_WIDTH;
        float dist = controlPanelTargetX - controlPanelCurrentX;
        if (std::abs(dist) < 1.0f) {
            controlPanelCurrentX = controlPanelTargetX;
        } else {
            controlPanelCurrentX += dist * 0.15f;
        }

        // GESTION DES ÉVÉNEMENTS
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GetRendererOutputSize(graphics.getRenderer(), &WINDOW_WIDTH, &WINDOW_HEIGHT);
                }
            }

            // GESTION DES ÉVÉNEMENTS DU MENU
            if (currentState == MENU) {
                Menu::MenuAction action = menu.handleEvents(event);

                if (menu.getCurrentScreenState() == Menu::MAIN_MENU) {
                    if (action == Menu::START_SIMULATION) {
                        // Lancement de la simulation (assumant que le constructeur Simulation est bien défini)
                        simulation = std::make_unique<Simulation>(maxEntities);
                        isPaused = false;
                        simulationSpeed = 1;
                        showDebug = false;
                        isControlPanelVisible = false;
                        currentState = SIMULATION;
                    } else if (action == Menu::QUIT) {
                        running = false;
                    } else if (action == Menu::OPEN_SETTINGS) {
                        menu.setScreenState(Menu::SETTINGS_SCREEN);
                    }
                }
                else if (menu.getCurrentScreenState() == Menu::SETTINGS_SCREEN) {
                    if (action == Menu::SAVE_SETTINGS) {
                        menu.setScreenState(Menu::MAIN_MENU);
                    }
                    else if (action == Menu::CHANGE_CELL_COUNT) {
                        int mouseX, mouseY;
                        SDL_GetMouseState(&mouseX, &mouseY);

                        if (SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &menu.countUpButton.rect)) {
                            if (maxEntities < MAX_CELLS) maxEntities++;
                        }
                        else if (SDL_PointInRect(new SDL_Point{mouseX, mouseY}, &menu.countDownButton.rect)) {
                            if (maxEntities > MIN_CELLS) maxEntities--;
                        }
                    }
                }
            }
                // --- GESTION DES CLICS EN SIMULATION ---
            else if (currentState == SIMULATION) {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePoint = {event.button.x, event.button.y};
                    bool clickHandled = false;

                    // 1. Priorité : Clic sur l'icône (ouverture/fermeture du panneau)
                    if (SDL_PointInRect(&mousePoint, &settingsIconRect)) {
                        isControlPanelVisible = !isControlPanelVisible;
                        clickHandled = true;
                    }

                        // 2. Priorité : Clic sur le panneau de contrôle
                    else if (mousePoint.x < (int)controlPanelCurrentX + CONTROL_PANEL_WIDTH) {
                        if (SDL_PointInRect(&mousePoint, &pauseButton.rect)) {
                            isPaused = !isPaused;
                            clickHandled = true;
                        } else if (SDL_PointInRect(&mousePoint, &speedButton.rect)) {
                            // Logique de changement de vitesse
                            if (simulationSpeed == 1) simulationSpeed = 2;
                            else if (simulationSpeed == 2) simulationSpeed = 5;
                            else if (simulationSpeed == 5) simulationSpeed = 10;
                            else if (simulationSpeed == 10) simulationSpeed = 50;
                            else if (simulationSpeed == 50) simulationSpeed = 100;
                            else simulationSpeed = 1;
                            clickHandled = true;
                        } else if (SDL_PointInRect(&mousePoint, &restartButton.rect)) {
                            simulation = std::make_unique<Simulation>(maxEntities);
                            isPaused = false;
                            simulationSpeed = 1;
                            showDebug = false;
                            clickHandled = true;
                        } else if (SDL_PointInRect(&mousePoint, &debugButton.rect)) {
                            showDebug = !showDebug;
                            clickHandled = true;
                        }
                        else if (SDL_PointInRect(&mousePoint, &autoRestartButton.rect)) {
                            autoRestart = !autoRestart;
                            clickHandled = true;
                        }
                        else if (SDL_PointInRect(&mousePoint, &manualRestartButton.rect)) {
                            if (currentSimRunState == POST_COMBAT) {
                                simulation->triggerManualRestart();
                                currentSimRunState = RUNNING;
                            }
                            clickHandled = true;
                        }
                    }

                    // 3. Clic sur la simulation (pour sélectionner une entité)
                    if (!clickHandled) {
                        simulation->handleEvent(event);
                    }
                }
            }
        }

        // --- LOGIQUE D'AFFICHAGE ET DE JEU PAR ÉTAT ---

        if (currentState == MENU) {
            SDL_RenderClear(graphics.getRenderer());
            menu.draw(maxEntities);
        }
        else if (currentState == SIMULATION) {

            if (!isPaused && currentSimRunState == RUNNING) {
                for (int i = 0; i < simulationSpeed; ++i) {
                    Simulation::SimUpdateStatus status = simulation->update(simulationSpeed, autoRestart);

                    if (status == Simulation::SimUpdateStatus::FINISHED) {
                        currentSimRunState = POST_COMBAT;
                        isControlPanelVisible = true;
                        break;
                    }
                }
            }

            // RENDU
            SDL_RenderClear(graphics.getRenderer());
            graphics.drawBackground();

            simulation->render(graphics.getRenderer(), showDebug);
            int genNum = simulation ? simulation->getCurrentGeneration() : 0;

            // Dessin du panneau de contrôle
            if (controlPanelCurrentX > (float)-CONTROL_PANEL_WIDTH) {
                drawControlPanel(graphics.getRenderer(), (int)controlPanelCurrentX, genNum);
            }

            // Dessin de l'icône (toujours au-dessus)
            if (settingsIconTexture) {
                roundedBoxRGBA(graphics.getRenderer(),
                               settingsIconRect.x, settingsIconRect.y,
                               settingsIconRect.x + settingsIconRect.w, settingsIconRect.y + settingsIconRect.h,
                               8, 45, 45, 45, 255);
                SDL_RenderCopy(graphics.getRenderer(), settingsIconTexture, NULL, &settingsIconRect);
            }

            SDL_RenderPresent(graphics.getRenderer());

            SDL_Delay(16);
        }
    }

    return 0;
}

// --- DÉFINITION DE LA FONCTION D'INITIALISATION DE LA SIMULATION (Rappel) ---
// NOTE: Cette fonction n'est plus appelée directement dans main() mais par Simulation::initialize()
// Elle est incluse ici uniquement pour référence des arguments génétiques.
std::vector<Entity> initializeSimulation(int maxEntities) {
    std::vector<Entity> newEntities;
    std::srand(std::time(0));

    const int RANGED_COUNT = maxEntities / 3;

    // --- Plages pour l'initialisation aléatoire simple (tirées du JSON) ---
    const float FRAGILITY_MAX = 0.3f;
    const float EFFICIENCY_MAX = 0.5f;
    const float REGEN_MAX = 0.2f;
    const float MYOPIA_MAX = 0.5f;
    const float AIM_MAX = 15.0f;
    const int FERTILITY_MAX = 2;
    const float AGING_MAX = 0.01f;
    const float NEUTRAL_FLOAT = 0.0f;
    const int NEUTRAL_INT = 0;

    for (int i = 0; i < maxEntities; ++i) {
        int randomRad = 10 + (std::rand() % 31);
        int randomX = randomRad + (std::rand() % (WINDOW_SIZE_WIDTH - 2 * randomRad));
        int randomY = randomRad + (std::rand() % (WINDOW_SIZE_HEIGHT - 2 * randomRad));

        std::string name = "E" + std::to_string(i + 1);
        SDL_Color color = Entity::generateRandomColor();

        bool isRangedGene = (i < RANGED_COUNT) ? true : (std::rand() % 2 == 0);

        // --- NOUVEAUX GÈNES (Initialisation à Neutre / Mutation) ---
        float df = NEUTRAL_FLOAT;
        float se = NEUTRAL_FLOAT;
        float bhr = NEUTRAL_FLOAT;
        float mf = NEUTRAL_FLOAT;
        float ap = NEUTRAL_FLOAT;
        int ff = NEUTRAL_INT;
        float ar = NEUTRAL_FLOAT;

        // Logique 50% Muté
        if (std::rand() % 2 != 0) {
            int geneIndex = std::rand() % 7;
            switch (geneIndex) {
                case 0: df = (float)(std::rand() % 101) / 100.0f * FRAGILITY_MAX; break;
                case 1: se = (float)(std::rand() % 101) / 100.0f * EFFICIENCY_MAX; break;
                case 2: bhr = (float)(std::rand() % 101) / 100.0f * REGEN_MAX; break;
                case 3: mf = (float)(std::rand() % 101) / 100.0f * MYOPIA_MAX; break;
                case 4: ap = (float)(std::rand() % 101) / 100.0f * AIM_MAX; break;
                case 5: ff = (std::rand() % (FERTILITY_MAX + 1)); break;
                case 6: ar = (float)(std::rand() % 101) / 100.0f * AGING_MAX; break;
            }
        }

        // Le constructeur doit accepter ces 18 arguments
        newEntities.emplace_back(
                name, randomX, randomY, randomRad, color, isRangedGene,
                0, "Initial", "Initial",
                50, 0.5f,
                df, se, bhr, mf, ap, ff, ar
        );
    }
    return newEntities;
}

// --- DÉFINITION DE drawControlPanel (Pour la compilation) ---
namespace {
    void drawControlPanel(SDL_Renderer* renderer, int panelX, int currentGen) {
        SDL_Rect panelRect = {panelX, 0, CONTROL_PANEL_WIDTH, WINDOW_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
        SDL_RenderFillRect(renderer, &panelRect);

        // 2. Définitions du style
        int x = panelX + 10;
        int y = 60;
        int buttonHeight = 40;
        int buttonWidth = CONTROL_PANEL_WIDTH - 20;
        int spacing = 15;

        // Couleurs
        const SDL_Color textColor = {255, 255, 255, 255};
        const SDL_Color buttonColor = {80, 80, 80, 255};
        const SDL_Color titleColor = {150, 200, 255, 255};
        const SDL_Color disabledButtonColor = {40, 40, 40, 255};
        const SDL_Color disabledTextColor = {100, 100, 100, 255};


        // Fonction d'aide pour dessiner un bouton
        auto drawButton = [&](ControlButton& btn, const std::string& text, bool enabled = true) {
            btn.text = text;
            btn.rect = {x, y, buttonWidth, buttonHeight};

            // Choisir la couleur (activé/désactivé)
            SDL_Color currentBtnColor = enabled ? buttonColor : disabledButtonColor;
            SDL_Color currentTextColor = enabled ? textColor : disabledTextColor;

            // Dessin du fond du bouton
            SDL_SetRenderDrawColor(renderer, currentBtnColor.r, currentBtnColor.g, currentBtnColor.b, 255);
            SDL_RenderFillRect(renderer, &btn.rect);

            // Dessin du texte au centre
            // NOTE: stringRGBA est une fonction de faible niveau, le calcul du centrage est approximatif.
            stringRGBA(renderer, x + buttonWidth / 2 - (text.length() * 4),
                       y + buttonHeight / 2 - 5,
                       text.c_str(), currentTextColor.r, currentTextColor.g, currentTextColor.b, 255);

            y += buttonHeight + spacing; // Avance le curseur Y
        };


        // --- TITRE ET ÉTATS ---

        y += 20;
        stringRGBA(renderer, x, y, "--- Settings ---", titleColor.r, titleColor.g, titleColor.b, 255);
        y += 25;

        // 3. Dessiner les boutons (Contrôles Généraux)
        drawButton(pauseButton, isPaused ? "Play" : "Pause");
        drawButton(speedButton, "Speed: " + std::to_string(simulationSpeed) + "x");
        drawButton(debugButton, showDebug ? "Debug: ON" : "Debug: OFF");

        y += 20;
        stringRGBA(renderer, x, y, "--- Simulation ---", titleColor.r, titleColor.g, titleColor.b, 255);
        y += 25;

        // Numéro de la génération
        stringRGBA(renderer, x, y, ("Generation: " + std::to_string(currentGen)).c_str(), textColor.r, textColor.g, textColor.b, 255);
        y+=20;

        // Affichage du statut
        std::string statusText;
        if (isPaused) statusText = "Status: Paused";
        else if (currentSimRunState == RUNNING) statusText = "Status: Running...";
        else statusText = "Status: Generation Complete";
        stringRGBA(renderer, x, y, statusText.c_str(), textColor.r, textColor.g, textColor.b, 255);
        y += 25;


        // --- CONTRÔLES DE REDÉMARRAGE ---

        // Bouton Auto Restart
        std::string autoText = autoRestart ? "Auto Restart: ON" : "Auto Restart: OFF";
        drawButton(autoRestartButton, autoText);

        // Bouton Manual Relaunch (activé seulement si en POST_COMBAT)
        bool manualEnabled = (currentSimRunState == POST_COMBAT);
        drawButton(manualRestartButton, "Relaunch (Survivors)", manualEnabled);

        // Bouton Restart (Gen 0)
        y += 20; // Espacement
        drawButton(restartButton, "Restart (Gen 0)");
    }
}