#include "TraitManager.h"
    #include <fstream>
    #include <iostream>
    #include <nlohmann/json.hpp>

    using json = nlohmann::json;

    // Static member initialization
    std::map<int, TraitStats> TraitManager::traitDatabase;
    TraitStats TraitManager::defaultTrait;

    // Load traits from a JSON file
    void TraitManager::loadTraits(const std::string& filepath) {
        traitDatabase.clear();

        // Add a default "Classique" trait (ID 0) manually, as it may not exist in the JSON file
        TraitStats classique;
        classique.name = "Classique";
        classique.description = "Neutral";
        traitDatabase[0] = classique;

        // Open the JSON file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Unable to open " << filepath << std::endl;
            return;
        }

        json jsonData;
        try {
            file >> jsonData; // Parse the JSON file
        } catch (const json::parse_error& e) {
            std::cerr << "[JSON ERROR] " << e.what() << std::endl;
            return;
        }

        // Iterate through the JSON data and populate the trait database
        for (auto& [key, value] : jsonData.items()) {
            int id = std::stoi(key);
            TraitStats stats;

            // Load basic trait information
            stats.name = value.value("name", "Unknown");
            stats.description = value.value("description", "");

            // Load trait statistics if available
            if (value.contains("stats")) {
                auto s = value["stats"];
                stats.speedMult = s.value("speedMult", 1.0f);
                stats.maxStaminaMult = s.value("maxStaminaMult", 1.0f);
                stats.radMult = s.value("radMult", 1.0f);
                stats.damageMult = s.value("damageMult", 1.0f);
                stats.armorFlatBonus = s.value("armorFlatBonus", 0.0f);
                stats.staminaMoveCostMult = s.value("staminaMoveCostMult", 1.0f);
                stats.staminaRegenBonus = s.value("staminaRegenBonus", 0.0f);
            }

            // Add the trait to the database
            traitDatabase[id] = stats;
            std::cout << "[TraitManager] Loaded ID " << id << ": " << stats.name << std::endl;
        }
    }

    // Retrieve a trait by its ID, or return the default trait if not found
    const TraitStats& TraitManager::get(int id) {
        if (traitDatabase.find(id) != traitDatabase.end()) {
            return traitDatabase[id];
        }
        return defaultTrait;
    }