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
    float staminaRegenBonus = 0.0f;

    // --- NOUVELLES STATS AJOUTÉES POUR L'ÉQUILIBRAGE ---
    float maxHealthMult = 1.0f;      // Pour Fragile/Obese (ex: 0.8)
    float fertilityMult = 1.0f;      // Pour Fertile (ex: 2.0)
    float damageTakenMult = 1.0f;    // Pour Fragile (ex: 1.5)
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