#include "Graphics.h"
#include "constants.h"
#include "Menu.h"
#include "core/Simulation.h"
#include <iostream>
#include <vector>
#include <map>
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

// --- NOUVEAU : Structure simple pour les boutons du panneau ---
struct ControlButton {
    SDL_Rect rect;
    std::string text;
};

// --- NOUVEAU : Variables globales pour le panneau de contrôle ---
namespace {
    // État de la simulation
    bool isPaused = false;
    int simulationSpeed = 1; // 1, 2, 5 ou 10
    bool showDebug = false; // Pour les rayons de vision

    SimRunState currentSimRunState = RUNNING;
    bool autoRestart = false; // Le "auto start"

    // État du panneau
    bool isControlPanelVisible = false;
    const int CONTROL_PANEL_WIDTH = 220;
    float controlPanelCurrentX = (float)-CONTROL_PANEL_WIDTH;
    float controlPanelTargetX = (float)-CONTROL_PANEL_WIDTH;

    // Ressources du panneau
    SDL_Texture* settingsIconTexture = nullptr;
    SDL_Rect settingsIconRect = {10, 10, 40, 40}; // Position de l'icône

    // Déclarations des boutons (les rects seront mis à jour dynamiquement)
    ControlButton pauseButton;
    ControlButton speedButton;
    ControlButton restartButton;
    ControlButton debugButton;
    ControlButton autoRestartButton;
    ControlButton manualRestartButton;

    const SDL_Color disabledButtonColor = {40, 40, 40, 255};
    const SDL_Color disabledTextColor = {100, 100, 100, 255};

    // Fonction d'aide pour dessiner le panneau
    void drawControlPanel(SDL_Renderer* renderer, int panelX, int currentGen);
}


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

    // --- INITIALISATION DU MENU ET DE L'ÉTAT ---
    Menu menu(graphics.getRenderer(), graphics.getMenuBackgroundTexture());
    GameState currentState = MENU;

    // --- NOUVEAU : Récupérer l'icône ---
    settingsIconTexture = graphics.getSettingsIconTexture();
    if (!settingsIconTexture) {
        std::cerr << "Échec du chargement de la texture de l'icône !" << std::endl;
    }

    SDL_Event event;
    bool running = true;

    while (running) {
        if (graphics.getRenderer()) {
            SDL_GetRendererOutputSize(graphics.getRenderer(), &WINDOW_WIDTH, &WINDOW_HEIGHT);
        }

        // --- NOUVEAU : Animation du panneau de contrôle (se produit toujours) ---
        controlPanelTargetX = isControlPanelVisible ? 0.0f : (float)-CONTROL_PANEL_WIDTH;
        float dist = controlPanelTargetX - controlPanelCurrentX;
        if (std::abs(dist) < 1.0f) {
            controlPanelCurrentX = controlPanelTargetX;
        } else {
            controlPanelCurrentX += dist * 0.15f; // Glissement un peu plus rapide
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
                        simulation = std::make_unique<Simulation>(maxEntities);
                        // Réinitialiser les états au cas où
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
                    // ... (logique settings inchangée) ...
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
                // --- NOUVEAU : GESTION DES CLICS EN SIMULATION (Panneau + Entités) ---
            else if (currentState == SIMULATION) {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePoint = {event.button.x, event.button.y};
                    bool clickHandled = false;

                    // 1. Priorité : Clic sur l'icône
                    if (SDL_PointInRect(&mousePoint, &settingsIconRect)) {
                        isControlPanelVisible = !isControlPanelVisible;
                        clickHandled = true;
                    }

                        // 2. Priorité : Clic sur le panneau (s'il est visible)
                    else if (mousePoint.x < (int)controlPanelCurrentX + CONTROL_PANEL_WIDTH) {
                        // (Les rects des boutons ont été mis à jour par drawControlPanel au frame précédent)
                        if (SDL_PointInRect(&mousePoint, &pauseButton.rect)) {
                            isPaused = !isPaused;
                            clickHandled = true;
                        } else if (SDL_PointInRect(&mousePoint, &speedButton.rect)) {
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
                            // Ne fonctionne que si la simulation est arrêtée
                            if (currentSimRunState == POST_COMBAT) {
                                simulation->triggerManualRestart(); // Demande à la Sim de se relancer
                                currentSimRunState = RUNNING; // Repasse en mode RUNNING
                            }
                            clickHandled = true;
                        }
                    }

                    // 3. Priorité : Clic sur la simulation
                    if (!clickHandled) {
                        simulation->handleEvent(event);
                    }
                }
            }
        }

        // --- LOGIQUE D'AFFICHAGE ET DE JEU PAR ÉTAT ---

        if (currentState == MENU) {
            // Nettoyer l'écran
            SDL_RenderClear(graphics.getRenderer());
            menu.draw(maxEntities);
        }
        else if (currentState == SIMULATION) {

            if (!isPaused && currentSimRunState == RUNNING) {
                for (int i = 0; i < simulationSpeed; ++i) {
                    // Passe l'état 'autoRestart' à la simulation
                    Simulation::SimUpdateStatus status = simulation->update(simulationSpeed, autoRestart);

                    // Si la simulation dit qu'elle a fini
                    if (status == Simulation::SimUpdateStatus::FINISHED) {
                        currentSimRunState = POST_COMBAT; // Arrêter la boucle d'update
                        isControlPanelVisible = true; // Ouvrir le panneau
                        break; // Sortir de la boucle for (vitesse)
                    }
                }
            }

            // Le rendu se fait toujours, même en pause
            SDL_RenderClear(graphics.getRenderer());
            graphics.drawBackground();

            // --- MODIFIÉ : Passe l'état de debug ---
            simulation->render(graphics.getRenderer(), showDebug);
            int genNum = simulation ? simulation->getCurrentGeneration() : 0;
            if (controlPanelCurrentX > (float)-CONTROL_PANEL_WIDTH) {
                drawControlPanel(graphics.getRenderer(), (int)controlPanelCurrentX, genNum);
            }

            // --- NOUVEAU : Dessin de l'icône (toujours au-dessus) ---
            if (settingsIconTexture) {
                // 1. Dessiner le fond arrondi plus foncé
                roundedBoxRGBA(graphics.getRenderer(),
                               settingsIconRect.x, settingsIconRect.y,
                               settingsIconRect.x + settingsIconRect.w, settingsIconRect.y + settingsIconRect.h,
                               8, // Rayon d'arrondi
                               45, 45, 45, 255); // Couleur foncée

                // 2. Dessiner l'icône par-dessus
                SDL_RenderCopy(graphics.getRenderer(), settingsIconTexture, NULL, &settingsIconRect);
            }

            // Afficher le rendu
            SDL_RenderPresent(graphics.getRenderer());

            SDL_Delay(16);
        }
    }

    return 0;
}

namespace {
    void drawControlPanel(SDL_Renderer* renderer, int panelX, int currentGen) {
        SDL_Rect panelRect = {panelX, 0, CONTROL_PANEL_WIDTH, WINDOW_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
        SDL_RenderFillRect(renderer, &panelRect);

        // 2. Définitions
        int x = panelX + 10;
        int y = 60;
        int buttonHeight = 40;
        int buttonWidth = CONTROL_PANEL_WIDTH - 20;
        int spacing = 15;
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Color buttonColor = {80, 80, 80, 255};
        SDL_Color titleColor = {150, 200, 255, 255};

        // Fonction d'aide pour dessiner un bouton
        auto drawButton = [&](ControlButton& btn, const std::string& text, bool enabled = true) {
            btn.text = text;
            btn.rect = {x, y, buttonWidth, buttonHeight};

            // Choisir la couleur (activé/désactivé)
            SDL_Color currentBtnColor = enabled ? buttonColor : disabledButtonColor;
            SDL_Color currentTextColor = enabled ? textColor : disabledTextColor;

            SDL_SetRenderDrawColor(renderer, currentBtnColor.r, currentBtnColor.g, currentBtnColor.b, 255);
            SDL_RenderFillRect(renderer, &btn.rect);

            stringRGBA(renderer, x + buttonWidth / 2 - (text.length() * 4),
                       y + buttonHeight / 2 - 5,
                       text.c_str(), currentTextColor.r, currentTextColor.g, currentTextColor.b, 255);

            y += buttonHeight + spacing;
        };
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
        //Numéro de la génération
        stringRGBA(renderer, x, y, ("Generation: " + std::to_string(currentGen)).c_str(), textColor.r, textColor.g, textColor.b, 255);
        y+=20;
        std::string statusText;
        if (isPaused) statusText = "Status: Paused";
        else if (currentSimRunState == RUNNING) statusText = "Status: Running...";
        else statusText = "Status: Generation Complete";
        stringRGBA(renderer, x, y, statusText.c_str(), textColor.r, textColor.g, textColor.b, 255);
        y += 25;


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