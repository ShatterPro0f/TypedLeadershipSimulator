#pragma once

#include "Core.h"
#include "Registries.h"
#include <string>
#include <vector>
#include <map>

namespace TLS {

// Forward declarations
struct InterpretedDecision;

/**
 * @class DialogueSystem
 * @brief Manages NPC-player conversation flow and state machine
 *
 * Handles:
 * - NPC state machine (IDLE -> PROBLEM_DETECTED -> PATHFINDING -> DIALOGUE -> RESOLVED)
 * - Problem severity calculation for dialogue triggers
 * - Conversation queue entry management
 * - Dialogue generation and NPC response feedback
 * - Problem resolution criteria and persistence mechanics
 */
class DialogueSystem {
public:
    // NPC dialogue state machine
    enum DialogueState {
        IDLE = 0,              // No problem
        PROBLEM_DETECTED = 1,  // NPC recognizes problem (severity > threshold)
        PATHFINDING = 2,       // NPC moving to player
        IN_DIALOGUE = 3,       // Conversation active
        ACKNOWLEDGED = 4,      // Player responded (may not solve problem)
        RESOLVED = 5,          // Problem solved
        PERSISTENT = 6         // Problem unresolved, cooldown before re-initiating
    };

    struct ProblemState {
        DialogueState state = IDLE;
        float severity = 0.0f;
        int ticksAtState = 0;
        int timeSinceLastDialogue = -1;  // -1 = never
        int escalationLevel = 0;         // How many times re-initiated
        std::string problemDescription;
        std::string category;            // "resource_shortage", "faction_conflict", "personal_grievance", etc.
    };

    static DialogueSystem& getInstance();

    // Initialize with problem threshold
    void initialize(float severityThreshold = 0.3f);

    // Update dialogue states for all NPCs
    void updateDialogueStates(NPCRegistry& registry, int currentTick);

    // Get current problem state for NPC
    ProblemState getProblemState(const NPC& npc) const;

    // Check if NPC should initiate dialogue
    bool shouldInitiateDialogue(const NPC& npc, int currentTick) const;

    // Generate dialogue text for NPC
    std::string generateDialogueText(const NPC& npc, const ProblemState& problem);

    // Process player response to dialogue
    void processPlayerResponse(NPC& npc, const std::string& playerResponse,
                              const InterpretedDecision& decision, int currentTick);

    // Mark problem as resolved
    void resolveProblem(NPC& npc);

    // Mark problem as acknowledged (may not be fully solved)
    void acknowledgeProblem(NPC& npc);

    // Check if problem is truly resolved
    bool isProblemResolved(const NPC& npc, const WorldState& worldState) const;

    // Get dialogue cooldown period
    int getDialogueCooldownTicks() const { return DIALOGUE_COOLDOWN_TICKS; }

private:
    DialogueSystem();
    DialogueSystem(const DialogueSystem&) = delete;
    DialogueSystem& operator=(const DialogueSystem&) = delete;

    float severityThreshold_ = 0.3f;

    // Problem state tracking per NPC
    std::map<int, ProblemState> npcProblems_;

    // Cooldown management
    static constexpr int DIALOGUE_COOLDOWN_TICKS = 14400;  // 1 game day
    static constexpr int MAX_PERSISTENCE_ATTEMPTS = 5;
    static constexpr float ESCALATION_INCREMENT = 0.1f;

    // Problem severity calculation
    float calculateProblemSeverity(const NPC& npc, const WorldState& worldState);

    // Determine problem category
    std::string determineProblemCategory(const NPC& npc, const WorldState& worldState);

    // Resolution criteria (problem-specific)
    bool isResourceShortageSolved(const NPC& npc, const WorldState& worldState) const;
    bool isFactionConflictSolved(const NPC& npc, const WorldState& worldState) const;
    bool isPersonalGrievanceSolved(const NPC& npc) const;
    bool isReligiousCrisisSolved(const NPC& npc, const WorldState& worldState) const;

    // Dialogue templates
    std::string generateResourceShortageDialogue(const NPC& npc);
    std::string generateFactionConflictDialogue(const NPC& npc);
    std::string generatePersonalGrievanceDialogue(const NPC& npc);
    std::string generateReligiousCrisisDialogue(const NPC& npc);
};

}  // namespace TLS
