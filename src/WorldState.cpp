#include "Core.h"

namespace TLS {

WorldState::WorldState()
    : moodThreshold_(0.2f),
      loyaltyThreshold_(0.15f),
      resourceThreshold_(50) {
}

WorldState::~WorldState() {
}

void WorldState::recordNPCChange(int npcId, float moodDelta, float loyaltyDelta) {
    // Record if deltas exceed thresholds
    if (std::abs(moodDelta) > moodThreshold_ || std::abs(loyaltyDelta) > loyaltyThreshold_) {
        changedNPCs_.push_back(npcId);
        npcMoodDeltas_.push_back(moodDelta);
        npcLoyaltyDeltas_.push_back(loyaltyDelta);
    }
}

void WorldState::recordFactionChange(int factionId, float loyaltyDelta) {
    if (std::abs(loyaltyDelta) > loyaltyThreshold_) {
        changedFactions_.push_back(factionId);
        factionLoyaltyDeltas_.push_back(loyaltyDelta);
    }
}

void WorldState::recordResourceChange(int resourceId, int quantityDelta) {
    if (std::abs(quantityDelta) > resourceThreshold_) {
        changedResources_.push_back(resourceId);
        resourceQuantityDeltas_.push_back(quantityDelta);
    }
}

void WorldState::recordEvent(int eventId) {
    triggeredEvents_.push_back(eventId);
}

WorldState::StateSnapshot WorldState::generateSnapshot(int tickNumber) {
    StateSnapshot snapshot;
    snapshot.tickNumber = tickNumber;
    
    // Collect significant NPCs
    for (size_t i = 0; i < changedNPCs_.size(); ++i) {
        if (std::abs(npcMoodDeltas_[i]) > moodThreshold_ || 
            std::abs(npcLoyaltyDeltas_[i]) > loyaltyThreshold_) {
            snapshot.significantNPCIds.push_back(changedNPCs_[i]);
        }
    }
    
    // Collect affected factions
    for (size_t i = 0; i < changedFactions_.size(); ++i) {
        if (std::abs(factionLoyaltyDeltas_[i]) > loyaltyThreshold_) {
            snapshot.affectedFactionIds.push_back(changedFactions_[i]);
        }
    }
    
    // Collect changed resources
    for (size_t i = 0; i < changedResources_.size(); ++i) {
        if (std::abs(resourceQuantityDeltas_[i]) > resourceThreshold_) {
            snapshot.changedResourceIds.push_back(changedResources_[i]);
        }
    }
    
    snapshot.triggeredEventIds = triggeredEvents_;
    
    // Clear for next frame
    clear();
    
    return snapshot;
}

void WorldState::clear() {
    changedNPCs_.clear();
    npcMoodDeltas_.clear();
    npcLoyaltyDeltas_.clear();
    
    changedFactions_.clear();
    factionLoyaltyDeltas_.clear();
    
    changedResources_.clear();
    resourceQuantityDeltas_.clear();
    
    triggeredEvents_.clear();
}

}  // namespace TLS
