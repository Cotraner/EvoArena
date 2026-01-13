# EvoArena - Simulation Génétique de Combat

**EvoArena** est une simulation de vie artificielle en 2D basée sur des algorithmes génétiques, développée en C++ avec la bibliothèque SDL2. Le projet simule l'évolution naturelle d'entités autonomes ("cellules") qui doivent combattre, se nourrir et survivre pour transmettre leurs gènes à la génération suivante.

![Status](https://img.shields.io/badge/Status-Development-yellow)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Library](https://img.shields.io/badge/Library-SDL2-red)
![Thread](https://img.shields.io/badge/Tech-Multithreading-green)

## 📋 Fonctionnalités

### Moteur Génétique
* **Évolution Darwinienne :** Sélection naturelle, croisement (crossover) et mutations aléatoires à chaque nouvelle génération.
* **ADN Complexe :** Chaque entité possède un code génétique de 14 paramètres influençant sa taille, sa vitesse, son armement, sa vision et son comportement.
* **Système de Traits :** Mutations spéciales (Gourmand, Robuste, Myope, etc.) gérées via un fichier de configuration JSON.

### Gameplay & Simulation
* **Classes Dynamiques :** Évolution vers des archétypes distincts : Mêlée (Tank), Distance (DPS), et Soigneur (Healer/Support).
* **Comportements (IA) :** Machine à états finis (FSM) gérant la fuite, la chasse, la recherche de nourriture et le combat.
* **Gestion de l'Énergie :** Système de Stamina et besoin de nourriture pour les entités actives.

### Interface & Technique
* **Moteur Graphique SDL2 :** Rendu 2D performant avec gestion de caméra (Zoom/Déplacement).
* **Multithreading (Nouveau) :** Architecture parallèle utilisant tous les cœurs du processeur pour gérer la logique et la physique de centaines d'entités simultanément.
* **HUD Détaillé :** Inspection en temps réel des statistiques, de la généalogie et de l'état mental des entités.
* **Audio :** Gestion de la musique d'ambiance et des effets sonores (SFX) via SDL_Mixer.

## ⚙️ Architecture Technique

### Optimisation Multithreading
Pour garantir la fluidité de la simulation avec un grand nombre d'entités, le moteur utilise une approche **Fork-Join** :
1.  **Détection Hardware :** Le jeu détecte automatiquement le nombre de cœurs disponibles (`std::thread::hardware_concurrency`).
2.  **Parallélisation :** À chaque frame, la mise à jour des entités (IA + Physique) est découpée en "chunks" et distribuée sur plusieurs threads (`std::thread`).
3.  **Sûreté (Thread-Safety) :** Utilisation de `std::mutex` pour protéger les sections critiques (écriture dans le vecteur de projectiles, résolution des collisions concurrentes), évitant ainsi les *Data Races*.

## 🛠️ Prérequis

Pour compiler ce projet, vous aurez besoin des bibliothèques suivantes :

* **Compilateur C++** (supportant C++17 et `<thread>`)
* **CMake** (version 3.10 ou supérieure)
* **SDL2** (Core)
* **SDL2_image**
* **SDL2_ttf**
* **SDL2_mixer**
* **SDL2_gfx**
* **nlohmann_json** (souvent inclus en header-only)

## 🚀 Installation et Compilation

### Sous Linux (Debian/Ubuntu)

1.  **Installer les dépendances :**
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-gfx-dev nlohmann-json3-dev
    ```

2.  **Cloner et compiler :**
    ```bash
    git clone https://votre-repo/EvoArena.git
    cd EvoArena
    mkdir build && cd build
    cmake ..
    make
    ```

3.  **Lancer :**
    ```bash
    ./EvoArena
    ```

### Sous Windows
Il est recommandé d'utiliser un gestionnaire de paquets comme `vcpkg` ou `MSYS2` pour installer les dépendances SDL2, ou de configurer votre IDE (Visual Studio / CLion) avec les bibliothèques liées.

## 🎮 Contrôles

* **Souris (Gauche) :** Sélectionner une entité / Interagir avec l'UI.
* **Souris (Droit + Glisser) :** Déplacer la caméra.
* **Molette :** Zoom Avant / Arrière.
* **Interface :** Utilisez le panneau latéral pour voir les stats ou le bouton "Settings" pour changer la vitesse de simulation.

## 📂 Structure du Projet

* `src/core/` : Gestion de la boucle de simulation multithreadée (`Simulation.cpp`).
* `src/Entity/` : Logique des entités, projectiles et gestion des traits (`Entity.cpp`, `TraitManager.cpp`).
* `src/Graphics.cpp` : Gestion du rendu SDL et de l'audio.
* `src/Menu.cpp` : Gestion des menus et de l'interface utilisateur.
* `assets/` : Contient les ressources (Images, Sons, JSON, Polices).

## 👥 Developpeurs

* **Maxime You** - *FISA 3*
* **Clément Robin** - *FISA 3*

---
*ISEN Yncréa Ouest.*