#pragma once

#include "Core.h"
#include "Registries.h"
#include "Player.h"
#include "World.h"
#include "ProximityDetection.h"
#include "ActivitySystem.h"
#include "NPCMovement.h"
#include "Pathfinding.h"
#include "LLM.h"
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <memory>
#include <random>

namespace TLS {

// Forward declarations
class LLMManager;
struct InterpretedDecision;

/**
 * @class SimulationManager
 * @brief Central coordinator for main simulation loop
 *
 * SimulationManager integrates all Phase 1, 2, and 3 systems into a continuous
 * real-time simulation. It:
 * - Updates player movement and rotation
 * - Updates all NPC positions, activities, and emotions
 * - Detects proximity-based interactions
 * - Monitors world state for significant changes
 * - Manages conversation queue and dialogue flow
 * - Tracks game time (ticks, hours, days)
 * - Interfaces with LLM for decision interpretation and narrative generation
 *
 * Architecture: Event-driven continuous loop, not time-based schedules
 */
class SimulationManager
{
public:
    // Singleton instance
    static SimulationManager& getInstance();

    // Initialization
    void initialize(NPCRegistry& registry, FactionRegistry& factionRegistry, 
                   ResourceRegistry& resourceRegistry, AdvisorRegistry& advisorRegistry,
                   const World& world, const WaypointGraph& graph,
                   const LLMConfig& llmConfig);

    // Main update loop (called every frame)
    void tick(float deltaTime);

    // Getters
    int getTick() const { return tick_; }
    int getCurrentTick() const { return tick_; }  // Phase 12 alias
    float getGameTime() const { return gameTime_; }
    int getGameHour() const;
    int getGameMinute() const;
    int getGameDay() const;
    int getGameSeason() const;
    std::string getTimeString() const;
    
    Player& getPlayer() { return player_; }
    const Player& getPlayer() const { return player_; }
    WorldState& getWorldState() { return worldState_; }
    const WorldState& getWorldState() const { return worldState_; }
    NPCRegistry& getNPCRegistry() { return *registry_; }
    FactionRegistry& getFactionRegistry() { return *factionRegistry_; }
    ResourceRegistry& getResourceRegistry() { return *resourceRegistry_; }
    AdvisorRegistry& getAdvisorRegistry() { return *advisorRegistry_; }

    // Player input (called from UI layer)
    void processPlayerInput(const std::string& command);
    void queuePlayerInput(const std::string& command);  // Phase 12: async input queue
    bool hasPlayerInput() const;
    std::string getNextPlayerInput();

    // Conversation queue management
    struct ConversationQueueEntry
    {
        int npcId;
        float problemSeverity;
        float influenceScore;
        float leadershipBonus;
        int tickArrived;
    };

    std::vector<ConversationQueueEntry> getConversationQueue() const { return conversationQueue_; }
    int getConversationQueueSize() const { return static_cast<int>(conversationQueue_.size()); }
    bool isInConversation() const { return currentConversationNpcId_ != -1; }
    int getCurrentConversationNpcId() const { return currentConversationNpcId_; }
    std::string getCurrentConversationText() const { return currentConversationText_; }
    void setInConversation(bool inConversation);  // Phase 12 setter for testing

    // Conversation responses
    void respondToCurrentConversation(const std::string& response);
    void endCurrentConversation();

    // World state queries
    bool hasSignificantWorldStateChange() const { return hasSignificantChange_; }
    std::vector<std::string> getActiveNarrativeIssues() const { return activeNarrativeIssues_; }

    // World state monitoring (Phase 8: Narrative Generation)
    void monitorWorldStateChanges();
    bool detectSignificantWorldStateChange();
    void triggerNarrativeGeneration();

    // Serialization
    bool saveToBinary(const std::string& filepath) const;
    bool loadFromBinary(const std::string& filepath);

    // Replay system
    void enableReplay(bool enable) { replayMode_ = enable; }
    bool isReplayMode() const { return replayMode_; }
    void logLLMCall(const std::string& callType, const std::string& prompt, const std::string& response);

private:
    // Private constructor (singleton)
    SimulationManager();
    SimulationManager(const SimulationManager&) = delete;
    SimulationManager& operator=(const SimulationManager&) = delete;

    // References to managed objects
    NPCRegistry* registry_ = nullptr;
    FactionRegistry* factionRegistry_ = nullptr;
    ResourceRegistry* resourceRegistry_ = nullptr;
    AdvisorRegistry* advisorRegistry_ = nullptr;
    const World* world_ = nullptr;
    const WaypointGraph* graph_ = nullptr;
    LLMManager* llmManager_ = nullptr;

    // Player and world state
    Player player_;
    WorldState worldState_;

    // Time tracking
    int tick_ = 0;
    float gameTime_ = 0.0f;  // In game minutes
    int globalSeed_ = 42;    // For deterministic RNG

    // Player input queue (Phase 12)
    std::queue<std::string> playerInputQueue_;

    // Conversation management
    std::vector<ConversationQueueEntry> conversationQueue_;
    int currentConversationNpcId_ = -1;
    int conversationDelayTicks_ = 0;
    std::string currentConversationText_;
    const int CONVERSATION_DELAY = 30;  // ~3 seconds at 10 ticks per game minute

    // Previous state tracking (for delta detection)
    struct NPCStateSnapshot
    {
        float mood = 0.5f;
        float loyalty = 0.5f;
        float immediateEmotion = 0.5f;
        Vector3 position;
        int lastStateChangeTick = 0;
    };
    std::map<int, NPCStateSnapshot> previousNPCState_;

    // World state change monitoring
    bool hasSignificantChange_ = false;
    std::vector<std::string> activeNarrativeIssues_;
    int lastLLMSnapshotTick_ = -100;  // Debounce: only one snapshot per 30 ticks
    const int LLM_SNAPSHOT_DEBOUNCE = 30;

    // NPC position history (for stuck detection)
    std::map<int, std::vector<Vector3>> npcPositionHistory_;
    const int MAX_POSITION_HISTORY = 30;  // Store last 30 positions

    // Replay and logging
    bool replayMode_ = false;
    std::map<int, std::pair<std::string, std::string>> llmCallLog_;  // tick -> (prompt, response)

    // RNG for deterministic randomness
    std::mt19937 rng_;

    // ======== PRIVATE METHODS ========

    // Update systems
    void updatePlayerMovement(float deltaTime);
    void updateNPCMovement(float deltaTime);
    void updateNPCActivities();
    void updateNPCEmotions();

    // Proximity and dialogue
    void checkProximityInteractions();
    void processConversationQueue();

    // Problem severity calculation
    float calculateProblemSeverity(const NPC& npc, float moodDelta, float loyaltyDelta) const;
    
    // Conversation queue management
    void addNPCToConversationQueue(const NPC& npc, float severity, float influence, float leadershipBonus);
    void sortConversationQueue();
    void displayNextConversation();

    // Decision processing
    void applyDecisionConsequences(const NPC& npc, const InterpretedDecision& decision);
    void updateNPCEmotion(NPC& npc, const std::string& tone);
    void updateNPCMood(NPC& npc);
    void updateNPCAttitude(NPC& npc);
    void updateFactionLoyalties(int targetNpcId, float loyaltyDelta);

    // LLM integration
    void requestDecisionInterpretation(const std::string& playerInput);
    void requestNarrativeGeneration(const std::vector<int>& affectedNpcIds);
    void onDecisionInterpretationComplete(const InterpretedDecision& decision);
    void onNarrativeGenerationComplete(const std::vector<std::string>& narrativeIssues);
};

}  // namespace TLS
