#ifndef CORE_H
#define CORE_H

#include "Enums.h"
#include "Vector3.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace TLS {

// Forward declarations for pathfinding and movement
struct Path;
struct MovementState;

// ============================================================================
// Forward Declarations
// ============================================================================

class NPC;
class Advisor;
class Faction;
class Resource;
class Event;
class WorldState;

// ============================================================================
// Multi-NPC Dialogue Queue System (Task #6)
// ============================================================================

struct DialogueQueueEntry {
    std::shared_ptr<NPC> npc;              // NPC waiting to dialogue
    float priorityScore = 0.0f;            // Priority in queue (higher = more urgent)
    float severityScore = 0.0f;            // Problem severity (0-1)
    float influenceScore = 0.0f;           // NPC influence/importance (0-1)
    float distanceToPlayer = 1000.0f;      // Distance from player
    int ticksInQueue = 0;                  // How long waiting
    int ticksArrived = 0;                  // When did NPC reach proximity
};

class DialogueQueue {
public:
    static DialogueQueue* instance();
    
    // Queue management
    void enqueue(std::shared_ptr<NPC> npc, float severity, float influence, float distance, int currentTick);
    std::shared_ptr<NPC> dequeue();  // Returns highest priority NPC
    bool hasQueuedNPCs() const;
    size_t getQueueSize() const { return queue_.size(); }
    void clearQueue();
    
    // Queue info
    std::vector<DialogueQueueEntry> getQueueContents() const { return queue_; }
    std::string getQueueStatus() const;  // Formatted queue display for UI
    
    // Priority calculation
    static float calculatePriority(float severity, float influence, float distance, int ticksWaiting);
    
private:
    DialogueQueue() = default;
    std::vector<DialogueQueueEntry> queue_;  // Priority-sorted queue
    
    // Sort queue by priority (highest first)
    void sortByPriority();
};

// ============================================================================
// Game Tick System (Task #7: Event-Driven Architecture)
// ============================================================================

// Tick configuration constants
const int TICKS_PER_GAME_MINUTE = 10;
const int TICKS_PER_GAME_HOUR = 600;
const int TICKS_PER_GAME_DAY = 14400;
const int TICKS_PER_GAME_YEAR = 5256000;

// Event types that can be triggered each tick
enum class TickEventType {
    NPC_UPDATE = 0,           // Update NPC position, emotion, activity
    PROXIMITY_CHECK = 1,      // Check if any NPC reached player
    PROBLEM_DETECTION = 2,    // Check for NPC problems to queue
    IMMIGRATION_CHECK = 3,    // Check immigration conditions
    BIRTHDAY_CHECK = 4,       // Check if any NPC has birthday
    FACTION_REBELLION = 5,    // Check faction rebellion thresholds
    RESOURCE_UPDATE = 6,      // Update resource production/consumption
    WORLD_STATE_CHECK = 7,    // Check for significant world state changes
    EVENT_TRIGGER = 8,        // Check for natural event triggers
    DIALOGUE_ADVANCE = 9      // Auto-advance dialogue queue
};

// Single tick event to be processed
struct TickEvent {
    TickEventType type;
    int tick;
    std::vector<int> npcIds;   // NPCs affected by this event
    std::string data;          // Event-specific data
};

// Game tick processor - handles continuous simulation updates
class GameTickProcessor {
public:
    static GameTickProcessor* instance();
    
    // Tick control
    void processTick(int currentTick);
    void enqueuTickEvent(const TickEvent& event);
    
    // Event processing
    void processNPCUpdates(int currentTick);
    void processProximityChecks(int currentTick);
    void processProblemDetection(int currentTick);
    void processImmigrationCheck(int currentTick);
    void processBirthdayCheck(int currentTick);
    void processFactionRebellion(int currentTick);
    void processResourceUpdates(int currentTick);
    void processWorldStateChanges(int currentTick);
    void processEventTriggers(int currentTick);
    void processDialogueQueue(int currentTick);
    
    // Statistics
    int getTicksProcessed() const { return ticksProcessed_; }
    float getLastTickDuration() const { return lastTickDuration_; }
    
private:
    GameTickProcessor() = default;
    
    int ticksProcessed_ = 0;
    float lastTickDuration_ = 0.0f;  // Milliseconds
    std::vector<TickEvent> tickEventQueue_;
};

// ============================================================================
// NPC Class - Primary entity representing settlement inhabitants
// ============================================================================

class NPC {
    // Forward declarations for friend classes (serialization support)
    friend class NPCSerializer;
    friend class AdvisorSerializer;

public:
    // Constructor
    NPC();
    explicit NPC(int id);
    NPC(int id, const std::string& name, const Vector3& position);  // Phase 12: Test-friendly constructor

    // Getters/Setters - Basic attributes
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    int getAge() const { return age_; }
    void setAge(int age) { age_ = age; }

    const std::string& getGender() const { return gender_; }
    void setGender(const std::string& gender) { gender_ = gender; }

    // Position and movement
    const Vector3& getPosition() const { return position_; }
    void setPosition(const Vector3& pos) { position_ = pos; }

    const Vector3& getDestination() const { return destination_; }
    void setDestination(const Vector3& dest) { destination_ = dest; }

    const Vector3& getHomeLocation() const { return homeLocation_; }
    void setHomeLocation(const Vector3& home) { homeLocation_ = home; }

    float getSpeed() const { return speed_; }
    void setSpeed(float speed) { speed_ = speed; }

    // Role and affiliation
    const std::string& getRole() const { return role_; }
    void setRole(const std::string& role) { role_ = role; }

    int getFactionId() const { return factionId_; }
    void setFactionId(int id) { factionId_ = id; }

    // Emotional state (from Equations.txt)
    float getImmediateEmotion() const { return immediateEmotion_; }
    void setImmediateEmotion(float emotion) { immediateEmotion_ = emotion; }

    float getShortTermMood() const { return shortTermMood_; }
    void setShortTermMood(float mood) { shortTermMood_ = mood; }

    float getLongTermAttitude() const { return longTermAttitude_; }
    void setLongTermAttitude(float attitude) { longTermAttitude_ = attitude; }

    // Loyalty and personality
    float getLoyalty() const { return loyalty_; }
    void setLoyalty(float loyalty) { loyalty_ = std::max(0.0f, std::min(1.0f, loyalty)); }

    const std::vector<std::string>& getPersonality() const { return personality_; }
    void addPersonalityTrait(const std::string& trait) { personality_.push_back(trait); }

    // Skills
    const std::vector<Skill>& getSkills() const { return skills_; }
    void addSkill(Skill skill) { skills_.push_back(skill); }

    // Activity state
    Activity getActivity() const { return activity_; }
    void setActivity(Activity act) { activity_ = act; }

    // Problem and conversation state
    int getConversationState() const { return conversationState_; }
    void setConversationState(int state) { conversationState_ = state; }

    float getProblemSeverity() const { return problemSeverity_; }
    void setProblemSeverity(float severity) { 
        problemSeverity_ = std::max(0.0f, std::min(1.0f, severity)); 
    }

    const std::string& getProblemDescription() const { return problemDescription_; }
    void setProblemDescription(const std::string& desc) { problemDescription_ = desc; }

    // Problem State Machine (Task #2)
    ProblemState getProblemState() const { return problemState_; }
    void setProblemState(ProblemState state) { problemState_ = state; }

    int getTicksAtProblem() const { return ticksAtProblem_; }
    void incrementTicksAtProblem() { ticksAtProblem_++; }
    void resetTicksAtProblem() { ticksAtProblem_ = 0; }

    int getLastDialogueTick() const { return lastDialogueTick_; }
    void recordDialogueTick(int tick) { lastDialogueTick_ = tick; }

    float getPreviousMood() const { return previousMood_; }
    float getPreviousLoyalty() const { return previousLoyalty_; }

    // Problem calculation methods
    void calculateProblemSeverity();      // Severity = 0.5×|mood_delta| + 0.5×|loyalty_delta|
    void updatePreviousStats();           // Store current mood/loyalty for next delta calculation
    bool isProblemResolved(float foodQty, float scarcityThreshold);  // Check if world improved
    void escalateProblem();               // Increase severity if unresolved >5 days

    // Pathfinding and movement (Phase 3)
    int lastPathCalcTick = 0;
    int stuckAttempts = 0;

    // Public fields for pathfinding system
    Path* currentPath = nullptr;  // Current navigation path (pointer to avoid incomplete type)
    std::vector<Vector3> positionHistory;  // Recent positions for stuck detection
    Activity currentActivity = Activity::IDLE;  // Current activity

    // Movement state (Phase 11)
    std::shared_ptr<MovementState> movementState;

    // Serialization support
    std::vector<uint8_t> serialize() const;
    static NPC deserialize(const std::vector<uint8_t>& data);

    // Comparison and hashing
    bool operator==(const NPC& other) const {
        return id_ == other.id_;
    }

private:
    int id_;
    std::string name_;
    int age_;
    std::string gender_;
    Vector3 position_;
    Vector3 destination_;
    Vector3 homeLocation_;
    float speed_;
    std::string role_;
    int factionId_;

    float immediateEmotion_;
    float shortTermMood_;
    float longTermAttitude_;
    float loyalty_;
    std::vector<std::string> personality_;
    std::vector<Skill> skills_;

    Activity activity_;
    int conversationState_;
    float problemSeverity_;
    std::string problemDescription_;

    // Problem State Machine (Task #2)
    ProblemState problemState_;
    int ticksAtProblem_;
    int lastDialogueTick_;
    float previousMood_;
    float previousLoyalty_;
};

// ============================================================================
// Advisor Class - Specialized NPC with influence and advice mechanisms
// ============================================================================

class Advisor : public NPC {
    // Forward declarations for friend classes (serialization support)
    friend class AdvisorSerializer;

public:
    Advisor();
    explicit Advisor(int id);

    // Getters/Setters
    Specialty getSpecialty() const { return specialty_; }
    void setSpecialty(Specialty spec) { specialty_ = spec; }

    float getInfluenceScore() const { return influenceScore_; }
    void setInfluenceScore(float score) { influenceScore_ = score; }

    float getTrustLevel() const { return trustLevel_; }
    void setTrustLevel(float trust) { trustLevel_ = std::max(0.0f, std::min(1.0f, trust)); }

    float getFactionAlignment() const { return factionAlignment_; }
    void setFactionAlignment(float align) { 
        factionAlignment_ = std::max(-1.0f, std::min(1.0f, align)); 
    }

    Agenda getAgenda() const { return agenda_; }
    void setAgenda(Agenda ag) { agenda_ = ag; }

    float getRiskTolerance() const { return riskTolerance_; }
    void setRiskTolerance(float risk) { 
        riskTolerance_ = std::max(0.0f, std::min(1.0f, risk)); 
    }

    StrategyStyle getStrategyStyle() const { return strategyStyle_; }
    void setStrategyStyle(StrategyStyle style) { strategyStyle_ = style; }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Advisor deserialize(const std::vector<uint8_t>& data);

private:
    Specialty specialty_;
    float influenceScore_;
    float trustLevel_;
    float factionAlignment_;
    Agenda agenda_;
    float riskTolerance_;
    StrategyStyle strategyStyle_;
};

// ============================================================================
// Resource Class - Represents settlements' material assets
// ============================================================================

class Resource {
    // Forward declarations for friend classes (serialization support)
    friend class ResourceSerializer;

public:
    Resource();
    explicit Resource(int id);
    Resource(int id, const std::string& name);  // Phase 12: Test-friendly constructor

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    int getQuantity() const { return quantity_; }
    void setQuantity(int qty) { quantity_ = qty; }
    void addQuantity(int delta) { quantity_ += delta; }

    int getProductionRate() const { return productionRate_; }
    void setProductionRate(int rate) { productionRate_ = rate; }

    int getConsumptionRate() const { return consumptionRate_; }
    void setConsumptionRate(int rate) { consumptionRate_ = rate; }

    int getScarcityThreshold() const { return scarcityThreshold_; }
    void setScarcityThreshold(int threshold) { scarcityThreshold_ = threshold; }

    const Vector3& getLocation() const { return location_; }
    void setLocation(const Vector3& loc) { location_ = loc; }

    bool isScarse() const { return quantity_ < scarcityThreshold_; }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Resource deserialize(const std::vector<uint8_t>& data);

private:
    int id_;
    std::string name_;
    int quantity_;
    int productionRate_;
    int consumptionRate_;
    int scarcityThreshold_;
    Vector3 location_;
};

// ============================================================================
// Faction Class - Represents groups of allied NPCs
// ============================================================================

class Faction {
    // Forward declarations for friend classes (serialization support)
    friend class FactionSerializer;

public:
    Faction();
    explicit Faction(int id);
    Faction(int id, const std::string& name);  // Phase 12: Test-friendly constructor

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    const std::vector<int>& getMemberIds() const { return memberIds_; }
    void addMemberId(int id) { memberIds_.push_back(id); }
    size_t getMemberCount() const { return memberIds_.size(); }

    float getStrength() const { return strength_; }
    void setStrength(float str) { strength_ = str; }

    float getEmergenceProbability() const { return emergenceProbability_; }
    void setEmergenceProbability(float prob) { emergenceProbability_ = prob; }

    Alignment getAlignment() const { return alignment_; }
    void setAlignment(Alignment align) { alignment_ = align; }

    const std::vector<int>& getLeaderIds() const { return leaderIds_; }
    void addLeaderId(int id) { leaderIds_.push_back(id); }

    const Vector3& getHomeLocation() const { return homeLocation_; }
    void setHomeLocation(const Vector3& loc) { homeLocation_ = loc; }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Faction deserialize(const std::vector<uint8_t>& data);

private:
    int id_;
    std::string name_;
    std::vector<int> memberIds_;
    float strength_;
    float emergenceProbability_;
    Alignment alignment_;
    std::vector<int> leaderIds_;
    Vector3 homeLocation_;
};

// ============================================================================
// Event Class - Represents occurrences in the simulation
// ============================================================================

class Event {
    // Forward declarations for friend classes (serialization support)
    friend class EventSerializer;

public:
    Event();
    explicit Event(int id);

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    EventType getType() const { return type_; }
    void setType(EventType type) { type_ = type; }

    int getImpactLevel() const { return impactLevel_; }
    void setImpactLevel(int level) { impactLevel_ = level; }

    const std::vector<int>& getAffectedNPCIds() const { return affectedNPCIds_; }
    void addAffectedNPCId(int id) { affectedNPCIds_.push_back(id); }

    const std::vector<int>& getAffectedResourceIds() const { return affectedResourceIds_; }
    void addAffectedResourceId(int id) { affectedResourceIds_.push_back(id); }

    const Vector3& getLocation() const { return location_; }
    void setLocation(const Vector3& loc) { location_ = loc; }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Event deserialize(const std::vector<uint8_t>& data);

private:
    int id_;
    std::string name_;
    EventType type_;
    int impactLevel_;
    std::vector<int> affectedNPCIds_;
    std::vector<int> affectedResourceIds_;
    Vector3 location_;
};

// ============================================================================
// World State - Tracks overall settlement state for snapshots and LLM context
// ============================================================================

class WorldState {
public:
    WorldState();
    ~WorldState();

    // Snapshot generation
    struct StateSnapshot {
        int tickNumber;
        std::vector<int> significantNPCIds;
        std::vector<int> affectedFactionIds;
        std::vector<int> changedResourceIds;
        std::vector<int> triggeredEventIds;
    };

    // Record significant changes
    void recordNPCChange(int npcId, float moodDelta, float loyaltyDelta);
    void recordFactionChange(int factionId, float loyaltyDelta);
    void recordResourceChange(int resourceId, int quantityDelta);
    void recordEvent(int eventId);

    // Generate snapshot for LLM
    StateSnapshot generateSnapshot(int tickNumber);

    // Threshold configuration
    void setMoodThreshold(float threshold) { moodThreshold_ = threshold; }
    void setLoyaltyThreshold(float threshold) { loyaltyThreshold_ = threshold; }
    void setResourceThreshold(int threshold) { resourceThreshold_ = threshold; }

    float getMoodThreshold() const { return moodThreshold_; }
    float getLoyaltyThreshold() const { return loyaltyThreshold_; }
    int getResourceThreshold() const { return resourceThreshold_; }

    // Clear state for next snapshot
    void clear();

private:
    // Tracking structures for changes this tick
    std::vector<int> changedNPCs_;
    std::vector<float> npcMoodDeltas_;
    std::vector<float> npcLoyaltyDeltas_;
    
    std::vector<int> changedFactions_;
    std::vector<float> factionLoyaltyDeltas_;
    
    std::vector<int> changedResources_;
    std::vector<int> resourceQuantityDeltas_;
    
    std::vector<int> triggeredEvents_;

    // Thresholds for significance
    float moodThreshold_;              // Default 0.2
    float loyaltyThreshold_;           // Default 0.15
    int resourceThreshold_;            // Default change of 50 units
};

// ============================================================================
// Culture and Religion (simplified for now)
// ============================================================================

struct Culture {
    std::string name;
    std::vector<std::string> norms;
    std::vector<std::string> traditions;
    float evolutionRate = 0.01f;
};

struct Religion {
    std::string name;
    std::vector<std::string> doctrines;
    std::vector<int> followerIds;
    float schismProbability = 0.05f;
};

}  // namespace TLS

#endif  // CORE_H
