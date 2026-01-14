#ifndef TRAITMANAGER_H
    #define TRAITMANAGER_H

    #include <string>
    #include <map>
    #include <vector>

    // Represents the statistics associated with a trait
    struct TraitStats {
        std::string name = "Unknown";          // Trait name
        std::string description = "";          // Trait description

        // Multipliers and bonuses for various attributes
        float speedMult = 1.0f;                // Speed multiplier
        float maxStaminaMult = 1.0f;           // Maximum stamina multiplier
        float radMult = 1.0f;                  // Radius multiplier
        float damageMult = 1.0f;               // Damage multiplier
        float armorFlatBonus = 0.0f;           // Flat armor bonus
        float staminaMoveCostMult = 1.0f;      // Stamina cost multiplier for movement
        float staminaRegenBonus = 0.0f;        // Bonus to stamina regeneration

        // Additional stats for balancing
        float maxHealthMult = 1.0f;            // Maximum health multiplier
        float fertilityMult = 1.0f;            // Fertility multiplier
        float damageTakenMult = 1.0f;          // Damage taken multiplier
    };

    // Manages the loading and retrieval of traits
    class TraitManager {
    public:
        static void loadTraits(const std::string& filepath); // Load traits from a file
        static const TraitStats& get(int id);               // Retrieve a trait by ID
        static int getCount() { return (int)traitDatabase.size(); } // Get the total number of traits

    private:
        static std::map<int, TraitStats> traitDatabase;     // Database of traits
        static TraitStats defaultTrait;                    // Default trait for fallback
    };

    #endif