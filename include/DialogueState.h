#pragma once

#include <map>
#include <vector>
#include <string>
#include <deque>

namespace TLS {

// Forward declarations
class NPC;
class WorldState;

/**
 * @enum DialogueState
 * @brief NPC dialogue state progression machine
 */
enum class DialogueState {
    IDLE = 0,                   // No problem, can't be talked to
    PROBLEM_DETECTED = 1,       // NPC recognizes problem internally
    PATHFINDING_TO_PLAYER = 2,  // NPC moving toward player
    PROXIMITY_REACHED = 3,      // Within 5 units, ready to talk
    IN_DIALOGUE = 4,            // Frozen, player conversing
    RESPONSE_ACKNOWLEDGED = 5,  // Player responded
    RESOLVED = 6,               // Problem cleared
    COOLDOWN = 7                // Waiting before can re-initiate
};

/**
 * @struct NpcDialogueState
 * @brief Tracks individual NPC dialogue state and history
 */
struct NpcDialogueState {
    int npcId = -1;
    DialogueState currentState = DialogueState::IDLE;
    int stateEnteredTick = 0;

    // Problem tracking
    float problemSeverity = 0.0f;
    std::string problemDescription;

    // Dialogue history
    int conversationCount = 0;              // How many times approached player
    int lastConversationTick = -1;
    std::vector<std::string> conversationHistory;

    // Escalation
    int escalationLevel = 0;  // Increases if problem unresolved

    NpcDialogueState() = default;
    explicit NpcDialogueState(int id) : npcId(id) {}
};

/**
 * @class NpcDialogueTracker
 * @brief Singleton managing dialogue states for all NPCs
 *
 * Tracks:
 * - Individual NPC dialogue state progression
 * - Problem severity and descriptions
 * - Conversation history per NPC
 * - Escalation levels for persistent problems
 */
class NpcDialogueTracker {
public:
    static NpcDialogueTracker& getInstance();

    // State management
    void updateNpcDialogueState(int npcId, DialogueState newState, int currentTick = 0);
    DialogueState getNpcDialogueState(int npcId) const;
    NpcDialogueState* getNpcDialogueInfo(int npcId);
    const NpcDialogueState* getNpcDialogueInfo(int npcId) const;

    // Problem tracking
    void setProblemSeverity(int npcId, float severity);
    float getProblemSeverity(int npcId) const;
    void setProblemDescription(int npcId, const std::string& description);
    std::string getProblemDescription(int npcId) const;

    // Conversation history
    void recordConversation(int npcId, const std::string& dialogue);
    const std::vector<std::string>& getConversationHistory(int npcId) const;
    int getConversationCount(int npcId) const;

    // Escalation
    void incrementEscalationLevel(int npcId);
    int getEscalationLevel(int npcId) const;

    // Utility
    bool canNpcReinitiate(int npcId, int currentTick, int cooldownTicks = 14400) const;
    void resetNpcDialogueState(int npcId);
    void clearAllDialogueStates();

    // Debug
    std::string getStateString(DialogueState state) const;

private:
    NpcDialogueTracker() = default;
    NpcDialogueTracker(const NpcDialogueTracker&) = delete;
    NpcDialogueTracker& operator=(const NpcDialogueTracker&) = delete;

    std::map<int, NpcDialogueState> dialogueStates_;
    std::vector<std::string> emptyHistory_;
};

}  // namespace TLS
