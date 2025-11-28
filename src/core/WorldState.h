#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include "NPC.h"
#include "Faction.h"
#include "Resource.h"
#include "Event.h"

namespace TLS {

// ============================================================================
// World State Tracking for LLM Narrative Generation
// ============================================================================

/**
 * WorldState tracks the complete settlement state and enables:
 * 1. Continuous world state monitoring (every tick)
 * 2. Detection of significant changes (mood deltas, faction loyalty shifts, etc.)
 * 3. Snapshots for LLM narrative generation (only when significant changes detected)
 * 4. Deterministic replay logging for debugging
 */
class WorldState {
public:
    // ========== PUBLIC TYPES ==========
    
    struct ResourceSnapshot {
        int resourceId;
        std::string name;
        int quantity;
        int productionRate;
        int consumptionRate;
        int previousQuantity;  // For detecting scarcity crossing
        
        bool hasChanged() const { return quantity != previousQuantity; }
    };
    
    struct NPCSnapshot {
        int npcId;
        std::string name;
        float mood;
        float previousMood;
        float loyalty;
        float previousLoyalty;
        int factionId;
        std::string currentActivity;
        
        float getMoodDelta() const { return std::abs(mood - previousMood); }
        float getLoyaltyDelta() const { return std::abs(loyalty - previousLoyalty); }
        bool isSignificant(float moodThreshold = 0.2f, float loyaltyThreshold = 0.15f) const {
            return getMoodDelta() > moodThreshold || getLoyaltyDelta() > loyaltyThreshold;
        }
    };
    
    struct FactionSnapshot {
        int factionId;
        std::string name;
        float averageLoyalty;
        float previousAverageLoyalty;
        float strength;
        int memberCount;
        
        float getLoyaltyDelta() const { return std::abs(averageLoyalty - previousAverageLoyalty); }
        bool isSignificant(float threshold = 0.15f) const {
            return getLoyaltyDelta() > threshold;
        }
    };
    
    struct EventSnapshot {
        int eventId;
        std::string name;
        std::string type;
        int impactLevel;
        int triggeredTick;
        std::vector<int> affectedNPCIds;
    };
    
    struct ImmigrationSnapshot {
        int newNPCId;
        std::string name;
        int arrivedTick;
    };
    
    // ========== MAIN STATE STRUCTURE ==========
    
    int currentTick;
    int gameTime;  // Milliseconds of simulated time
    
    std::unordered_map<int, NPCSnapshot> npcs;  // By NPC ID
    std::unordered_map<int, FactionSnapshot> factions;  // By Faction ID
    std::unordered_map<int, ResourceSnapshot> resources;  // By Resource ID
    std::vector<EventSnapshot> recentEvents;
    std::vector<ImmigrationSnapshot> newImmigrants;
    
    // Derived metrics
    float settlementMorale;  // Average mood across all NPCs
    float settlementStability;  // Overall faction cohesion
    int populationCount;
    int totalResources;
    
    // ========== CHANGE DETECTION ==========
    
    struct WorldStateChange {
        enum ChangeType {
            NPC_MOOD_SHIFT,
            NPC_LOYALTY_SHIFT,
            FACTION_LOYALTY_SHIFT,
            RESOURCE_SCARCITY_CROSSED,
            EVENT_TRIGGERED,
            IMMIGRATION_OCCURRED,
            EMIGRATION_OCCURRED
        };
        
        ChangeType type;
        int primaryId;  // NPC ID, Faction ID, or Resource ID
        std::string description;
        float magnitude;  // How significant (0-1)
        int tickOccurred;
    };
    
    std::vector<WorldStateChange> changesSinceLastSnapshot;
    
    // ========== PUBLIC METHODS ==========
    
    WorldState();
    
    /**
     * Update world state based on current game state
     * Call this every tick to track changes
     */
    void update(const std::vector<NPC*>& activeNPCs,
                const std::vector<Faction*>& factions,
                const std::vector<Resource*>& resources,
                const std::vector<Event*>& recentEvents,
                int currentTick);
    
    /**
     * Detect if world state has significantly changed
     * Used to decide whether to call LLM for narrative generation
     */
    bool hasSignificantChanges(float moodThreshold = 0.2f, 
                               float loyaltyThreshold = 0.15f,
                               float resourceThreshold = 0.15f) const;
    
    /**
     * Get summary of significant changes for LLM context
     */
    std::string getSummaryOfChanges() const;
    
    /**
     * Check if any NPC mood delta exceeds threshold
     */
    bool hasNPCMoodShifts(float threshold = 0.2f) const;
    
    /**
     * Check if any faction loyalty delta exceeds threshold
     */
    bool hasFactionLoyaltyShifts(float threshold = 0.15f) const;
    
    /**
     * Check if any resource crossed scarcity threshold
     */
    bool hasResourceScarcityCrossing() const;
    
    /**
     * Get count of significant NPCs (those with deltas exceeding threshold)
     */
    int getSignificantNPCCount(float moodThreshold = 0.2f) const;
    
    /**
     * Get list of NPC IDs with significant changes
     */
    std::vector<int> getSignificantNPCIds(float moodThreshold = 0.2f) const;
    
    /**
     * Clear tracked changes (call after LLM snapshot generated)
     */
    void clearChanges();
    
    /**
     * Calculate overall settlement metrics from current state
     */
    void recalculateMetrics();
    
    /**
     * Serialize for replay logging
     */
    std::string serializeToJSON() const;
    
    /**
     * Human-readable summary for debugging
     */
    std::string getDebugSummary() const;
    
private:
    float previousSettlementMorale_;
    float previousSettlementStability_;
    int previousPopulationCount_;
    
    void recordChange(const WorldStateChange& change);
    float calculateSettlementMorale() const;
    float calculateSettlementStability() const;
};

// ============================================================================
// World State Change Detector - Encapsulates detection logic
// ============================================================================

class WorldStateChangeDetector {
public:
    struct DetectionThresholds {
        float npcMoodThreshold;
        float npcLoyaltyThreshold;
        float factionLoyaltyThreshold;
        float resourceScarcityFraction;
        int significantNPCCountThreshold;  // Trigger if >= N NPCs changed
        
        DetectionThresholds() 
            : npcMoodThreshold(0.2f), 
              npcLoyaltyThreshold(0.15f), 
              factionLoyaltyThreshold(0.15f),
              resourceScarcityFraction(0.15f),
              significantNPCCountThreshold(1) {}
    };
    
    WorldStateChangeDetector(const DetectionThresholds& thresholds = DetectionThresholds());
    
    /**
     * Check if changes warrant LLM narrative generation call
     * Returns true if any threshold exceeded
     */
    bool shouldGenerateNarrative(const WorldState& currentState) const;
    
    /**
     * Get detailed reason why narrative generation triggered
     */
    std::string getDetectionReason(const WorldState& currentState) const;
    
    /**
     * Adjust detection thresholds dynamically based on game state
     * (e.g., increase thresholds during crisis to reduce LLM spam)
     */
    void adjustThresholdsForGameState(int populationCount, float settlementStability);
    
private:
    DetectionThresholds thresholds_;
    int ticksSinceLastNarrative_ = 0;
    int minTicksBetweenNarratives_ = 100;  // Don't call LLM more than every 100 ticks
};

// ============================================================================
// Continuous World State Monitor - Aggregates changes over multiple ticks
// ============================================================================

class ContinuousWorldStateMonitor {
public:
    ContinuousWorldStateMonitor();
    
    /**
     * Update monitor with current world state
     * Returns true if should trigger LLM narrative generation
     */
    bool tick(const WorldState& currentState, int currentTick);
    
    /**
     * Get pending snapshot that triggered narrative generation
     */
    WorldState getPendingSnapshot() const { return pendingSnapshot_; }
    
    /**
     * Acknowledge snapshot was processed (call after LLM response received)
     */
    void acknowledgePendingSnapshot() { pendingSnapshot_ = lastSnapshot_; }
    
    /**
     * Get statistics about detection performance
     */
    struct MonitorStats {
        int totalTicksMonitored = 0;
        int totalSnapshotTriggered = 0;
        int ticksSinceLastTrigger = 0;
        float averageTicksPerTrigger = 0.0f;
    };
    
    MonitorStats getStats() const;
    
private:
    WorldState lastSnapshot_;
    WorldState pendingSnapshot_;
    WorldStateChangeDetector detector_;
    MonitorStats stats_;
    int lastNarrativeTriggerTick_ = -1;
};

// ============================================================================
// Batch Change Accumulator - For handling rapid multi-system changes
// ============================================================================

class BatchChangeAccumulator {
public:
    /**
     * Start accumulation window (e.g., 5-10 ticks)
     */
    void startBatch(int currentTick, int windowSize = 10);
    
    /**
     * Add a change to current batch
     */
    void addChange(const WorldState::WorldStateChange& change);
    
    /**
     * Check if batch window closed and ready to process
     */
    bool isBatchReady(int currentTick) const;
    
    /**
     * Get all accumulated changes as single LLM context
     */
    std::string getBatchContext() const;
    
    /**
     * Clear batch for next window
     */
    void clearBatch();
    
private:
    int batchStartTick_ = -1;
    int batchWindowSize_ = 10;
    std::vector<WorldState::WorldStateChange> accumulatedChanges_;
};

}  // namespace TLS
