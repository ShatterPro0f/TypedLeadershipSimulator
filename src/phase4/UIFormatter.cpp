#include "UIFormatter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace TLS;

UIFormatter::UIFormatter()
{
}

UIFormatter& UIFormatter::getInstance()
{
    static UIFormatter instance;
    return instance;
}

std::string UIFormatter::formatHighConfidencePrompt(const ParseResult& result,
                                                    const std::string& playerInput)
{
    std::ostringstream oss;
    oss << COLOR_SUCCESS << "[You]: " << playerInput << COLOR_RESET << "\n";
    oss << COLOR_INFO << "[SYSTEM]: Executing action: " << result.action << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatMediumConfidencePrompt(const ParseResult& result,
                                                      const std::string& playerInput)
{
    std::ostringstream oss;
    oss << COLOR_WARNING << "[You]: " << playerInput << COLOR_RESET << "\n";
    oss << COLOR_WARNING << "[SYSTEM]: Interpreting as: " << result.action 
        << ". Confirm? (Y/N)" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatAmbiguityPrompt(const std::string& playerInput,
                                              const std::vector<ParseResult>& matches,
                                              int showLimit)
{
    std::ostringstream oss;
    oss << COLOR_WARNING << "[You]: " << playerInput << COLOR_RESET << "\n";
    oss << COLOR_WARNING << "[SYSTEM]: Your input matches multiple actions. Which did you mean?\n";

    for (int i = 0; i < std::min(showLimit, static_cast<int>(matches.size())); i++)
    {
        oss << "[" << (i + 1) << "] " << matches[i].action << "\n";
    }

    oss << "[" << (showLimit + 1) << "] Try something else" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatLowConfidencePrompt(const std::string& playerInput)
{
    std::ostringstream oss;
    oss << COLOR_ERROR << "[You]: " << playerInput << COLOR_RESET << "\n";
    oss << COLOR_ERROR << "[SYSTEM]: I didn't understand that. Try rephrasing.\n";
    oss << "[Hint] Known actions: allocate, delegate, inspire, negotiate, suppress" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatUnknownCommandPrompt(const std::string& playerInput,
                                                    const std::vector<std::string>& suggestions)
{
    std::ostringstream oss;
    oss << COLOR_ERROR << "[You]: " << playerInput << COLOR_RESET << "\n";
    oss << COLOR_ERROR << "[SYSTEM]: Unknown command. Did you mean:\n";

    for (size_t i = 0; i < suggestions.size() && i < 3; i++)
    {
        oss << "  - " << suggestions[i] << "\n";
    }

    oss << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatDecisionResult(const DecisionResult& result)
{
    std::ostringstream oss;
    oss << COLOR_SUCCESS << "[RESULT] Your decision: " << result.decision.action << COLOR_RESET << "\n";

    for (const auto& npc : result.affectedNpcs)
    {
        if (npc)
        {
            oss << "[" << npc->getName() << " (" << npc->getRole() << ")] "
                << "Loyalty " << formatMeterBar(npc->getLoyalty()) << "\n";
        }
    }

    for (const auto& change : result.factionChanges)
    {
        oss << COLOR_INFO << "[FACTION] " << change << COLOR_RESET << "\n";
    }

    for (const auto& change : result.resourceChanges)
    {
        oss << COLOR_INFO << "[RESOURCE] " << change << COLOR_RESET << "\n";
    }

    oss << COLOR_INFO << "[CONTEXT] " << result.narrativeContext << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatDetailedResult(const NPC& npc, const InterpretedDecision& decision,
                                             float loyaltyBefore, float loyaltyAfter,
                                             float moodBefore, float moodAfter,
                                             const std::string& npcReaction)
{
    std::ostringstream oss;
    oss << COLOR_SUCCESS << "[RESULT] Action: " << decision.action << COLOR_RESET << "\n";
    oss << "[" << npc.getName() << " (" << npc.getRole() << ")]\n";
    oss << "  \"" << npcReaction << "\"\n";
    oss << "[IMPACT]\n";
    oss << "  Loyalty: " << formatDelta(loyaltyBefore, loyaltyAfter) << "\n";
    oss << "  Mood: " << formatDelta(moodBefore, moodAfter) << "\n";
    return oss.str();
}

std::string UIFormatter::formatDialogueWindow(const NPC& npc, const std::string& dialogue,
                                             const std::vector<std::string>& responseOptions)
{
    std::ostringstream oss;
    oss << COLOR_INFO << "┌─ DIALOGUE ─────────────────────────────────┐" << COLOR_RESET << "\n";
    oss << "│ " << npc.getName() << " (" << npc.getRole() << ")" << "\n";
    oss << "│ Loyalty: " << formatMeterBar(npc.getLoyalty()) << "\n";
    oss << "│\n";
    oss << "│ \"" << dialogue << "\"\n";
    oss << COLOR_INFO << "├─ YOUR RESPONSES ──────────────────────────┤" << COLOR_RESET << "\n";

    for (size_t i = 0; i < responseOptions.size(); i++)
    {
        oss << "[" << (i + 1) << "] " << responseOptions[i] << "\n";
    }

    oss << COLOR_INFO << "└──────────────────────────────────────────┘" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatNPCStatus(const NPC& npc)
{
    std::ostringstream oss;
    oss << npc.getName() << " (" << npc.getRole() << ")\n";
    oss << "  Loyalty: " << formatMeterBar(npc.getLoyalty()) << "\n";
    oss << "  Mood: " << formatMeterBar(npc.getShortTermMood()) << "\n";
    oss << "  Attitude: " << formatMeterBar(npc.getLongTermAttitude()) << "\n";
    return oss.str();
}

std::string UIFormatter::formatScarcityWarning(const std::string& resourceName, int currentLevel, int threshold)
{
    std::ostringstream oss;
    oss << COLOR_WARNING << "⚠ " << resourceName << " approaching scarcity threshold (" 
        << currentLevel << "/" << threshold << "). Consider trade or rationing." << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatFactionWarning(const std::string& factionName, float loyaltyLevel,
                                             const std::string& concern)
{
    std::ostringstream oss;
    oss << COLOR_WARNING << "⚠ " << factionName << " faction morale declining (" 
        << formatMeterBar(loyaltyLevel) << "). Risk of rebellion if ignored.\n";
    oss << "  Concern: " << concern << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatOpportunityHint(const std::string& opportunity)
{
    std::ostringstream oss;
    oss << COLOR_SUCCESS << "✓ " << opportunity << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatCascadeRisk(const std::vector<std::string>& potentialEvents)
{
    std::ostringstream oss;
    oss << COLOR_WARNING << "⚠ Multiple events could cascade:\n";
    for (const auto& event : potentialEvents)
    {
        oss << "  - " << event << "\n";
    }
    oss << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatTimeWarning(int ticksUntilEvent, const std::string& eventName)
{
    std::ostringstream oss;
    int daysUntil = ticksUntilEvent / 14400;
    oss << COLOR_WARNING << "⏱ " << daysUntil << " days until " << eventName 
        << ". Plan now." << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatGeneralHelp()
{
    std::ostringstream oss;
    oss << COLOR_INFO << "=== LEADERSHIP SIMULATOR HELP ===\n";
    oss << "Commands: allocate, delegate, inspire, negotiate, suppress\n";
    oss << "Type 'help [command]' for details\n";
    oss << "Type 'status' for settlement overview\n";
    oss << "Type 'factions' for faction info\n" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatActionHelp(const std::string& actionName)
{
    std::ostringstream oss;
    oss << COLOR_INFO << "=== " << actionName << " ===\n";
    oss << "Description: Allocate resources to NPCs or factions\n";
    oss << "Syntax: allocate [resource] to [target] [amount]\n";
    oss << "Example: allocate food to farmers 50\n" << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatFactionsList(const std::vector<Faction*>& factions)
{
    std::ostringstream oss;
    oss << COLOR_INFO << "=== FACTIONS ===\n";
    for (const auto& faction : factions)
    {
        if (faction)
        {
            oss << faction->getName() << " (Members: " << faction->getMemberCount() << ")\n";
        }
    }
    oss << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatResourcesList(const std::vector<Resource*>& resources)
{
    std::ostringstream oss;
    oss << COLOR_INFO << "=== RESOURCES ===\n";
    for (const auto& resource : resources)
    {
        if (resource)
        {
            oss << resource->getName() << ": " << resource->getQuantity() << " available\n";
        }
    }
    oss << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::formatCurrentStatus(const WorldState& worldState, int tick)
{
    std::ostringstream oss;
    oss << COLOR_INFO << "=== SETTLEMENT STATUS ===\n";
    oss << "Tick: " << tick << "\n";
    oss << COLOR_RESET;
    return oss.str();
}

std::string UIFormatter::stripFormatting(const std::string& formatted)
{
    std::string result = formatted;
    // Remove ANSI codes
    size_t pos = 0;
    while ((pos = result.find("\033[")) != std::string::npos)
    {
        size_t end = result.find("m", pos);
        if (end != std::string::npos)
        {
            result.erase(pos, end - pos + 1);
        }
    }
    return result;
}

std::string UIFormatter::colorize(const std::string& text, const std::string& colorCode)
{
    return colorCode + text + COLOR_RESET;
}

std::string UIFormatter::formatMeterBar(float value, int width)
{
    int filled = static_cast<int>(value * width);
    std::ostringstream oss;
    oss << "[";
    for (int i = 0; i < width; i++)
    {
        oss << (i < filled ? "=" : " ");
    }
    oss << "] " << std::fixed << std::setprecision(0) << (value * 100) << "%";
    return oss.str();
}

std::string UIFormatter::formatDelta(float before, float after)
{
    std::ostringstream oss;
    float delta = after - before;
    oss << std::fixed << std::setprecision(2) << before << " → " << after;
    if (delta >= 0)
        oss << " (+" << delta << ")";
    else
        oss << " (" << delta << ")";
    return oss.str();
}

std::string UIFormatter::formatActionDescription(const std::string& action)
{
    if (action == "allocate")
        return "Give resources to NPCs or factions";
    if (action == "delegate")
        return "Assign tasks to NPCs";
    if (action == "inspire")
        return "Boost morale of NPCs or factions";
    if (action == "negotiate")
        return "Resolve conflicts with factions";
    if (action == "suppress")
        return "Control dissent and factions";
    return "Unknown action";
}
