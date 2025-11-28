/**
 * ConversationQueue.h
 * 
 * Multi-NPC Dialogue Queue Management System (Task #6)
 * 
 * Handles scenarios where multiple NPCs reach the player simultaneously.
 * Prioritizes NPCs based on:
 * - Problem severity (0.4 weight)
 * - NPC influence/leadership (0.3 weight)
 * - Distance to player (0.15 weight)
 * - Time since last dialogue (0.15 weight)
 * 
 * Features:
 * - Up to 5 NPCs can queue for conversation
 * - Automatic priority calculation
 * - Deterministic ordering (seeded RNG for ties)
 * - O(n log n) priority calculation
 * - Max queue size with overflow handling
 */

#ifndef TLS_CONVERSATION_QUEUE_H
#define TLS_CONVERSATION_QUEUE_H

#include "Registries.h"
#include <vector>
#include <memory>
#include <cmath>

namespace TLS {

// ============================================================================
// ConversationQueueEntry: Single NPC waiting for dialogue
// ============================================================================

struct ConversationQueueEntry {
    int npcId;                          // Reference to NPC
    float severityScore;                // Problem severity (0-1)
    float influenceScore;               // NPC influence/leadership (0-1)
    float distanceToPlayer;             // Distance in world units
    int tickArrived;                    // Game tick when reached player
    int lastDialogueTick;               // Last time dialogue with this NPC
    float calculatedPriority;           // Final priority score (0-1)
    int queuePosition;                  // Position in queue (0 = first to talk)
    
    /**
     * Create queue entry for NPC
     */
    ConversationQueueEntry(
        int _npcId = -1,
        float _severity = 0.0f,
        float _influence = 0.0f,
        float _distance = 999.0f,
        int _tickArrived = 0,
        int _lastDialogueTick = 0
    ) : npcId(_npcId), severityScore(_severity), influenceScore(_influence),
        distanceToPlayer(_distance), tickArrived(_tickArrived),
        lastDialogueTick(_lastDialogueTick), calculatedPriority(0.0f),
        queuePosition(-1) {}
};

// ============================================================================
// ConversationQueue: Priority queue management
// ============================================================================

class ConversationQueue {
public:
    /**
     * Maximum NPCs in queue at once
     * Overflow handled with random selection fallback
     */
    static const int MAX_QUEUE_SIZE = 5;
    
    /**
     * Overflow pool size before random selection
     */
    static const int OVERFLOW_POOL_SIZE = 10;
    
    // ========================================================================
    // Queue Management
    // ========================================================================
    
    /**
     * Add NPC to conversation queue
     * 
     * @param npcId ID of NPC reaching player
     * @param severityScore NPC problem severity (0-1)
     * @param influenceScore NPC influence/leadership (0-1)
     * @param distanceToPlayer Distance in world units
     * @param currentTick Current game tick
     * @return true if successfully added, false if full
     */
    bool enqueueNPC(
        int npcId,
        float severityScore,
        float influenceScore,
        float distanceToPlayer,
        int currentTick
    );
    
    /**
     * Remove and return highest-priority NPC from queue
     * 
     * @return Conversation entry for next NPC, or entry with npcId=-1 if empty
     */
    ConversationQueueEntry dequeueNextNPC();
    
    /**
     * Peek at next NPC without removing
     * 
     * @return Next queue entry without modification
     */
    const ConversationQueueEntry& peekNextNPC() const;
    
    /**
     * Clear all entries from queue
     */
    void clear();
    
    /**
     * Get current queue size
     * 
     * @return Number of NPCs waiting
     */
    int getQueueSize() const;
    
    /**
     * Check if queue is empty
     * 
     * @return true if no NPCs queued
     */
    bool isEmpty() const;
    
    /**
     * Check if queue is full (at MAX_QUEUE_SIZE)
     * 
     * @return true if at capacity
     */
    bool isFull() const;
    
    // ========================================================================
    // Priority Calculation
    // ========================================================================
    
    /**
     * Calculate priority score for NPC
     * 
     * Priority formula (weights sum to 1.0):
     * priority = 0.4×severity + 0.3×influence + 0.15×distance + 0.15×time
     * 
     * Where:
     * - severity: problem intensity (0-1)
     * - influence: NPC power/leadership (0-1)
     * - distance: (1 - distance/50) normalized to 0-1
     * - time: (1 - timeSinceDialogue/86400) normalized to 0-1
     * 
     * @param entry Queue entry with all metrics
     * @param currentTick Current game tick
     * @return Priority score (0-1, higher = more urgent)
     */
    static float calculatePriority(
        const ConversationQueueEntry& entry,
        int currentTick
    );
    
    /**
     * Sort queue by priority (highest first)
     * 
     * Called internally after enqueue if needed
     */
    void sortByPriority();
    
    // ========================================================================
    // Queue Display & Status
    // ========================================================================
    
    /**
     * Get formatted string describing queue status for UI
     * 
     * Example: "NPC queue: 3 waiting | Alice (severity=8) next"
     * 
     * @return String representation of queue status
     */
    std::string getQueueStatusString() const;
    
    /**
     * Get formatted string for next NPC in queue
     * 
     * Example: "Alice (Farmer, Concern: Food Shortage)"
     * 
     * @return Formatted description of next NPC
     */
    std::string getNextNPCDescription() const;
    
    /**
     * Get list of all NPCs in queue (for display purposes)
     * 
     * @return Vector of queue entries
     */
    std::vector<ConversationQueueEntry> getQueueList() const;
    
    // ========================================================================
    // Utility Functions
    // ========================================================================
    
    /**
     * Get statistics about queue performance
     * 
     * @return String with stats (avg priority, total processed, etc.)
     */
    std::string getStatistics() const;
    
    /**
     * Reset all statistics counters
     */
    void resetStatistics();
    
    // ========================================================================
    // Singleton Pattern
    // ========================================================================
    
    /**
     * Get singleton instance
     * 
     * @return Reference to conversation queue
     */
    static ConversationQueue& getInstance();
    
    /**
     * Prevent copy construction
     */
    ConversationQueue(const ConversationQueue&) = delete;
    
    /**
     * Prevent assignment
     */
    ConversationQueue& operator=(const ConversationQueue&) = delete;
    
private:
    /**
     * Private constructor (singleton)
     */
    ConversationQueue() = default;
    
    /**
     * Internal queue storage (sorted by priority)
     */
    std::vector<ConversationQueueEntry> m_queue;
    
    /**
     * Overflow pool for >5 NPCs (stored but not prioritized)
     */
    std::vector<ConversationQueueEntry> m_overflowPool;
    
    /**
     * Statistics tracking
     */
    int m_totalEnqueued = 0;              // Total NPCs ever queued
    int m_totalDequeued = 0;              // Total NPCs who conversed
    int m_totalOverflows = 0;             // Times overflow occurred
    int m_maxQueueDepth = 0;              // Peak queue size ever reached
    float m_avgPriorityScore = 0.0f;      // Running average priority
    
    /**
     * Helper: Add entry to overflow pool if queue full
     * 
     * @param entry Queue entry to add
     * @return true if added to overflow, false if overflow full
     */
    bool addToOverflowPool(const ConversationQueueEntry& entry);
    
    /**
     * Helper: Handle overflow with random selection
     * 
     * When overflow pool exceeds OVERFLOW_POOL_SIZE, randomly
     * select one NPC from overflow to add to main queue
     */
    void handleOverflow();
    
    /**
     * Helper: Update statistics
     */
    void updateStatistics(const ConversationQueueEntry& entry);
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Calculate NPC influence score based on faction
 * 
 * @param npcId ID of NPC
 * @return Influence score (0-1)
 */
float calculateNPCInfluence(int npcId);

/**
 * Get NPC problem severity
 * 
 * Severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
 * Threshold: if >= 0.3, NPC recognizes problem
 * 
 * @param npcId ID of NPC
 * @return Severity score (0-1)
 */
float getNPCProblemSeverity(int npcId);

/**
 * Get time since last dialogue with NPC (in ticks)
 * 
 * @param npcId ID of NPC
 * @return Ticks since last conversation (or INT_MAX if never)
 */
int getTimeSinceLastDialogue(int npcId);

/**
 * Format priority score for display
 * 
 * @param priority Priority score (0-1)
 * @return String like "HIGH (0.92)" or "MEDIUM (0.54)"
 */
std::string formatPriority(float priority);

} // namespace TLS

#endif // TLS_CONVERSATION_QUEUE_H
