#include "TraitManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::map<int, TraitStats> TraitManager::traitDatabase;
TraitStats TraitManager::defaultTrait;

void TraitManager::loadTraits(const std::string& filepath) {
    traitDatabase.clear();

    // Ajout manuel du trait Classique (ID 0) car il n'est pas forcément dans le JSON
    TraitStats classique;
    classique.name = "Classique";
    classique.description = "Neutre";
    traitDatabase[0] = classique;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[ERREUR] Impossible d'ouvrir " << filepath << std::endl;
        return;
    }

    json jsonData;
    try {
        file >> jsonData;
    } catch (const json::parse_error& e) {
        std::cerr << "[ERREUR JSON] " << e.what() << std::endl;
        return;
    }

    for (auto& [key, value] : jsonData.items()) {
        int id = std::stoi(key);
        TraitStats stats;

        stats.name = value.value("name", "Inconnu");
        stats.description = value.value("description", "");

        if (value.contains("stats")) {
            auto s = value["stats"];
            stats.speedMult = s.value("speedMult", 1.0f);
            stats.maxStaminaMult = s.value("maxStaminaMult", 1.0f);
            stats.radMult = s.value("radMult", 1.0f);
            stats.damageMult = s.value("damageMult", 1.0f);
            stats.armorFlatBonus = s.value("armorFlatBonus", 0.0f);
            stats.staminaMoveCostMult = s.value("staminaMoveCostMult", 1.0f);
        }

        traitDatabase[id] = stats;
        std::cout << "[TraitManager] Charge ID " << id << ": " << stats.name << std::endl;
    }
}

const TraitStats& TraitManager::get(int id) {
    if (traitDatabase.find(id) != traitDatabase.end()) {
        return traitDatabase[id];
    }
    return defaultTrait;
}