#pragma once

#include "Enums.h"
#include <string>
#include <vector>
#include <cmath>

namespace TLS {

// Use EventType from Enums.h instead of redefining

class Event {
public:
    Event() : id_(-1), name_(""), type_(EventType::ENVIRONMENTAL), impact_level_(0), cascadeLevel_(0), parentEventId_(-1) {}
    
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    EventType getType() const { return type_; }
    int getImpactLevel() const { return impact_level_; }
    const std::vector<int>& getAffectedNpcIds() const { return affected_npc_ids_; }
    
    void setId(int id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setType(EventType type) { type_ = type; }
    void setImpactLevel(int level) { impact_level_ = level; }
    void addAffectedNpc(int npc_id) { affected_npc_ids_.push_back(npc_id); }
    
    // ========================================================================
    // Cascade System (Task #3)
    // ========================================================================
    
    // Cascade tracking
    int getCascadeLevel() const { return cascadeLevel_; }
    void setCascadeLevel(int level) { cascadeLevel_ = level; }
    
    int getParentEventId() const { return parentEventId_; }
    void setParentEventId(int id) { parentEventId_ = id; }
    
    const std::vector<int>& getCascadedEventIds() const { return cascadedEventIds_; }
    void addCascadedEventId(int id) { cascadedEventIds_.push_back(id); }
    
    // Cascade probability calculation: P = sigmoid(impact × 0.15)
    static float calculateCascadeProbability(int impactLevel) {
        // sigmoid(x) = 1 / (1 + e^(-x))
        float exponent = impactLevel * 0.15f;
        float denominator = 1.0f + std::exp(-exponent);
        return 1.0f / denominator;
    }
    
    // Check if cascade should occur based on probability
    bool shouldCascade(int randomSeed) {
        float cascadeProb = calculateCascadeProbability(impact_level_);
        // Use seeded random: normalize seed to [0, 1)
        float randomValue = (randomSeed % 1000) / 1000.0f;
        return randomValue < cascadeProb;
    }

private:
    int id_;
    std::string name_;
    EventType type_;
    int impact_level_;
    std::vector<int> affected_npc_ids_;
    
    // Cascade tracking fields
    int cascadeLevel_;          // 0=primary, 1=secondary, 2+=tertiary
    int parentEventId_;         // ID of event that triggered this cascade
    std::vector<int> cascadedEventIds_;  // IDs of events this cascade triggered
};

}  // namespace TLS
