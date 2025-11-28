#include "NpcProblemSystem.h"
#include "Core.h"
#include <sstream>

namespace TLS {

// Problem identification - stub implementation
NpcProblem NpcProblemSystem::identifyNpcProblem(
    const NPC& npc,
    const WorldState& world) {
    // Return a generic problem for now
    NpcProblem problem(ProblemType::RESOURCE_SCARCITY);
    problem.description = "I have a concern that needs your attention.";
    problem.severity = 0.5f;
    problem.isResolvable = true;
    problem.solutions = {"Address this concern", "Dismiss for now", "Ask for more details"};
    return problem;
}

bool NpcProblemSystem::isProblemResolved(
    const NPC& npc,
    const NpcProblem& problem,
    const WorldState& world) {
    return false;  // Stub
}

std::string NpcProblemSystem::generateProblemStatement(const NpcProblem& problem) {
    std::stringstream ss;
    ss << "Problem: ";
    if (!problem.description.empty()) {
        ss << problem.description;
    } else {
        ss << problemTypeToString(problem.type);
    }
    return ss.str();
}

std::string NpcProblemSystem::generateProblemDescription(const NpcProblem& problem) {
    if (!problem.description.empty()) {
        return problem.description;
    }
    // Generate default descriptions based on problem type
    switch (problem.type) {
        case ProblemType::RESOURCE_SCARCITY:
            return "We are running low on essential resources.";
        case ProblemType::FACTION_CONFLICT:
            return "Our faction is facing serious challenges.";
        case ProblemType::MORAL_CRISIS:
            return "There is a moral issue affecting our settlement.";
        case ProblemType::PERSONAL_GRIEVANCE:
            return "I have a personal concern I need to discuss.";
        case ProblemType::UNKNOWN:
        default:
            return "I have an important matter to discuss.";
    }
}

std::vector<std::string> NpcProblemSystem::getSuggestedSolutions(
    const NPC& npc,
    const NpcProblem& problem) {
    return problem.solutions;
}

std::string NpcProblemSystem::problemTypeToString(ProblemType type) {
    switch (type) {
        case ProblemType::RESOURCE_SCARCITY:
            return "RESOURCE_SCARCITY";
        case ProblemType::FACTION_CONFLICT:
            return "FACTION_CONFLICT";
        case ProblemType::MORAL_CRISIS:
            return "MORAL_CRISIS";
        case ProblemType::PERSONAL_GRIEVANCE:
            return "PERSONAL_GRIEVANCE";
        case ProblemType::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

ProblemType NpcProblemSystem::stringToProblemType(const std::string& str) {
    if (str == "RESOURCE_SCARCITY") return ProblemType::RESOURCE_SCARCITY;
    if (str == "FACTION_CONFLICT") return ProblemType::FACTION_CONFLICT;
    if (str == "MORAL_CRISIS") return ProblemType::MORAL_CRISIS;
    if (str == "PERSONAL_GRIEVANCE") return ProblemType::PERSONAL_GRIEVANCE;
    return ProblemType::UNKNOWN;
}

// Private stub implementations
NpcProblem NpcProblemSystem::checkResourceScarcity(const NPC& npc, const WorldState& world) {
    NpcProblem p(ProblemType::RESOURCE_SCARCITY);
    p.description = "Resource issue";
    p.severity = 0.3f;
    return p;
}

NpcProblem NpcProblemSystem::checkFactionConflict(const NPC& npc, const WorldState& world) {
    NpcProblem p(ProblemType::FACTION_CONFLICT);
    p.description = "Faction issue";
    p.severity = 0.3f;
    p.affectedFactionId = 0;
    return p;
}

NpcProblem NpcProblemSystem::checkMoralCrisis(const NPC& npc, const WorldState& world) {
    NpcProblem p(ProblemType::MORAL_CRISIS);
    p.description = "Morale issue";
    p.severity = 0.3f;
    return p;
}

NpcProblem NpcProblemSystem::checkPersonalGrievance(const NPC& npc, const WorldState& world) {
    NpcProblem p(ProblemType::PERSONAL_GRIEVANCE);
    p.description = "Personal grievance";
    p.severity = 0.3f;
    p.affectedFactionId = 0;
    return p;
}

bool NpcProblemSystem::isResourceScarcityResolved(
    const NPC& npc,
    const NpcProblem& problem,
    const WorldState& world) {
    return false;
}

bool NpcProblemSystem::isFactionConflictResolved(
    const NPC& npc,
    const NpcProblem& problem,
    const WorldState& world) {
    return false;
}

bool NpcProblemSystem::isMoralCrisisResolved(const NPC& npc) {
    return false;
}

bool NpcProblemSystem::isPersonalGrievanceResolved(const NPC& npc) {
    return false;
}

std::string NpcProblemSystem::generateResourceScarcityDialogue(const NPC& npc) {
    return "We need resources.";
}

std::string NpcProblemSystem::generateFactionConflictDialogue(const NPC& npc) {
    return "Our faction is dissatisfied.";
}

std::string NpcProblemSystem::generateMoralCrisisDialogue(const NPC& npc) {
    return "People are losing morale.";
}

std::string NpcProblemSystem::generatePersonalGrievanceDialogue(const NPC& npc) {
    return "I have a personal issue with you.";
}

}  // namespace TLS
