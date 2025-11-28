#pragma once

#include <deque>
#include <vector>
#include <memory>
#include "Core.h"

namespace TLS {

// Forward declarations
class NPC;
class Player;

/**
 * @struct ConversationQueueEntry
 * @brief Entry in the NPC conversation queue waiting to talk with player
 */
struct ConversationQueueEntry {
    int npcId = -1;
    float severityScore = 0.0f;      // Problem severity 0-1
    float influenceScore = 0.0f;     // NPC influence/importance 0-1
    int tickArrived = 0;             // When NPC entered proximity
    int queuePosition = 0;           // Position in queue
    float priorityScore = 0.0f;      // Calculated priority for sorting
};

/**
 * @class ProximityDialogueSystem
 * @brief Manages NPC proximity detection and conversation queueing
 *
 * Core responsibilities:
 * - Detect NPCs within proximity range (5 units) of player
 * - Track problem severity for dialogue initiation
 * - Maintain conversation queue with priority sorting
 * - Calculate priority scores based on severity, influence, distance, time in queue
 * - Handle queue overflow (max 5 NPCs)
 *
 * Key Formula (from Phase 6 Plan Section 8a):
 * priority = 0.4*severity + 0.3*influence + 0.15*(1-dist/5) + 0.15*(1-timeSinceArrival/maxTime)
 */
class ProximityDialogueSystem {
public:
    static ProximityDialogueSystem& getInstance();

    // Configuration constants
    static constexpr float PROXIMITY_RANGE = 5.0f;      // Units
    static constexpr int MAX_QUEUE_LENGTH = 5;
    static constexpr int MAX_TIME_IN_QUEUE = 600;       // Ticks (~10 game minutes)
    static constexpr float SEVERITY_THRESHOLD = 0.3f;   // Min severity to queue

    // Priority weights (must sum to 1.0)
    static constexpr float W_SEVERITY = 0.4f;
    static constexpr float W_INFLUENCE = 0.3f;
    static constexpr float W_DISTANCE = 0.15f;
    static constexpr float W_TIME = 0.15f;

    // Per-tick: detect NPCs in proximity and manage queue
    void updateProximityDetection(
        const std::vector<std::shared_ptr<class NPC>>& allNpcs,
        const std::shared_ptr<class Player>& player,
        int currentTick
    );

    // Queue management
    void queueNpcForConversation(
        int npcId,
        int currentTick,
        const std::vector<std::shared_ptr<class NPC>>& allNpcs
    );

    void sortConversationQueue(
        const std::vector<std::shared_ptr<class NPC>>& allNpcs
    );

    // Access queue entries
    ConversationQueueEntry* peekNextConversation();
    const ConversationQueueEntry* peekNextConversation() const;
    ConversationQueueEntry dequeueNextConversation();

    // Priority calculation
    float calculateConversationPriority(
        const ConversationQueueEntry& entry,
        const std::vector<std::shared_ptr<class NPC>>& allNpcs
    ) const;

    // Queue status
    int getQueueLength() const;
    bool isQueueEmpty() const;
    void clearConversationQueue();
    void removeNpcFromQueue(int npcId);

    // Distance calculation helper
    float calculateDistance(const Vector3& pos1, const Vector3& pos2) const;

    // NPC lookup helper
    std::shared_ptr<class NPC> findNpc(
        const std::vector<std::shared_ptr<class NPC>>& allNpcs,
        int npcId
    ) const;

private:
    ProximityDialogueSystem() : npcInProximity_(), lastUpdateTick_(0) {}
    ProximityDialogueSystem(const ProximityDialogueSystem&) = delete;
    ProximityDialogueSystem& operator=(const ProximityDialogueSystem&) = delete;

    std::deque<ConversationQueueEntry> conversationQueue_;
    std::vector<ConversationQueueEntry> npcInProximity_;
    int lastUpdateTick_;
};

}  // namespace TLS
