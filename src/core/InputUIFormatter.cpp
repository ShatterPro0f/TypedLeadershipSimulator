#include "InputUIFormatter.h"
#include "CommandValidator.h"
#include "ParameterExtractor.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace TLS {

// ============================================================================
// Singleton Instance
// ============================================================================

InputUIFormatter& InputUIFormatter::getInstance() {
    static InputUIFormatter instance;
    return instance;
}

InputUIFormatter::InputUIFormatter() {}

// ============================================================================
// Validation Result Display
// ============================================================================

std::string InputUIFormatter::formatValidationResult(const ValidationResult& result) {
    std::ostringstream oss;

    // Status line with icon
    if (result.isValid && result.warningCount == 0) {
        oss << COLOR_SUCCESS << ICON_SUCCESS << " VALIDATION PASSED" << COLOR_RESET << "\n";
    } else if (result.isValid && result.warningCount > 0) {
        oss << COLOR_WARNING << ICON_WARNING << " VALIDATION PASSED WITH WARNINGS" << COLOR_RESET << "\n";
    } else {
        oss << COLOR_ERROR << ICON_ERROR << " VALIDATION FAILED" << COLOR_RESET << "\n";
    }

    // Summary
    oss << "  " << result.summary << "\n\n";

    // Errors (if any)
    if (!result.errors.empty()) {
        oss << COLOR_ERROR << "ERRORS (" << result.errorCount << "):" << COLOR_RESET << "\n";
        for (const auto& error : result.errors) {
            oss << formatValidationError(error) << "\n";
        }
        oss << "\n";
    }

    // Warnings (if any)
    if (!result.warnings.empty()) {
        oss << COLOR_WARNING << "WARNINGS (" << result.warningCount << "):" << COLOR_RESET << "\n";
        for (const auto& warning : result.warnings) {
            oss << "  " << ICON_WARNING << " " << warning.message;
            if (!warning.field.empty()) {
                oss << " [" << warning.field << "]";
            }
            oss << "\n";
        }
        oss << "\n";
    }

    // Confidence score
    oss << COLOR_INFO << "Confidence: " << std::fixed << std::setprecision(1) 
        << (result.confidence * 100.0f) << "% " << formatConfidenceBar(result.confidence) 
        << COLOR_RESET << "\n";

    return oss.str();
}

std::string InputUIFormatter::formatValidationError(const ValidationError& error) {
    std::ostringstream oss;
    oss << "  " << COLOR_ERROR << ICON_ERROR << COLOR_RESET << " " << error.message;
    
    if (!error.field.empty()) {
        oss << " [" << COLOR_DIM << error.field << COLOR_RESET << "]";
    }
    
    if (!error.suggestions.empty()) {
        oss << "\n    " << COLOR_INFO << "Suggestions:" << COLOR_RESET;
        for (size_t i = 0; i < error.suggestions.size(); ++i) {
            if (i > 0) oss << ";";
            oss << " " << error.suggestions[i];
        }
    }
    
    return oss.str();
}

std::string InputUIFormatter::formatValidationSummary(const ValidationResult& result) {
    if (result.errorCount == 0 && result.warningCount == 0) {
        return std::string(COLOR_SUCCESS) + "✓ Ready to execute" + COLOR_RESET;
    } else if (result.errorCount > 0) {
        return std::string(COLOR_ERROR) + "✗ " + std::to_string(result.errorCount) + " error(s)" + COLOR_RESET;
    } else {
        return std::string(COLOR_WARNING) + "⚠ " + std::to_string(result.warningCount) + " warning(s)" + COLOR_RESET;
    }
}

// ============================================================================
// Confidence-based Prompts
// ============================================================================

std::string InputUIFormatter::formatConfirmationPrompt(
    float confidence,
    const std::string& action,
    const std::vector<std::string>& parameters,
    const ParseResult& parseResult
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_INFO << "CONFIRM COMMAND?" << COLOR_RESET << "\n";
    oss << "Action: " << COLOR_BOLD << action << COLOR_RESET << "\n";
    
    if (!parameters.empty()) {
        oss << "Parameters: ";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << parameters[i];
        }
        oss << "\n";
    }

    oss << "Confidence: " << std::fixed << std::setprecision(0) 
        << (confidence * 100.0f) << "% ";
    
    if (confidence >= 0.8f) {
        oss << COLOR_SUCCESS << "HIGH" << COLOR_RESET;
    } else if (confidence >= 0.7f) {
        oss << COLOR_WARNING << "MEDIUM" << COLOR_RESET;
    }
    oss << "\n\n";

    oss << "Proceed? [Y]es / [N]o / [C]ancel\n";
    oss << "> ";

    return oss.str();
}

std::string InputUIFormatter::formatDisambiguationMenu(
    const std::string& playerInput,
    const std::vector<ParseResult>& candidates,
    int maxOptions
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_WARNING << "AMBIGUOUS INPUT" << COLOR_RESET << "\n";
    oss << "Your input: \"" << COLOR_DIM << playerInput << COLOR_RESET << "\"\n";
    oss << "Matches multiple actions. Which did you mean?\n\n";

    int displayCount = std::min(static_cast<int>(candidates.size()), maxOptions);
    for (int i = 0; i < displayCount; ++i) {
        oss << "  [" << (i + 1) << "] " << candidates[i].action 
            << " (confidence: " << std::fixed << std::setprecision(0) 
            << (candidates[i].confidence * 100.0f) << "%)\n";
    }

    if (candidates.size() > maxOptions) {
        oss << "  [" << (maxOptions + 1) << "] Show more options\n";
    }
    oss << "  [0] Cancel\n\n";
    oss << "Enter number: ";

    return oss.str();
}

std::string InputUIFormatter::formatExtractedParameters(
    const ExtractedParameters& params,
    int maxParams
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_INFO << "EXTRACTED PARAMETERS:" << COLOR_RESET << "\n";
    oss << "Tone: " << COLOR_BOLD << params.tone << COLOR_RESET << "\n";
    oss << "Parameters found: " << params.validCount << " / " 
        << params.parameters.size() << "\n";

    int displayCount = std::min(static_cast<int>(params.parameters.size()), maxParams);
    for (int i = 0; i < displayCount; ++i) {
        const auto& param = params.parameters[i];
        oss << "  • " << param.rawValue 
            << " [" << COLOR_DIM << std::to_string(static_cast<int>(param.type)) << COLOR_RESET << "]"
            << " confidence: " << std::fixed << std::setprecision(0) 
            << (param.confidence * 100.0f) << "%\n";
    }

    if (params.parameters.size() > maxParams) {
        oss << "  ... and " << (params.parameters.size() - maxParams) << " more\n";
    }

    return oss.str();
}

// ============================================================================
// Error Recovery
// ============================================================================

std::string InputUIFormatter::formatErrorRecoveryMenu(
    const ValidationResult& result,
    const std::vector<std::string>& suggestions
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_ERROR << "COMMAND VALIDATION FAILED" << COLOR_RESET << "\n";
    oss << result.summary << "\n\n";

    oss << "Possible solutions:\n";
    for (size_t i = 0; i < suggestions.size() && i < 3; ++i) {
        oss << "  [" << (i + 1) << "] " << suggestions[i] << "\n";
    }

    oss << "  [0] Cancel\n\n";
    oss << "Choose option: ";

    return oss.str();
}

std::string InputUIFormatter::formatEntityNotFoundWithAlternatives(
    const std::string& entityType,
    const std::string& searchTerm,
    const std::vector<std::string>& alternatives
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_ERROR << entityType << " not found: \"" << searchTerm << "\"" << COLOR_RESET << "\n";

    if (!alternatives.empty()) {
        oss << "Did you mean one of these?\n";
        for (size_t i = 0; i < alternatives.size() && i < 3; ++i) {
            oss << "  • " << alternatives[i] << "\n";
        }
    }

    return oss.str();
}

std::string InputUIFormatter::formatQuantityErrorWithHints(
    int requested,
    int minBound,
    int maxBound,
    const std::string& resource
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_ERROR << "Invalid quantity: " << requested << COLOR_RESET << "\n";
    oss << "Must be between " << COLOR_BOLD << minBound << COLOR_RESET 
        << " and " << COLOR_BOLD << maxBound << COLOR_RESET << "\n";
    
    if (!resource.empty()) {
        oss << "Available " << resource << ": " << COLOR_WARNING << "?" << COLOR_RESET << "\n";
        oss << "Tip: Check resource levels with 'resources' command\n";
    }

    return oss.str();
}

std::string InputUIFormatter::formatPermissionDeniedWithContext(
    const std::string& action,
    const std::string& reason,
    const std::vector<std::string>& recommendations
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_ERROR << "Permission denied for action: " << action << COLOR_RESET << "\n";
    oss << "Reason: " << reason << "\n\n";

    if (!recommendations.empty()) {
        oss << "You could:\n";
        for (const auto& rec : recommendations) {
            oss << "  • " << rec << "\n";
        }
    }

    return oss.str();
}

// ============================================================================
// Parsing State Display
// ============================================================================

std::string InputUIFormatter::formatParsingState(
    const std::string& stage,
    float progress,
    const std::vector<std::string>& details
) {
    std::ostringstream oss;

    oss << "  " << COLOR_INFO << "[" << stage << "]" << COLOR_RESET << " ";
    oss << formatConfidenceBar(progress) << " ";
    oss << std::fixed << std::setprecision(0) << (progress * 100.0f) << "%\n";

    for (const auto& detail : details) {
        oss << "    " << detail << "\n";
    }

    return oss.str();
}

std::string InputUIFormatter::formatParsingStats(
    int inputLength,
    int tokensExtracted,
    float averageConfidence,
    int matchesFound
) {
    std::ostringstream oss;

    oss << "\n" << COLOR_INFO << "PARSING STATISTICS:" << COLOR_RESET << "\n";
    oss << "  Input length: " << inputLength << " chars\n";
    oss << "  Tokens extracted: " << tokensExtracted << "\n";
    oss << "  Average confidence: " << std::fixed << std::setprecision(1) 
        << (averageConfidence * 100.0f) << "%\n";
    oss << "  Matches found: " << matchesFound << "\n";

    return oss.str();
}

// ============================================================================
// Structured Response Builders
// ============================================================================

InputUIFormatter::InputPrompt InputUIFormatter::buildInputPrompt(
    const std::string& promptType,
    const ValidationResult& validationResult,
    const ParseResult& parseResult
) {
    InputPrompt prompt;

    if (promptType == "confirm") {
        prompt.title = "CONFIRM COMMAND";
        prompt.message = "Do you want to execute this command?";
        prompt.options = {"Yes", "No", "Cancel"};
        prompt.hint = "Type Y, N, or C";
        prompt.requiresNumericInput = false;
    } else if (promptType == "choose") {
        prompt.title = "CHOOSE ACTION";
        prompt.message = "Multiple matches found. Which action?";
        prompt.requiresNumericInput = true;
        prompt.minValue = 1;
        prompt.maxValue = static_cast<int>(prompt.options.size());
    } else if (promptType == "rephrase") {
        prompt.title = "REPHRASE INPUT";
        prompt.message = "Command not understood. Please try again:";
        prompt.hint = "Example: 'allocate 50 food to alice'";
        prompt.requiresNumericInput = false;
    } else if (promptType == "fix_error") {
        prompt.title = "FIX ERROR";
        prompt.message = "Validation failed. Choose an option:";
        prompt.hint = "Type the number of your choice";
        prompt.requiresNumericInput = true;
        prompt.minValue = 0;
        prompt.maxValue = 3;
    }

    return prompt;
}

std::string InputUIFormatter::formatInputPrompt(const InputPrompt& prompt) {
    std::ostringstream oss;

    oss << "\n" << COLOR_BOLD << prompt.title << COLOR_RESET << "\n";
    oss << prompt.message << "\n\n";

    if (!prompt.options.empty()) {
        for (size_t i = 0; i < prompt.options.size(); ++i) {
            oss << "  [" << (i + 1) << "] " << prompt.options[i] << "\n";
        }
        oss << "\n";
    }

    if (!prompt.hint.empty()) {
        oss << COLOR_DIM << prompt.hint << COLOR_RESET << "\n";
    }

    oss << prompt.userInputPrefix;

    return oss.str();
}

// ============================================================================
// Utility Methods
// ============================================================================

std::string InputUIFormatter::getConfidenceDescription(float confidence) {
    if (confidence >= 0.95f) return "Very High";
    if (confidence >= 0.80f) return "High";
    if (confidence >= 0.70f) return "Medium";
    if (confidence >= 0.60f) return "Fair";
    if (confidence >= 0.40f) return "Low";
    return "Very Low";
}

std::string InputUIFormatter::formatConfidenceBar(float confidence, int width) {
    int filled = static_cast<int>(confidence * width);
    std::ostringstream oss;
    oss << "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            oss << COLOR_SUCCESS << "=" << COLOR_RESET;
        } else {
            oss << " ";
        }
    }
    oss << "]";
    return oss.str();
}

std::string InputUIFormatter::formatSeverityIndicator(int severity) {
    switch (severity) {
        case 0: return COLOR_INFO + std::string(ICON_INFO) + COLOR_RESET + " INFO";
        case 1: return COLOR_WARNING + std::string(ICON_WARNING) + COLOR_RESET + " WARNING";
        case 2: return COLOR_ERROR + std::string(ICON_ERROR) + COLOR_RESET + " ERROR";
        case 3: return COLOR_ERROR + std::string(ICON_ERROR) + COLOR_RESET + " CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string InputUIFormatter::formatInputFlowBreadcrumb(
    const std::vector<std::string>& stages
) {
    std::ostringstream oss;
    for (size_t i = 0; i < stages.size(); ++i) {
        if (i > 0) oss << " " << ICON_ARROW << " ";
        oss << stages[i];
    }
    return oss.str();
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string InputUIFormatter::formatErrorList(const std::vector<ValidationError>& errors) {
    std::ostringstream oss;
    for (const auto& error : errors) {
        oss << formatValidationError(error) << "\n";
    }
    return oss.str();
}

std::string InputUIFormatter::formatWarningList(const std::vector<ValidationError>& warnings) {
    std::ostringstream oss;
    for (const auto& warning : warnings) {
        oss << "  " << ICON_WARNING << " " << warning.message << "\n";
    }
    return oss.str();
}

std::string InputUIFormatter::formatSuggestionsList(const std::vector<std::string>& suggestions) {
    std::ostringstream oss;
    oss << "Suggestions:\n";
    for (size_t i = 0; i < suggestions.size(); ++i) {
        oss << "  [" << (i + 1) << "] " << suggestions[i] << "\n";
    }
    return oss.str();
}

}  // namespace TLS
