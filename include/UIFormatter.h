#pragma once

#include "Core.h"
#include "InputParser.h"
#include <string>
#include <vector>

namespace TLS {

/**
 * @class UIFormatter
 * @brief Formats simulation state and feedback for player display
 *
 * Provides consistent, readable feedback to player including:
 * - Input parsing prompts (high confidence, medium confidence, ambiguous, low confidence)
 * - Decision result feedback with mechanical and narrative components
 * - Context hints and warnings
 * - NPC dialogue display
 * - Help system messages
 */
class UIFormatter {
public:
    static UIFormatter& getInstance();

    // ====== INPUT PARSING PROMPTS ======

    // High confidence (>= 0.9): Execute immediately
    std::string formatHighConfidencePrompt(const ParseResult& result,
                                          const std::string& playerInput);

    // Medium confidence (0.7-0.89): Confirm before executing
    std::string formatMediumConfidencePrompt(const ParseResult& result,
                                            const std::string& playerInput);

    // Ambiguous (0.6-0.8, multiple matches): Ask which one?
    std::string formatAmbiguityPrompt(const std::string& playerInput,
                                     const std::vector<ParseResult>& matches,
                                     int showLimit = 3);

    // Low confidence (< 0.6): Ask to rephrase
    std::string formatLowConfidencePrompt(const std::string& playerInput);

    // Unknown command: Suggest closest matches
    std::string formatUnknownCommandPrompt(const std::string& playerInput,
                                          const std::vector<std::string>& suggestions);

    // ====== DECISION RESULT FEEDBACK ======

    struct DecisionResult {
        const InterpretedDecision& decision;
        const std::vector<NPC*>& affectedNpcs;
        const std::vector<std::string>& factionChanges;
        const std::vector<std::string>& resourceChanges;
        const std::string& narrativeContext;
    };

    std::string formatDecisionResult(const DecisionResult& result);

    // Standard result format with all components
    std::string formatDetailedResult(const NPC& npc, const InterpretedDecision& decision,
                                    float loyaltyBefore, float loyaltyAfter,
                                    float moodBefore, float moodAfter,
                                    const std::string& npcReaction);

    // ====== NPC DIALOGUE DISPLAY ======

    std::string formatDialogueWindow(const NPC& npc, const std::string& dialogue,
                                    const std::vector<std::string>& responseOptions);

    std::string formatNPCStatus(const NPC& npc);  // Loyalty, mood, concern

    // ====== CONTEXT HINTS & WARNINGS ======

    std::string formatScarcityWarning(const std::string& resourceName, int currentLevel, int threshold);

    std::string formatFactionWarning(const std::string& factionName, float loyaltyLevel,
                                    const std::string& concern);

    std::string formatOpportunityHint(const std::string& opportunity);

    std::string formatCascadeRisk(const std::vector<std::string>& potentialEvents);

    std::string formatTimeWarning(int ticksUntilEvent, const std::string& eventName);

    // ====== HELP SYSTEM ======

    std::string formatGeneralHelp();
    std::string formatActionHelp(const std::string& actionName);
    std::string formatFactionsList(const std::vector<Faction*>& factions);
    std::string formatResourcesList(const std::vector<Resource*>& resources);
    std::string formatCurrentStatus(const WorldState& worldState, int tick);

    // ====== UTILITY ======

    // Strip ANSI color codes if rendering to plain text
    std::string stripFormatting(const std::string& formatted);

    // Add color codes for terminal display (ANSI)
    std::string colorize(const std::string& text, const std::string& colorCode);

private:
    UIFormatter();
    UIFormatter(const UIFormatter&) = delete;
    UIFormatter& operator=(const UIFormatter&) = delete;

    // Helper formatting
    std::string formatMeterBar(float value, int width = 20);  // [=====>    ] 42%
    std::string formatDelta(float before, float after);  // "0.5 → 0.7 (+0.2)"
    std::string formatActionDescription(const std::string& action);

    // Color codes (ANSI)
    static constexpr const char* COLOR_SUCCESS = "\033[32m";   // Green
    static constexpr const char* COLOR_WARNING = "\033[33m";   // Yellow
    static constexpr const char* COLOR_ERROR = "\033[31m";     // Red
    static constexpr const char* COLOR_INFO = "\033[36m";      // Cyan
    static constexpr const char* COLOR_RESET = "\033[0m";      // Reset
};

}  // namespace TLS
