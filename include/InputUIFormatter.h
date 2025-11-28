#pragma once

#include <string>
#include <vector>
#include <memory>
#include "InputParser.h"        // Includes ParseResult
#include "CommandValidator.h"   // Includes ValidationResult, ValidationError
#include "ParameterExtractor.h" // Includes ExtractedParameters

namespace TLS {

// Forward declarations (if needed for other uses)
// struct ParseResult;         // Now included from InputParser.h
// struct ValidationResult;    // Now included from CommandValidator.h
// struct ValidationError;     // Now included from CommandValidator.h
// struct ExtractedParameters; // Now included from ParameterExtractor.h
// class CommandValidator;     // Now included from CommandValidator.h

/**
 * @class InputUIFormatter
 * @brief Phase 5 specific UI formatting for input parsing and validation
 * 
 * Extends UIFormatter with Phase 5.5 specific features:
 *  - Validation result display (errors, warnings, suggestions)
 *  - Confidence-based user prompts
 *  - Parameter extraction feedback
 *  - Disambiguation menus for ambiguous input
 *  - Error recovery suggestions
 */
class InputUIFormatter {
public:
    static InputUIFormatter& getInstance();

    // ====== VALIDATION RESULT DISPLAY ======

    /**
     * Format validation result for player display
     * Shows errors, warnings, and actionable suggestions
     */
    std::string formatValidationResult(const ValidationResult& result);

    /**
     * Format single validation error with context
     */
    std::string formatValidationError(const ValidationError& error);

    /**
     * Format validation summary (brief status message)
     */
    std::string formatValidationSummary(const ValidationResult& result);

    // ====== CONFIDENCE-BASED PROMPTS ======

    /**
     * Format confirmation prompt for medium-confidence commands (0.7-0.89)
     * Player must confirm "Y/N" before execution
     */
    std::string formatConfirmationPrompt(
        float confidence,
        const std::string& action,
        const std::vector<std::string>& parameters,
        const ParseResult& parseResult
    );

    /**
     * Format ambiguity resolution menu
     * Multiple actions match with similar confidence - player chooses
     */
    std::string formatDisambiguationMenu(
        const std::string& playerInput,
        const std::vector<ParseResult>& candidates,
        int maxOptions = 3
    );

    /**
     * Format parameter extraction feedback
     * Shows what parameters were extracted with confidence scores
     */
    std::string formatExtractedParameters(
        const ExtractedParameters& params,
        int maxParams = 5
    );

    // ====== ERROR RECOVERY SUGGESTIONS ======

    /**
     * Format error recovery menu
     * Suggest corrections when command fails validation
     */
    std::string formatErrorRecoveryMenu(
        const ValidationResult& result,
        const std::vector<std::string>& suggestions
    );

    /**
     * Format entity not found with alternatives
     * When NPC/Faction/Resource not found, suggest similar entities
     */
    std::string formatEntityNotFoundWithAlternatives(
        const std::string& entityType,
        const std::string& searchTerm,
        const std::vector<std::string>& alternatives
    );

    /**
     * Format quantity bounds error with help
     */
    std::string formatQuantityErrorWithHints(
        int requested,
        int minBound,
        int maxBound,
        const std::string& resource
    );

    /**
     * Format permission denied with explanation
     */
    std::string formatPermissionDeniedWithContext(
        const std::string& action,
        const std::string& reason,
        const std::vector<std::string>& recommendations
    );

    // ====== PARSING STATE DISPLAY ======

    /**
     * Format current parsing state (for progress/debugging)
     */
    std::string formatParsingState(
        const std::string& stage,  // "parsing" | "extracting" | "validating" | "complete"
        float progress,             // 0.0-1.0
        const std::vector<std::string>& details
    );

    /**
     * Format parsing statistics for debugging
     */
    std::string formatParsingStats(
        int inputLength,
        int tokensExtracted,
        float averageConfidence,
        int matchesFound
    );

    // ====== STRUCTURED RESPONSE BUILDERS ======

    struct InputPrompt {
        std::string title;
        std::string message;
        std::vector<std::string> options;  // Numbered options for user selection
        std::string hint;
        bool requiresNumericInput = false;
        int minValue = 0;
        int maxValue = 0;
        std::string userInputPrefix = "> ";
    };

    /**
     * Build structured input prompt for player response
     */
    InputPrompt buildInputPrompt(
        const std::string& promptType,  // "confirm" | "choose" | "rephrase" | "fix_error"
        const ValidationResult& validationResult,
        const ParseResult& parseResult
    );

    /**
     * Format prompt for display to player
     */
    std::string formatInputPrompt(const InputPrompt& prompt);

    // ====== UTILITY ======

    /**
     * Confidence level description
     */
    std::string getConfidenceDescription(float confidence);

    /**
     * Format confidence as visual indicator
     */
    std::string formatConfidenceBar(float confidence, int width = 15);

    /**
     * Format severity level with icon/color
     */
    std::string formatSeverityIndicator(int severity);

    /**
     * Create breadcrumb trail showing input flow
     */
    std::string formatInputFlowBreadcrumb(
        const std::vector<std::string>& stages
    );

private:
    InputUIFormatter();
    InputUIFormatter(const InputUIFormatter&) = delete;
    InputUIFormatter& operator=(const InputUIFormatter&) = delete;

    // Helper methods
    std::string formatErrorList(const std::vector<ValidationError>& errors);
    std::string formatWarningList(const std::vector<ValidationError>& warnings);
    std::string formatSuggestionsList(const std::vector<std::string>& suggestions);
    
    // Color/formatting constants
    static constexpr const char* ICON_ERROR = "✗";
    static constexpr const char* ICON_WARNING = "⚠";
    static constexpr const char* ICON_SUCCESS = "✓";
    static constexpr const char* ICON_INFO = "ℹ";
    static constexpr const char* ICON_ARROW = "→";

    // ANSI color codes
    static constexpr const char* COLOR_ERROR = "\033[31m";     // Red
    static constexpr const char* COLOR_WARNING = "\033[33m";   // Yellow
    static constexpr const char* COLOR_SUCCESS = "\033[32m";   // Green
    static constexpr const char* COLOR_INFO = "\033[36m";      // Cyan
    static constexpr const char* COLOR_RESET = "\033[0m";      // Reset
    static constexpr const char* COLOR_DIM = "\033[2m";        // Dim
    static constexpr const char* COLOR_BOLD = "\033[1m";       // Bold
};

}  // namespace TLS
