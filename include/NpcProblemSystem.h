#pragma once

#include <string>
#include <vector>

namespace TLS {

// Forward declarations
class NPC;
class WorldState;

/**
 * @enum ProblemType
 * @brief Categories of NPC problems that trigger dialogue
 */
enum class ProblemType {
    RESOURCE_SCARCITY = 0,   // Food/wood/water shortage
    FACTION_CONFLICT = 1,    // Factional tensions
    MORAL_CRISIS = 2,        // Despair, hopelessness
    PERSONAL_GRIEVANCE = 3,  // Low loyalty, disrespect
    UNKNOWN = 4
};

/**
 * @struct NpcProblem
 * @brief Structured representation of an NPC's current problem
 */
struct NpcProblem {
    ProblemType type = ProblemType::UNKNOWN;
    std::string description;           // Short problem summary
    float severity = 0.0f;             // 0-1 scale
    int affectedFactionId = -1;        // For faction-related problems
    int rootCauseResourceId = -1;      // For resource-related problems
    bool isResolvable = false;         // Can player actually fix it?
    std::vector<std::string> solutions; // Suggested player actions

    NpcProblem() = default;
    explicit NpcProblem(ProblemType t) : type(t) {}
};

/**
 * @class NpcProblemSystem
 * @brief Identifies NPC problems and generates dialogue content
 *
 * Core responsibilities:
 * - Identify NPC problems from world state (scarcity, faction conflict, etc.)
 * - Generate natural language problem statements
 * - Determine if problem is resolvable by player
 * - Check if problem has been resolved
 * - Provide suggested solutions/dialogue options
 */
class NpcProblemSystem {
public:
    // Problem identification
    static NpcProblem identifyNpcProblem(
        const NPC& npc,
        const WorldState& world
    );

    // Problem state
    static bool isProblemResolved(
        const NPC& npc,
        const NpcProblem& problem,
        const WorldState& world
    );

    // Dialogue generation - simplified for testing
    static std::string generateProblemStatement(const NpcProblem& problem);
    static std::string generateProblemDescription(const NpcProblem& problem);

    // Solutions/suggestions
    static std::vector<std::string> getSuggestedSolutions(
        const NPC& npc,
        const NpcProblem& problem
    );

    // Helper: problem type to string
    static std::string problemTypeToString(ProblemType type);
    static ProblemType stringToProblemType(const std::string& str);

private:
    // Problem identification helpers
    static NpcProblem checkResourceScarcity(const NPC& npc, const WorldState& world);
    static NpcProblem checkFactionConflict(const NPC& npc, const WorldState& world);
    static NpcProblem checkMoralCrisis(const NPC& npc, const WorldState& world);
    static NpcProblem checkPersonalGrievance(const NPC& npc, const WorldState& world);

    // Problem resolution helpers
    static bool isResourceScarcityResolved(
        const NPC& npc,
        const NpcProblem& problem,
        const WorldState& world
    );
    static bool isFactionConflictResolved(
        const NPC& npc,
        const NpcProblem& problem,
        const WorldState& world
    );
    static bool isMoralCrisisResolved(const NPC& npc);
    static bool isPersonalGrievanceResolved(const NPC& npc);

    // Dialogue templates
    static std::string generateResourceScarcityDialogue(const NPC& npc);
    static std::string generateFactionConflictDialogue(const NPC& npc);
    static std::string generateMoralCrisisDialogue(const NPC& npc);
    static std::string generatePersonalGrievanceDialogue(const NPC& npc);

    // Constants for scarcity thresholds
    static constexpr int FOOD_SCARCITY_THRESHOLD = 150;
    static constexpr float LOYALTY_GRIEVANCE_THRESHOLD = 0.3f;
    static constexpr float LOYALTY_CONFLICT_THRESHOLD = 0.2f;
    static constexpr float MOOD_CRISIS_THRESHOLD = 0.2f;
};

}  // namespace TLS
