#include "Core.h"
#include <algorithm>

namespace TLS {

// ============================================================================
// WorldState Constructor/Destructor
// ============================================================================

WorldState::WorldState() 
    : moodThreshold_(0.2f), 
      loyaltyThreshold_(0.15f), 
      resourceThreshold_(50) {
    // Initialize with default thresholds per copilot-instructions.md Section 12a2
}

WorldState::~WorldState() {
    clear();
}

// ============================================================================
// Record World State Changes
// ============================================================================

void WorldState::recordNPCChange(int npcId, float moodDelta, float loyaltyDelta) {
    // Only record if changes exceed threshold
    if (std::abs(moodDelta) > moodThreshold_ || std::abs(loyaltyDelta) > loyaltyThreshold_) {
        auto it = std::find(changedNPCs_.begin(), changedNPCs_.end(), npcId);
        if (it == changedNPCs_.end()) {
            changedNPCs_.push_back(npcId);
            npcMoodDeltas_.push_back(moodDelta);
            npcLoyaltyDeltas_.push_back(loyaltyDelta);
        } else {
            // Update existing entry (accumulate deltas)
            size_t index = std::distance(changedNPCs_.begin(), it);
            npcMoodDeltas_[index] += moodDelta;
            npcLoyaltyDeltas_[index] += loyaltyDelta;
        }
    }
}

void WorldState::recordFactionChange(int factionId, float loyaltyDelta) {
    // Record faction-level changes
    if (std::abs(loyaltyDelta) > loyaltyThreshold_) {
        auto it = std::find(changedFactions_.begin(), changedFactions_.end(), factionId);
        if (it == changedFactions_.end()) {
            changedFactions_.push_back(factionId);
            factionLoyaltyDeltas_.push_back(loyaltyDelta);
        } else {
            size_t index = std::distance(changedFactions_.begin(), it);
            factionLoyaltyDeltas_[index] += loyaltyDelta;
        }
    }
}

void WorldState::recordResourceChange(int resourceId, int quantityDelta) {
    // Record resource-level changes
    if (std::abs(quantityDelta) > resourceThreshold_) {
        auto it = std::find(changedResources_.begin(), changedResources_.end(), resourceId);
        if (it == changedResources_.end()) {
            changedResources_.push_back(resourceId);
            resourceQuantityDeltas_.push_back(quantityDelta);
        } else {
            size_t index = std::distance(changedResources_.begin(), it);
            resourceQuantityDeltas_[index] += quantityDelta;
        }
    }
}

void WorldState::recordEvent(int eventId) {
    // Record triggered events
    auto it = std::find(triggeredEvents_.begin(), triggeredEvents_.end(), eventId);
    if (it == triggeredEvents_.end()) {
        triggeredEvents_.push_back(eventId);
    }
}

// ============================================================================
// Generate Snapshot for LLM Context
// ============================================================================

WorldState::StateSnapshot WorldState::generateSnapshot(int tickNumber) {
    StateSnapshot snapshot;
    snapshot.tickNumber = tickNumber;
    snapshot.significantNPCIds = changedNPCs_;
    snapshot.affectedFactionIds = changedFactions_;
    snapshot.changedResourceIds = changedResources_;
    snapshot.triggeredEventIds = triggeredEvents_;
    return snapshot;
}

// ============================================================================
// Clear State for Next Tick
// ============================================================================

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
