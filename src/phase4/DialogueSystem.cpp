#include "DialogueSystem.h"
#include "GameTime.h"
#include <sstream>
#include <cmath>

using namespace TLS;

DialogueSystem::DialogueSystem()
{
    severityThreshold_ = 0.3f;  // Default threshold
}

DialogueSystem& DialogueSystem::getInstance()
{
    static DialogueSystem instance;
    return instance;
}

void DialogueSystem::initialize(float severityThreshold)
{
    severityThreshold_ = severityThreshold;
}

void DialogueSystem::updateDialogueStates(NPCRegistry& registry, int currentTick)
{
    auto allNPCs = registry.getAllNPCs();
    for (auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        auto it = npcProblems_.find(npc->getId());
        if (it == npcProblems_.end())
        {
            // Initialize problem state
            ProblemState newProblem;
            newProblem.state = IDLE;
            newProblem.severity = 0.0f;
            newProblem.ticksAtState = 0;
            newProblem.timeSinceLastDialogue = -1;
            newProblem.escalationLevel = 0;
            newProblem.problemDescription = "";
            newProblem.category = "";
            npcProblems_[npc->getId()] = newProblem;
            it = npcProblems_.find(npc->getId());
        }

        ProblemState& problem = it->second;
        problem.ticksAtState++;

        // Update problem severity based on world state
        // (For now, simplified calculation. Full version would use WorldState)
        problem.severity = std::abs(npc->getShortTermMood() - 0.5f);

        // State machine transitions
        if (problem.state == IDLE && problem.severity >= severityThreshold_)
        {
            problem.state = PROBLEM_DETECTED;
            problem.ticksAtState = 0;
            problem.category = determineProblemCategory(*npc, WorldState());
        }
        else if (problem.state == RESOLVED || problem.state == PERSISTENT)
        {
            // Check cooldown
            if (problem.timeSinceLastDialogue >= 0)
            {
                problem.timeSinceLastDialogue++;
                if (problem.timeSinceLastDialogue >= DIALOGUE_COOLDOWN_TICKS)
                {
                    if (problem.severity >= severityThreshold_)
                    {
                        problem.state = PROBLEM_DETECTED;
                        problem.escalationLevel++;
                        problem.severity += ESCALATION_INCREMENT;
                        problem.ticksAtState = 0;
                    }
                }
            }
        }
    }
}

DialogueSystem::ProblemState DialogueSystem::getProblemState(const NPC& npc) const
{
    auto it = npcProblems_.find(npc.getId());
    if (it != npcProblems_.end())
    {
        return it->second;
    }

    ProblemState empty;
    return empty;
}

bool DialogueSystem::shouldInitiateDialogue(const NPC& npc, int currentTick) const
{
    auto it = npcProblems_.find(npc.getId());
    if (it == npcProblems_.end())
        return false;

    const ProblemState& problem = it->second;

    // Only initiate if in PROBLEM_DETECTED state and enough ticks have passed since last dialogue
    if (problem.state == PROBLEM_DETECTED && problem.severity >= severityThreshold_)
    {
        if (problem.timeSinceLastDialogue < 0 || problem.timeSinceLastDialogue >= DIALOGUE_COOLDOWN_TICKS)
        {
            return true;
        }
    }

    return false;
}

std::string DialogueSystem::generateDialogueText(const NPC& npc, const ProblemState& problem)
{
    switch (problem.category[0])  // Use first character as simple type indicator
    {
    case 'r':  // resource
        return generateResourceShortageDialogue(npc);
    case 'f':  // faction
        return generateFactionConflictDialogue(npc);
    case 'p':  // personal
        return generatePersonalGrievanceDialogue(npc);
    case 'c':  // religious/cultural crisis
        return generateReligiousCrisisDialogue(npc);
    default:
        std::ostringstream oss;
        oss << npc.getName() << ": I have a concern I'd like to discuss with you.";
        return oss.str();
    }
}

void DialogueSystem::processPlayerResponse(NPC& npc, const std::string& playerResponse,
                                          const InterpretedDecision& decision, int currentTick)
{
    auto it = npcProblems_.find(npc.getId());
    if (it == npcProblems_.end())
        return;

    ProblemState& problem = it->second;
    problem.state = ACKNOWLEDGED;
    problem.ticksAtState = 0;
}

void DialogueSystem::resolveProblem(NPC& npc)
{
    auto it = npcProblems_.find(npc.getId());
    if (it != npcProblems_.end())
    {
        ProblemState& problem = it->second;
        problem.state = RESOLVED;
        problem.ticksAtState = 0;
        problem.severity = 0.0f;
    }
}

void DialogueSystem::acknowledgeProblem(NPC& npc)
{
    auto it = npcProblems_.find(npc.getId());
    if (it != npcProblems_.end())
    {
        ProblemState& problem = it->second;
        problem.state = ACKNOWLEDGED;
        problem.ticksAtState = 0;
    }
}

bool DialogueSystem::isProblemResolved(const NPC& npc, const WorldState& worldState) const
{
    auto it = npcProblems_.find(npc.getId());
    if (it == npcProblems_.end())
        return false;

    const ProblemState& problem = it->second;

    // Check resolution criteria based on problem category
    if (problem.category[0] == 'r')  // resource shortage
        return isResourceShortageSolved(npc, worldState);
    if (problem.category[0] == 'f')  // faction conflict
        return isFactionConflictSolved(npc, worldState);
    if (problem.category[0] == 'p')  // personal grievance
        return isPersonalGrievanceSolved(npc);
    if (problem.category[0] == 'c')  // religious crisis
        return isReligiousCrisisSolved(npc, worldState);

    return problem.state == RESOLVED;
}

float DialogueSystem::calculateProblemSeverity(const NPC& npc, const WorldState& worldState)
{
    // Simple calculation: distance from neutral mood
    return std::abs(npc.getShortTermMood() - 0.5f);
}

std::string DialogueSystem::determineProblemCategory(const NPC& npc, const WorldState& worldState)
{
    // Simplified category detection
    // In full version, would check specific conditions
    if (npc.getShortTermMood() < 0.3f)
        return "resource_shortage";  // Start with 'r'
    if (npc.getFactionId() != -1)
        return "faction_conflict";  // Start with 'f'
    return "personal_grievance";  // Start with 'p'
}

bool DialogueSystem::isResourceShortageSolved(const NPC& npc, const WorldState& worldState) const
{
    // Check if NPC's mood has improved
    return npc.getShortTermMood() > 0.5f;
}

bool DialogueSystem::isFactionConflictSolved(const NPC& npc, const WorldState& worldState) const
{
    // Check if NPC's loyalty to player has improved
    return npc.getLoyalty() > 0.6f;
}

bool DialogueSystem::isPersonalGrievanceSolved(const NPC& npc) const
{
    // Check if NPC has been in ACKNOWLEDGED state long enough
    auto it = npcProblems_.find(npc.getId());
    if (it == npcProblems_.end())
        return false;

    const ProblemState& problem = it->second;
    return problem.state == ACKNOWLEDGED || problem.state == RESOLVED;
}

bool DialogueSystem::isReligiousCrisisSolved(const NPC& npc, const WorldState& worldState) const
{
    // Check if cultural/religious values have stabilized
    return npc.getShortTermMood() > 0.4f;
}

std::string DialogueSystem::generateResourceShortageDialogue(const NPC& npc)
{
    std::ostringstream oss;
    oss << npc.getName() << ": \"We're running low on supplies. ";
    oss << "I'm worried about how long we can sustain this. What's your plan?\"";
    return oss.str();
}

std::string DialogueSystem::generateFactionConflictDialogue(const NPC& npc)
{
    std::ostringstream oss;
    oss << npc.getName() << ": \"The tension between factions is growing. ";
    oss << "We need leadership and direction or this settlement will tear itself apart.\"";
    return oss.str();
}

std::string DialogueSystem::generatePersonalGrievanceDialogue(const NPC& npc)
{
    std::ostringstream oss;
    oss << npc.getName() << ": \"I need to talk to you about something that's been bothering me. ";
    oss << "Can we discuss it?\"";
    return oss.str();
}

std::string DialogueSystem::generateReligiousCrisisDialogue(const NPC& npc)
{
    std::ostringstream oss;
    oss << npc.getName() << ": \"There's been religious discord lately. ";
    oss << "Different interpretations of our beliefs are creating conflict. What should we do?\"";
    return oss.str();
}
