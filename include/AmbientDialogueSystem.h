#pragma once

#include "Core.h"
#include "World.h"
#include "LLMProvider.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <queue>
#include <memory>

using namespace TLS;

namespace TypedLeadership {

// ============================================================================
// Enums & Constants
// ============================================================================

enum class DialogueTopic {
    WORK = 0,
    CONCERNS = 1,
    TRADE = 2,
    GOSSIP = 3,
    MORALE = 4,
    FOOD_SHORTAGE = 5,
    IMMIGRATION = 6,
    FACTION_CONFLICT = 7,
    CELEBRATION = 8,
    UNKNOWN = 9
};

enum class DialogueTone {
    CASUAL = 0,
    SERIOUS = 1,
    CONCERNED = 2,
    EXCITED = 3,
    HOSTILE = 4,
    DIPLOMATIC = 5,
    UNKNOWN = 6
};

enum class CascadeType {
    FACTION_TENSION = 0,
    GOSSIP_PROPAGATION = 1,
    ALLIANCE_FORMATION = 2,
    LEADERSHIP_AWARENESS = 3,
    NONE = 4
};

// ============================================================================
// Data Structures
// ============================================================================

struct NPCPair {
    int npcId1;
    int npcId2;
    float compatibility;
    int lastConversationTick;
    float proximityDistance;
};

struct DialogueContext {
    int npcId1;
    int npcId2;
    DialogueTopic topic;
    DialogueTone tone;
    std::string location;
    float npc1Mood;
    float npc2Mood;
    std::string npc1Name;
    std::string npc2Name;
    std::string npc1Role;
    std::string npc2Role;
    int factionId1;
    int factionId2;
    int currentTick;
};

struct GeneratedDialogue {
    std::string npc1Dialogue;
    std::string npc2Dialogue;
    std::string implication;
    float qualityScore;
    bool isLLMGenerated;
    int generatedAtTick;
};

struct ConversationRecord {
    int npcId1;
    int npcId2;
    DialogueContext context;
    GeneratedDialogue dialogue;
    std::vector<CascadeType> cascadesTriggered;
    int recordedAtTick;
};

struct ConversationQueueEntry {
    int npcId1;
    int npcId2;
    float priorityScore;
    DialogueTopic suggestedTopic;
};

// ============================================================================
// Main Ambient Dialogue System Class
// ============================================================================

class AmbientDialogueSystem {
public:
    static AmbientDialogueSystem& getInstance();
    
    // Initialization & Lifecycle
    void initialize();
    void shutdown();
    
    // Internal template initialization
    void initializeOfflineTemplates();
    
    // Main Conversation Loop
    std::vector<NPCPair> findConversationPairs(
        const std::vector<NPC*>& activeNPCs,
        int currentTick
    );
    
    std::optional<DialogueContext> selectTopic(
        const NPC& npc1,
        const NPC& npc2,
        const WorldState& worldState,
        int currentTick
    );
    
    std::optional<GeneratedDialogue> generateDialogue(
        const DialogueContext& context,
        LLMProvider* llmProvider,
        bool allowLLM = true
    );
    
    void validateAndStoreConversation(
        const DialogueContext& context,
        const GeneratedDialogue& dialogue,
        int currentTick
    );
    
    // Compatibility & Scoring
    float calculateCompatibilityScore(
        const NPC& npc1,
        const NPC& npc2,
        const WorldState& worldState,
        int currentTick
    );
    
    // Topic Selection
    DialogueTopic selectTopicByWeight(
        const NPC& npc1,
        const NPC& npc2,
        const WorldState& worldState,
        int currentTick
    );
    
    DialogueTone selectToneByMood(float avgMood);
    
    // Dialogue Generation
    std::string buildLLMPrompt(const DialogueContext& context);
    
    std::optional<GeneratedDialogue> parseLLMResponse(
        const std::string& llmOutput,
        const DialogueContext& context
    );
    
    GeneratedDialogue generateOfflineDialogue(
        const DialogueContext& context
    );
    
    // Quality Control
    float scoreDialogueQuality(
        const GeneratedDialogue& dialogue,
        const DialogueContext& context
    );
    
    bool shouldRejectDialogue(float qualityScore);
    
    // Cascade Detection
    std::vector<CascadeType> detectCascades(
        const GeneratedDialogue& dialogue,
        const NPC& npc1,
        const NPC& npc2,
        WorldState& worldState
    );
    
    void processCascade(
        CascadeType cascadeType,
        const NPC& npc1,
        const NPC& npc2,
        const GeneratedDialogue& dialogue,
        WorldState& worldState
    );
    
    // Conversation Storage & History
    void storeConversation(
        const ConversationRecord& record
    );
    
    std::vector<ConversationRecord> getConversationHistory(
        int npcId1,
        int npcId2,
        int maxDaysBack = 10
    );
    
    std::vector<ConversationRecord> getAllConversations(
        int maxRecent = 100
    );
    
    // Queue Management
    void enqueueConversation(
        int npcId1,
        int npcId2,
        float priorityScore,
        DialogueTopic suggestedTopic
    );
    
    std::optional<ConversationQueueEntry> dequeueTopPriority();
    
    size_t getQueueSize() const;
    
    // Utility & Debugging
    std::string dialogueTopicToString(DialogueTopic topic) const;
    std::string dialogueToneToString(DialogueTone tone) const;
    std::string cascadeTypeToString(CascadeType cascadeType) const;
    
    void printConversationBuffer(int maxRecent = 20) const;
    void printQueueStatus() const;
    
    // Performance Metrics
    struct PerformanceMetrics {
        int totalPairsEvaluated;
        int validPairsFound;
        int conversationsGenerated;
        int llmCallsAttempted;
        int llmCallsSucceeded;
        float avgQualityScore;
        float totalExecutionTimeMs;
    };
    
    PerformanceMetrics getMetrics() const;
    void resetMetrics();
    
private:
    // Singleton pattern
    AmbientDialogueSystem();
    ~AmbientDialogueSystem();
    
    AmbientDialogueSystem(const AmbientDialogueSystem&) = delete;
    AmbientDialogueSystem& operator=(const AmbientDialogueSystem&) = delete;
    
    // Internal methods
    bool isCompatiblePair(
        const NPC& npc1,
        const NPC& npc2,
        int currentTick
    );
    
    float getProximityScore(float distance);
    float getFactionBonus(const NPC& npc1, const NPC& npc2);
    float getMoodCompatibility(const NPC& npc1, const NPC& npc2);
    float getCooldownBonus(int currentTick, int lastConversationTick);
    float getActivityBonus(const NPC& npc1, const NPC& npc2);
    
    std::string getConversationTopicKey(
        DialogueTopic topic,
        DialogueTone tone
    );
    
    std::string buildOfflineTemplate(
        DialogueTopic topic,
        DialogueTone tone,
        const NPC& npc1,
        const NPC& npc2
    );
    
    bool containsGossipKeywords(const std::string& dialogue);
    bool containsLeadershipKeywords(const std::string& dialogue);
    
    float analyzeDialogueSentiment(const std::string& dialogue);
    float analyzeRelevanceScore(
        const std::string& dialogue,
        const DialogueContext& context
    );
    float analyzeGrammarScore(const std::string& dialogue);
    float analyzeSentimentScore(
        const std::string& dialogue,
        DialogueTone expectedTone
    );
    float analyzeLengthScore(const std::string& dialogue);
    
    // Data members
    static AmbientDialogueSystem* instance_;
    
    // Conversation storage (circular buffer, max 1000 conversations)
    std::vector<ConversationRecord> conversationBuffer_;
    size_t bufferHead_;
    static constexpr size_t BUFFER_SIZE = 1000;
    
    // Pair conversation history (npcId1, npcId2) -> last tick
    std::unordered_map<std::string, int> pairConversationMap_;
    
    // Conversation queue
    std::queue<ConversationQueueEntry> conversationQueue_;
    static constexpr size_t MAX_QUEUE_SIZE = 100;
    
    // Offline dialogue templates (topic+tone -> list of dialogue pairs)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> offlineTemplates_;
    
    // Bond tracking for alliance formation (npcId1, npcId2) -> bond_strength
    std::unordered_map<std::string, float> npcBondStrength_;
    
    // Faction tension tracking (factionId1, factionId2) -> tension_score
    std::unordered_map<std::string, float> factionTensionMap_;
    
    // Performance metrics
    PerformanceMetrics metrics_;
    
    // Constants
    static constexpr float PROXIMITY_RANGE = 15.0f;
    static constexpr float MIN_COMPATIBILITY = 0.5f;
    static constexpr int COOLDOWN_TICKS = 14400;  // 5 game minutes
    static constexpr int HALF_COOLDOWN_TICKS = 7200;  // 2.5 game minutes
    static constexpr float MIN_QUALITY_SCORE = 0.7f;
    static constexpr int ALLIANCE_FORMATION_THRESHOLD = 5;  // 5 conversations
    static constexpr int ALLIANCE_THRESHOLD_TICKS = 144000;  // 10 game days
    static constexpr float ALLIANCE_BOND_THRESHOLD = 0.7f;
    static constexpr float FACTION_TENSION_THRESHOLD = 0.4f;
    static constexpr int GOSSIP_PROPAGATION_RADIUS = 20;
    
    // Weights for compatibility scoring
    static constexpr float W_PROXIMITY = 0.35f;
    static constexpr float W_FACTION = 0.25f;
    static constexpr float W_MOOD = 0.20f;
    static constexpr float W_COOLDOWN = 0.15f;
    static constexpr float W_ACTIVITY = 0.05f;
    
    // Weights for dialogue quality
    static constexpr float W_GRAMMAR = 0.4f;
    static constexpr float W_RELEVANCE = 0.25f;
    static constexpr float W_SENTIMENT = 0.2f;
    static constexpr float W_LENGTH = 0.15f;
};

}  // namespace TypedLeadership
