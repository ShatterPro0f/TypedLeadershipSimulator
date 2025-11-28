#include "Core.h"
#include "Registries.h"
#include "SimulationManager.h"
#include <cstring>
#include <algorithm>
#include <chrono>

namespace TLS {

// ============================================================================
// GameTickProcessor Implementation (Task #7: Event-Driven Architecture)
// ============================================================================

GameTickProcessor* GameTickProcessor::instance() {
    static GameTickProcessor processor;
    return &processor;
}

void GameTickProcessor::processTick(int currentTick) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ticksProcessed_++;
    
    // Process all event types in order (continuous, event-driven)
    processNPCUpdates(currentTick);
    processProximityChecks(currentTick);
    processProblemDetection(currentTick);
    processImmigrationCheck(currentTick);
    processBirthdayCheck(currentTick);
    processFactionRebellion(currentTick);
    processResourceUpdates(currentTick);
    processWorldStateChanges(currentTick);
    processEventTriggers(currentTick);
    processDialogueQueue(currentTick);
    
    // Calculate tick duration
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    lastTickDuration_ = duration.count() / 1000.0f;  // Convert to milliseconds
}

void GameTickProcessor::enqueuTickEvent(const TickEvent& event) {
    tickEventQueue_.push_back(event);
}

void GameTickProcessor::processNPCUpdates(int /* currentTick */) {
    // Update all active NPCs every tick:
    // - Position (pathfinding one step toward destination)
    // - Emotion (continuous update based on world state)
    // - Mood (smooth integration of emotions)
    // - Attitude (long-term memory of player decisions)
    // - Activity state (based on current task)
    
    auto npcs = NPCRegistry::getInstance().getAllNPCs();
    for (auto& npc : npcs) {
        if (!npc) continue;
        
        // Update position if NPC has destination
        if (npc->getDestination() != npc->getPosition()) {
            Vector3 direction = npc->getDestination() - npc->getPosition();
            float distance = direction.magnitude();
            
            if (distance > 0.5f) {
                // Move one step toward destination
                direction = direction.normalized();
                Vector3 newPos = npc->getPosition() + (direction * 0.5f);
                npc->setPosition(newPos);
            } else {
                // Reached destination
                npc->setPosition(npc->getDestination());
            }
        }
        
        // Update emotion based on world state changes (tick by tick)
        // This is triggered by events, not scheduled
        npc->calculateProblemSeverity();
        
        // Update mood continuously (exponential smoothing)
        float immediateEmotion = npc->getImmediateEmotion();
        float currentMood = npc->getShortTermMood();
        float alpha = 0.1f;  // Smoothing factor
        float newMood = alpha * immediateEmotion + (1.0f - alpha) * currentMood;
        npc->setShortTermMood(newMood);
        
        // Update attitude slowly (long-term memory)
        float beta = 0.01f;  // Slow integration factor
        float currentAttitude = npc->getLongTermAttitude();
        float newAttitude = currentAttitude + beta * newMood;
        npc->setLongTermAttitude(newAttitude);
    }
}

void GameTickProcessor::processProximityChecks(int /* currentTick */) {
    // Check if any NPC reached player proximity (< 5 units)
    // This is checked every tick, not on schedule
    // Trigger is: NPC has problem AND reached proximity
    
    // Note: Player position not available in this context
    // This will be called from main.cpp with player position
}

void GameTickProcessor::processProblemDetection(int /* currentTick */) {
    // Check for NPC problems and enqueue them
    // This is called from updateNPCProblems() in main.cpp
}

void GameTickProcessor::processImmigrationCheck(int /* currentTick */) {
    // Check immigration conditions every tick
    // Trigger: immigration conditions met (reputation, housing, food available)
    // When triggered: add new NPC to settlement
}

void GameTickProcessor::processBirthdayCheck(int /* currentTick */) {
    // Check if any NPC has birthday this tick
    // Trigger: (currentTick - creationTick) % TICKS_PER_GAME_YEAR == 0 && creationTick > 0
    // When triggered: age++, promote children to adults if age >= 16
}

void GameTickProcessor::processFactionRebellion(int /* currentTick */) {
    // Check if any faction meets rebellion threshold
    // Trigger: faction_strength * (1 - avg_loyalty) > threshold
    // When triggered: initiate faction rebellion event
}

void GameTickProcessor::processResourceUpdates(int /* currentTick */) {
    // Update resource levels continuously
    // - Production: food from farmers, etc.
    // - Consumption: food consumed by population, etc.
    // - Happens every tick (gradual depletion/growth)
}

void GameTickProcessor::processWorldStateChanges(int currentTick) {
    // Monitor for significant world state changes (Phase 8: Narrative Generation)
    // Trigger: mood_delta > 0.2, faction_loyalty_delta > 0.15, resource crossed scarcity, event occurred
    // When triggered: [async] call LLM for narrative generation
    
    // NOTE: SimulationManager integration deferred until full build
    // Phase 1 tests don't link SimulationManager, so this is a stub
    (void)currentTick;  // Silence unused parameter warning
}

void GameTickProcessor::processEventTriggers(int /* currentTick */) {
    // Check if any natural events should trigger based on probability
    // Trigger: random probability check each tick for each event type
    // When triggered: apply event effects (cascade if applicable)
}

void GameTickProcessor::processDialogueQueue(int /* currentTick */) {
    // Update dialogue queue timing
    // - Increase ticksInQueue for each queued NPC
    // - Re-prioritize if needed (distance changed, etc.)
    
    auto queue = DialogueQueue::instance();
    if (queue->hasQueuedNPCs()) {
        auto entries = queue->getQueueContents();
        // Entries are updated in-place (would need queue modification methods)
    }
}

// ============================================================================
// NPC Implementation
// ============================================================================

NPC::NPC() 
    : id_(0), age_(0), speed_(10.0f), factionId_(-1),
      immediateEmotion_(0.5f), shortTermMood_(0.5f), longTermAttitude_(0.5f),
      loyalty_(0.5f), activity_(Activity::IDLE), conversationState_(0),
      problemSeverity_(0.0f), problemState_(ProblemState::UNRESOLVED),
      ticksAtProblem_(0), lastDialogueTick_(0), previousMood_(0.5f), previousLoyalty_(0.5f) {}

NPC::NPC(int id)
    : id_(id), age_(0), speed_(10.0f), factionId_(-1),
      immediateEmotion_(0.5f), shortTermMood_(0.5f), longTermAttitude_(0.5f),
      loyalty_(0.5f), activity_(Activity::IDLE), conversationState_(0),
      problemSeverity_(0.0f), problemState_(ProblemState::UNRESOLVED),
      ticksAtProblem_(0), lastDialogueTick_(0), previousMood_(0.5f), previousLoyalty_(0.5f) {}

// Phase 12: Test-friendly constructor
NPC::NPC(int id, const std::string& name, const Vector3& position)
    : id_(id), name_(name), age_(0), speed_(10.0f), factionId_(-1),
      position_(position), homeLocation_(position),
      immediateEmotion_(0.5f), shortTermMood_(0.5f), longTermAttitude_(0.5f),
      loyalty_(0.5f), activity_(Activity::IDLE), conversationState_(0),
      problemSeverity_(0.0f), problemState_(ProblemState::UNRESOLVED),
      ticksAtProblem_(0), lastDialogueTick_(0), previousMood_(0.5f), previousLoyalty_(0.5f) {}

std::vector<uint8_t> NPC::serialize() const {
    std::vector<uint8_t> data;
    // This is placeholder; full serialization implemented in Phase 14
    data.resize(70);  // Target ~70 bytes per NPC
    std::memcpy(data.data(), &id_, sizeof(id_));
    return data;
}

NPC NPC::deserialize(const std::vector<uint8_t>& data) {
    NPC npc;
    if (data.size() >= sizeof(int)) {
        std::memcpy(&npc.id_, data.data(), sizeof(int));
    }
    return npc;
}

// ============================================================================
// Problem System Implementation (Task #2)
// ============================================================================

void NPC::calculateProblemSeverity() {
    // Problem Severity Formula (from Equations.txt):
    // severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
    
    float moodDelta = std::abs(shortTermMood_ - previousMood_);
    float loyaltyDelta = std::abs(loyalty_ - previousLoyalty_);
    
    // Weighted combination
    float newSeverity = 0.5f * moodDelta + 0.5f * loyaltyDelta;
    
    // Update severity with smoothing (alpha = 0.3)
    problemSeverity_ = 0.3f * newSeverity + 0.7f * problemSeverity_;
    
    // Clamp to valid range
    problemSeverity_ = std::max(0.0f, std::min(1.0f, problemSeverity_));
}

void NPC::updatePreviousStats() {
    // Store current values for next frame's delta calculation
    previousMood_ = shortTermMood_;
    previousLoyalty_ = loyalty_;
}

bool NPC::isProblemResolved(float foodQty, float scarcityThreshold) {
    // Check if world state improved enough to resolve problem
    
    // Case 1: Food scarcity issue - resolved if food above threshold
    if (role_ == "Farmer" || role_ == "Worker" || role_ == "Laborer") {
        if (foodQty > scarcityThreshold) {
            return true;  // Food scarcity resolved
        }
    }
    
    // Case 2: Loyalty-based resolution - NPC satisfied with player
    if (loyalty_ > 0.6f) {
        return true;
    }
    
    // Case 3: Mood improvement - NPC feeling better
    if (shortTermMood_ > 0.5f && problemSeverity_ < 0.2f) {
        return true;
    }
    
    // Default: not resolved
    return false;
}

void NPC::escalateProblem() {
    // Increase severity if problem persists too long
    // Called after 5 days (1 day = 14400 ticks / 24 hours = 600 ticks per hour)
    
    const int ESCALATION_THRESHOLD = 5 * 14400;  // 5 game days in ticks
    
    if (ticksAtProblem_ > ESCALATION_THRESHOLD) {
        // Increase severity by 0.1 (cap at 1.0)
        problemSeverity_ = std::min(1.0f, problemSeverity_ + 0.1f);
        ticksAtProblem_ = 0;  // Reset counter
    }
}

// ============================================================================
// Advisor Implementation
// ============================================================================

Advisor::Advisor()
    : NPC(), specialty_(Specialty::POLITICS), influenceScore_(0.0f),
      trustLevel_(0.5f), factionAlignment_(0.0f), agenda_(Agenda::SHORT_TERM),
      riskTolerance_(0.5f), strategyStyle_(StrategyStyle::HONEST) {}

Advisor::Advisor(int id)
    : NPC(id), specialty_(Specialty::POLITICS), influenceScore_(0.0f),
      trustLevel_(0.5f), factionAlignment_(0.0f), agenda_(Agenda::SHORT_TERM),
      riskTolerance_(0.5f), strategyStyle_(StrategyStyle::HONEST) {}

std::vector<uint8_t> Advisor::serialize() const {
    auto base = NPC::serialize();
    // Append advisor-specific data
    return base;
}

Advisor Advisor::deserialize(const std::vector<uint8_t>& data) {
    NPC npc = NPC::deserialize(data);
    Advisor advisor(npc.getId());
    return advisor;
}

// ============================================================================
// Resource Implementation
// ============================================================================

Resource::Resource()
    : id_(0), quantity_(0), productionRate_(0), consumptionRate_(0),
      scarcityThreshold_(0) {}

Resource::Resource(int id)
    : id_(id), quantity_(0), productionRate_(0), consumptionRate_(0),
      scarcityThreshold_(0) {}

// Phase 12: Test-friendly constructor
Resource::Resource(int id, const std::string& name)
    : id_(id), name_(name), quantity_(0), productionRate_(0), consumptionRate_(0),
      scarcityThreshold_(0) {}

std::vector<uint8_t> Resource::serialize() const {
    std::vector<uint8_t> data;
    data.resize(50);  // ~50 bytes per resource
    std::memcpy(data.data(), &id_, sizeof(id_));
    return data;
}

Resource Resource::deserialize(const std::vector<uint8_t>& data) {
    Resource resource;
    if (data.size() >= sizeof(int)) {
        std::memcpy(&resource.id_, data.data(), sizeof(int));
    }
    return resource;
}

// ============================================================================
// Faction Implementation
// ============================================================================

Faction::Faction()
    : id_(0), strength_(0.0f), emergenceProbability_(0.0f),
      alignment_(Alignment::NEUTRAL) {}

Faction::Faction(int id)
    : id_(id), strength_(0.0f), emergenceProbability_(0.0f),
      alignment_(Alignment::NEUTRAL) {}

// Phase 12: Test-friendly constructor
Faction::Faction(int id, const std::string& name)
    : id_(id), name_(name), strength_(0.0f), emergenceProbability_(0.0f),
      alignment_(Alignment::NEUTRAL) {}

std::vector<uint8_t> Faction::serialize() const {
    std::vector<uint8_t> data;
    data.resize(100);  // ~100 bytes per faction (varies with member count)
    std::memcpy(data.data(), &id_, sizeof(id_));
    return data;
}

Faction Faction::deserialize(const std::vector<uint8_t>& data) {
    Faction faction;
    if (data.size() >= sizeof(int)) {
        std::memcpy(&faction.id_, data.data(), sizeof(int));
    }
    return faction;
}

// ============================================================================
// Event Implementation
// ============================================================================

Event::Event()
    : id_(0), type_(EventType::ENVIRONMENTAL), impactLevel_(0) {}

Event::Event(int id)
    : id_(id), type_(EventType::ENVIRONMENTAL), impactLevel_(0) {}

std::vector<uint8_t> Event::serialize() const {
    std::vector<uint8_t> data;
    data.resize(80);  // ~80 bytes per event
    std::memcpy(data.data(), &id_, sizeof(id_));
    return data;
}

Event Event::deserialize(const std::vector<uint8_t>& data) {
    Event event;
    if (data.size() >= sizeof(int)) {
        std::memcpy(&event.id_, data.data(), sizeof(int));
    }
    return event;
}

// ============================================================================
// DialogueQueue Implementation (Task #6)
// ============================================================================

DialogueQueue* DialogueQueue::instance() {
    static DialogueQueue queue;
    return &queue;
}

void DialogueQueue::enqueue(std::shared_ptr<NPC> npc, float severity, float influence, float distance, int currentTick) {
    if (!npc) return;
    
    DialogueQueueEntry entry;
    entry.npc = npc;
    entry.severityScore = severity;
    entry.influenceScore = influence;
    entry.distanceToPlayer = distance;
    entry.ticksArrived = currentTick;
    entry.ticksInQueue = 0;
    entry.priorityScore = calculatePriority(severity, influence, distance, 0);
    
    queue_.push_back(entry);
    sortByPriority();
}

std::shared_ptr<NPC> DialogueQueue::dequeue() {
    if (queue_.empty()) {
        return nullptr;
    }
    
    // Get highest priority NPC (first in sorted list)
    std::shared_ptr<NPC> npc = queue_[0].npc;
    queue_.erase(queue_.begin());
    
    return npc;
}

bool DialogueQueue::hasQueuedNPCs() const {
    return !queue_.empty();
}

void DialogueQueue::clearQueue() {
    queue_.clear();
}

float DialogueQueue::calculatePriority(float severity, float influence, float distance, int ticksWaiting) {
    // Priority formula (from copilot instructions Section 8a):
    // priority = 0.4×severity + 0.3×influence + 0.15×distance_factor + 0.15×time_factor
    
    // Distance factor: closer = higher priority (invert distance, normalize)
    float distanceFactor = std::max(0.0f, 1.0f - (distance / 50.0f));  // 50 units = max distance
    
    // Time factor: longer wait = higher priority
    float timeFactor = std::min(1.0f, ticksWaiting / 3000.0f);  // Max priority after 3000 ticks
    
    // Weighted combination
    float priority = 0.4f * severity + 0.3f * influence + 0.15f * distanceFactor + 0.15f * timeFactor;
    
    return priority;
}

void DialogueQueue::sortByPriority() {
    // Sort in descending order (highest priority first)
    std::sort(queue_.begin(), queue_.end(), 
        [](const DialogueQueueEntry& a, const DialogueQueueEntry& b) {
            return a.priorityScore > b.priorityScore;
        });
}

std::string DialogueQueue::getQueueStatus() const {
    if (queue_.empty()) {
        return "[Queue] Empty";
    }
    
    std::string status = "[Queue] " + std::to_string(queue_.size()) + " NPC(s) waiting:\n";
    
    for (size_t i = 0; i < queue_.size() && i < 5; ++i) {  // Show max 5 in queue
        const auto& entry = queue_[i];
        status += "  " + std::to_string(i + 1) + ". " + entry.npc->getName() + 
                 " [Priority: " + std::to_string(static_cast<int>(entry.priorityScore * 100)) + "%]\n";
    }
    
    if (queue_.size() > 5) {
        status += "  ... and " + std::to_string(queue_.size() - 5) + " more\n";
    }
    
    return status;
}

}  // namespace TLS

