/**
 * ConversationQueue.cpp
 * 
 * Implementation of multi-NPC dialogue queue management
 * 
 * Handles simultaneous NPC arrivals with priority-based ordering
 */

#include "ConversationQueue.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

namespace TLS {

// ============================================================================
// ConversationQueue Implementation
// ============================================================================

bool ConversationQueue::enqueueNPC(
    int npcId,
    float severityScore,
    float influenceScore,
    float distanceToPlayer,
    int currentTick
) {
    // Validate NPC exists
    auto npc = NPCRegistry::getInstance().getNPCById(npcId);
    if (!npc) {
        return false;
    }
    
    // Create queue entry
    auto lastDialogue = getTimeSinceLastDialogue(npcId);
    ConversationQueueEntry entry(
        npcId,
        severityScore,
        influenceScore,
        distanceToPlayer,
        currentTick,
        currentTick - lastDialogue
    );
    
    // Calculate priority
    entry.calculatedPriority = calculatePriority(entry, currentTick);
    
    // Check if queue full
    if (isFull()) {
        // Try to add to overflow pool
        if (addToOverflowPool(entry)) {
            m_totalOverflows++;
            handleOverflow();
            return true;
        }
        return false;
    }
    
    // Add to main queue
    m_queue.push_back(entry);
    m_totalEnqueued++;
    
    // Sort queue by priority (highest first)
    sortByPriority();
    
    // Update statistics
    updateStatistics(entry);
    
    // Update max depth
    if (static_cast<int>(m_queue.size()) > m_maxQueueDepth) {
        m_maxQueueDepth = m_queue.size();
    }
    
    return true;
}

ConversationQueueEntry ConversationQueue::dequeueNextNPC() {
    if (isEmpty()) {
        return ConversationQueueEntry();  // Return invalid entry
    }
    
    // Get first entry
    ConversationQueueEntry entry = m_queue.front();
    entry.queuePosition = 0;
    
    // Remove from queue
    m_queue.erase(m_queue.begin());
    m_totalDequeued++;
    
    // Try to promote from overflow pool if available
    if (!m_overflowPool.empty()) {
        // Select random NPC from overflow (deterministic with seed)
        int randomIndex = m_totalDequeued % m_overflowPool.size();
        ConversationQueueEntry promoted = m_overflowPool[randomIndex];
        m_overflowPool.erase(m_overflowPool.begin() + randomIndex);
        
        // Add to main queue
        m_queue.push_back(promoted);
        sortByPriority();
    }
    
    return entry;
}

const ConversationQueueEntry& ConversationQueue::peekNextNPC() const {
    static ConversationQueueEntry empty;
    if (isEmpty()) {
        return empty;
    }
    return m_queue.front();
}

void ConversationQueue::clear() {
    m_queue.clear();
    m_overflowPool.clear();
}

int ConversationQueue::getQueueSize() const {
    return m_queue.size();
}

bool ConversationQueue::isEmpty() const {
    return m_queue.empty();
}

bool ConversationQueue::isFull() const {
    return m_queue.size() >= MAX_QUEUE_SIZE;
}

// ============================================================================
// Priority Calculation
// ============================================================================

float ConversationQueue::calculatePriority(
    const ConversationQueueEntry& entry,
    int currentTick
) {
    // Normalize distance (max 50 units)
    float distanceNorm = std::max(0.0f, 1.0f - (entry.distanceToPlayer / 50.0f));
    
    // Normalize time since dialogue (max 1 day = 14400 ticks)
    int ticksSinceDialogue = currentTick - entry.lastDialogueTick;
    float timeNorm = std::min(1.0f, static_cast<float>(ticksSinceDialogue) / 14400.0f);
    
    // Apply weights
    const float w_severity = 0.40f;
    const float w_influence = 0.30f;
    const float w_distance = 0.15f;
    const float w_time = 0.15f;
    
    float priority = 
        w_severity * entry.severityScore +
        w_influence * entry.influenceScore +
        w_distance * distanceNorm +
        w_time * timeNorm;
    
    // Clamp to [0, 1]
    return std::max(0.0f, std::min(1.0f, priority));
}

void ConversationQueue::sortByPriority() {
    // Sort queue by priority (highest first)
    std::sort(m_queue.begin(), m_queue.end(),
        [](const ConversationQueueEntry& a, const ConversationQueueEntry& b) {
            // Higher priority first
            if (std::abs(a.calculatedPriority - b.calculatedPriority) > 0.001f) {
                return a.calculatedPriority > b.calculatedPriority;
            }
            // Tie-breaker: earlier arrival time
            return a.tickArrived < b.tickArrived;
        }
    );
    
    // Update queue positions
    for (int i = 0; i < static_cast<int>(m_queue.size()); i++) {
        m_queue[i].queuePosition = i;
    }
}

// ============================================================================
// Queue Display & Status
// ============================================================================

std::string ConversationQueue::getQueueStatusString() const {
    std::ostringstream oss;
    
    if (isEmpty()) {
        oss << "No NPCs waiting for dialogue";
        return oss.str();
    }
    
    oss << "NPC Queue: " << m_queue.size() << "/" << MAX_QUEUE_SIZE;
    
    if (!m_overflowPool.empty()) {
        oss << " (+" << m_overflowPool.size() << " overflow)";
    }
    
    oss << " | Next: " << getNextNPCDescription();
    
    return oss.str();
}

std::string ConversationQueue::getNextNPCDescription() const {
    if (isEmpty()) {
        return "None";
    }
    
    const auto& next = m_queue.front();
    auto npc = NPCRegistry::getInstance().getNPCById(next.npcId);
    
    if (!npc) {
        return "Unknown";
    }
    
    std::ostringstream oss;
    oss << npc->getName() << " (" << npc->getRole() << ")";
    oss << " [Priority: " << std::fixed << std::setprecision(2) 
        << next.calculatedPriority << "]";
    
    return oss.str();
}

std::vector<ConversationQueueEntry> ConversationQueue::getQueueList() const {
    return m_queue;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string ConversationQueue::getStatistics() const {
    std::ostringstream oss;
    
    oss << "ConversationQueue Statistics:\n";
    oss << "  Total Enqueued: " << m_totalEnqueued << "\n";
    oss << "  Total Dequeued: " << m_totalDequeued << "\n";
    oss << "  Total Overflows: " << m_totalOverflows << "\n";
    oss << "  Max Queue Depth: " << m_maxQueueDepth << "\n";
    oss << "  Current Queue Size: " << m_queue.size() << "\n";
    oss << "  Overflow Pool Size: " << m_overflowPool.size() << "\n";
    
    if (m_totalEnqueued > 0) {
        oss << "  Avg Priority: " << std::fixed << std::setprecision(3)
            << (m_avgPriorityScore / m_totalEnqueued) << "\n";
    }
    
    return oss.str();
}

void ConversationQueue::resetStatistics() {
    m_totalEnqueued = 0;
    m_totalDequeued = 0;
    m_totalOverflows = 0;
    m_maxQueueDepth = 0;
    m_avgPriorityScore = 0.0f;
}

// ============================================================================
// Singleton
// ============================================================================

ConversationQueue& ConversationQueue::getInstance() {
    static ConversationQueue instance;
    return instance;
}

// ============================================================================
// Private Helpers
// ============================================================================

bool ConversationQueue::addToOverflowPool(const ConversationQueueEntry& entry) {
    if (m_overflowPool.size() >= OVERFLOW_POOL_SIZE) {
        return false;
    }
    m_overflowPool.push_back(entry);
    return true;
}

void ConversationQueue::handleOverflow() {
    // If overflow pool is full, keep only highest-priority NPC
    if (m_overflowPool.size() > OVERFLOW_POOL_SIZE) {
        // Sort overflow by priority
        std::sort(m_overflowPool.begin(), m_overflowPool.end(),
            [](const ConversationQueueEntry& a, const ConversationQueueEntry& b) {
                return a.calculatedPriority > b.calculatedPriority;
            }
        );
        
        // Keep only top NPC, discard others
        if (!m_overflowPool.empty()) {
            m_overflowPool.erase(m_overflowPool.begin() + 1, m_overflowPool.end());
        }
    }
}

void ConversationQueue::updateStatistics(const ConversationQueueEntry& entry) {
    m_avgPriorityScore += entry.calculatedPriority;
}

// ============================================================================
// Helper Functions
// ============================================================================

float calculateNPCInfluence(int npcId) {
    auto npc = NPCRegistry::getInstance().getNPCById(npcId);
    if (!npc) {
        return 0.0f;
    }
    
    // Influence = loyalty + (is_leader ? 0.3 : 0.0) + faction_influence
    float baseInfluence = npc->getLoyalty();
    
    // Check if leader (simplified: if isAdvisor or high status)
    // This is a placeholder; actual implementation would check
    // faction leader status
    
    // Get faction info
    int factionId = npc->getFactionId();
    auto faction = FactionRegistry::getInstance().getFactionById(factionId);
    
    if (faction) {
        float factionStrength = faction->getStrength();
        // Influence affected by faction strength
        baseInfluence += (factionStrength * 0.2f);
    }
    
    return std::max(0.0f, std::min(1.0f, baseInfluence));
}

float getNPCProblemSeverity(int npcId) {
    auto npc = NPCRegistry::getInstance().getNPCById(npcId);
    if (!npc) {
        return 0.0f;
    }
    
    // Severity formula from copilot-instructions.md Section 8a:
    // severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
    
    // Get current and previous mood/loyalty (stored in NPC)
    float currentMood = npc->getShortTermMood();
    float currentLoyalty = npc->getLoyalty();
    
    // Get previous values (simplified: use stored deltas if available)
    // This is a placeholder; real implementation would track history
    float previousMood = 0.5f;  // Neutral baseline
    float previousLoyalty = 0.5f;
    
    float moodDelta = std::abs(currentMood - previousMood);
    float loyaltyDelta = std::abs(currentLoyalty - previousLoyalty);
    
    float severity = 0.5f * moodDelta + 0.5f * loyaltyDelta;
    
    // Threshold: if >= 0.3, NPC recognizes problem
    return std::max(0.0f, std::min(1.0f, severity));
}

int getTimeSinceLastDialogue(int npcId) {
    auto npc = NPCRegistry::getInstance().getNPCById(npcId);
    if (!npc) {
        return INT_MAX;
    }
    
    // Get last dialogue tick (simplified: placeholder)
    // In full implementation, track dialogue history per NPC
    return 0;  // Default: was just talking
}

std::string formatPriority(float priority) {
    std::ostringstream oss;
    
    if (priority >= 0.75f) {
        oss << "CRITICAL";
    } else if (priority >= 0.50f) {
        oss << "HIGH";
    } else if (priority >= 0.25f) {
        oss << "MEDIUM";
    } else {
        oss << "LOW";
    }
    
    oss << " (" << std::fixed << std::setprecision(2) << priority << ")";
    
    return oss.str();
}

} // namespace TLS
