#ifndef TRAITMANAGER_H
#define TRAITMANAGER_H

#include <string>
#include <map>
#include <vector>

struct TraitStats {
    std::string name = "Inconnu";
    std::string description = "";

    // Valeurs par défaut (1.0 = 100% = normal)
    float speedMult = 1.0f;
    float maxStaminaMult = 1.0f;
    float radMult = 1.0f;
    float damageMult = 1.0f;
    float armorFlatBonus = 0.0f;
    float staminaMoveCostMult = 1.0f;
};

class TraitManager {
public:
    static void loadTraits(const std::string& filepath);
    static const TraitStats& get(int id);
    static int getCount() { return (int)traitDatabase.size(); }

private:
    static std::map<int, TraitStats> traitDatabase;
    static TraitStats defaultTrait;
};

#endif