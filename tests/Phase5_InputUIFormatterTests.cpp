#include <gtest/gtest.h>
#include "InputUIFormatter.h"
#include "CommandValidator.h"
#include "ParameterExtractor.h"
#include <iostream>

using namespace TLS;

/**
 * Phase 5.5 Input UI Formatter Tests
 * 
 * Tests for:
 *  - Validation result formatting
 *  - Confidence-based prompts
 *  - Error recovery menus
 *  - Parameter extraction feedback
 *  - UI utility functions (confidence bars, severity indicators, etc.)
 */

class InputUIFormatterTest : public ::testing::Test {
protected:
    InputUIFormatter& formatter;

    InputUIFormatterTest() : formatter(InputUIFormatter::getInstance()) {}

    void SetUp() override {
        // Tests are mostly string formatting, no setup needed
    }

    void TearDown() override {
    }

    // Helper to create mock validation results
    ValidationResult createPassedValidation() {
        ValidationResult result;
        result.isValid = true;
        result.hasWarnings = false;
        result.errorCount = 0;
        result.warningCount = 0;
        result.confidence = 0.95f;
        result.summary = "✓ Validation passed: Command is ready to execute";
        return result;
    }

    ValidationResult createFailedValidation() {
        ValidationResult result;
        result.isValid = false;
        result.hasWarnings = false;
        result.errorCount = 1;
        result.warningCount = 0;
        result.confidence = 0.5f;
        result.summary = "✗ Validation failed: 1 error(s) found";

        ValidationError error;
        error.severity = ValidationError::ERROR;
        error.code = "NPC_NOT_FOUND";
        error.message = "NPC 'unknown' not found in settlement";
        error.field = "target_npc";
        error.invalidValue = "unknown";
        error.suggestions = {"Check spelling", "List NPCs with 'npcs' command"};
        result.errors.push_back(error);

        return result;
    }

    ValidationResult createWarningValidation() {
        ValidationResult result;
        result.isValid = true;
        result.hasWarnings = true;
        result.errorCount = 0;
        result.warningCount = 1;
        result.confidence = 0.85f;
        result.summary = "⚠ Validation passed with 1 warning(s)";

        ValidationError warning;
        warning.severity = ValidationError::WARNING;
        warning.code = "UNKNOWN_TONE";
        warning.message = "Tone not recognized, using neutral";
        warning.field = "tone";
        result.warnings.push_back(warning);

        return result;
    }
};

// ============================================================================
// Validation Result Display Tests
// ============================================================================

TEST_F(InputUIFormatterTest, FormatValidationResult_Passed) {
    auto result = createPassedValidation();
    std::string formatted = formatter.formatValidationResult(result);
    
    EXPECT_NE(formatted.find("PASSED"), std::string::npos);
    EXPECT_NE(formatted.find("95"), std::string::npos);  // 95% confidence
}

TEST_F(InputUIFormatterTest, FormatValidationResult_Failed) {
    auto result = createFailedValidation();
    std::string formatted = formatter.formatValidationResult(result);
    
    EXPECT_NE(formatted.find("FAILED"), std::string::npos);
    EXPECT_NE(formatted.find("ERRORS"), std::string::npos);
    EXPECT_NE(formatted.find("NPC"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatValidationResult_WithWarnings) {
    auto result = createWarningValidation();
    std::string formatted = formatter.formatValidationResult(result);
    
    EXPECT_NE(formatted.find("WARNINGS"), std::string::npos);
    EXPECT_NE(formatted.find("PASSED"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatValidationError_WithSuggestions) {
    ValidationError error;
    error.severity = ValidationError::ERROR;
    error.code = "NPC_NOT_FOUND";
    error.message = "NPC not found";
    error.field = "target_npc";
    error.invalidValue = "unknown";
    error.suggestions = {"Check spelling", "List NPCs"};
    
    std::string formatted = formatter.formatValidationError(error);
    
    EXPECT_NE(formatted.find("not found"), std::string::npos);
    EXPECT_NE(formatted.find("Suggestions"), std::string::npos);
    EXPECT_NE(formatted.find("Check spelling"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatValidationSummary_Success) {
    ValidationResult result;
    result.errorCount = 0;
    result.warningCount = 0;
    result.isValid = true;
    
    std::string summary = formatter.formatValidationSummary(result);
    EXPECT_NE(summary.find("Ready"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatValidationSummary_Errors) {
    ValidationResult result;
    result.errorCount = 2;
    result.warningCount = 0;
    result.isValid = false;
    
    std::string summary = formatter.formatValidationSummary(result);
    EXPECT_NE(summary.find("2"), std::string::npos);
    EXPECT_NE(summary.find("error"), std::string::npos);
}

// ============================================================================
// Confidence-based Prompt Tests
// ============================================================================

TEST_F(InputUIFormatterTest, FormatConfirmationPrompt_HighConfidence) {
    ParseResult parseResult;
    parseResult.action = "allocate";
    parseResult.confidence = 0.85f;
    
    std::string formatted = formatter.formatConfirmationPrompt(
        0.85f,
        "allocate",
        {"food", "alice", "50"},
        parseResult
    );
    
    EXPECT_NE(formatted.find("CONFIRM"), std::string::npos);
    EXPECT_NE(formatted.find("allocate"), std::string::npos);
    EXPECT_NE(formatted.find("[Y]"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatDisambiguationMenu_MultipleOptions) {
    std::vector<ParseResult> candidates;
    
    ParseResult r1;
    r1.action = "allocate";
    r1.confidence = 0.90f;
    candidates.push_back(r1);
    
    ParseResult r2;
    r2.action = "delegate";
    r2.confidence = 0.85f;
    candidates.push_back(r2);
    
    std::string formatted = formatter.formatDisambiguationMenu("aloc food", candidates);
    
    EXPECT_NE(formatted.find("AMBIGUOUS"), std::string::npos);
    EXPECT_NE(formatted.find("allocate"), std::string::npos);
    EXPECT_NE(formatted.find("delegate"), std::string::npos);
    EXPECT_NE(formatted.find("[1]"), std::string::npos);
    EXPECT_NE(formatted.find("[2]"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatExtractedParameters_MultipleParams) {
    ExtractedParameters params;
    params.tone = "positive";
    params.validCount = 2;
    params.confidenceScore = 0.85f;
    
    ExtractedParameter p1;
    p1.rawValue = "Alice";
    p1.confidence = 0.95f;
    params.parameters.push_back(p1);
    
    ExtractedParameter p2;
    p2.rawValue = "50";
    p2.confidence = 0.80f;
    params.parameters.push_back(p2);
    
    std::string formatted = formatter.formatExtractedParameters(params);
    
    EXPECT_NE(formatted.find("EXTRACTED"), std::string::npos);
    EXPECT_NE(formatted.find("Alice"), std::string::npos);
    EXPECT_NE(formatted.find("positive"), std::string::npos);
}

// ============================================================================
// Error Recovery Tests
// ============================================================================

TEST_F(InputUIFormatterTest, FormatErrorRecoveryMenu_WithSuggestions) {
    auto result = createFailedValidation();
    std::vector<std::string> suggestions = {
        "Check NPC name spelling",
        "Use 'npcs' command to list all NPCs",
        "Try 'help' for available commands"
    };
    
    std::string formatted = formatter.formatErrorRecoveryMenu(result, suggestions);
    
    EXPECT_NE(formatted.find("VALIDATION FAILED"), std::string::npos);
    EXPECT_NE(formatted.find("solutions"), std::string::npos);
    EXPECT_NE(formatted.find("NPC name"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatEntityNotFoundWithAlternatives_NPCs) {
    std::vector<std::string> alternatives = {"Alice", "Bob", "Charlie"};
    
    std::string formatted = formatter.formatEntityNotFoundWithAlternatives(
        "NPC",
        "alec",
        alternatives
    );
    
    EXPECT_NE(formatted.find("not found"), std::string::npos);
    EXPECT_NE(formatted.find("alec"), std::string::npos);
    EXPECT_NE(formatted.find("Alice"), std::string::npos);
    EXPECT_NE(formatted.find("Did you mean"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatQuantityErrorWithHints) {
    std::string formatted = formatter.formatQuantityErrorWithHints(
        2000,  // requested
        1,     // min
        1000,  // max
        "Food"
    );
    
    EXPECT_NE(formatted.find("Invalid quantity"), std::string::npos);
    EXPECT_NE(formatted.find("2000"), std::string::npos);
    EXPECT_NE(formatted.find("1000"), std::string::npos);
    EXPECT_NE(formatted.find("resources"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatPermissionDeniedWithContext) {
    std::vector<std::string> recommendations = {
        "Improve relations with the faction",
        "Try negotiating instead",
        "Wait for opportunity"
    };
    
    std::string formatted = formatter.formatPermissionDeniedWithContext(
        "suppress",
        "Warriors faction is too powerful to suppress directly",
        recommendations
    );
    
    EXPECT_NE(formatted.find("Permission denied"), std::string::npos);
    EXPECT_NE(formatted.find("suppress"), std::string::npos);
    EXPECT_NE(formatted.find("Warriors"), std::string::npos);
    EXPECT_NE(formatted.find("could"), std::string::npos);
}

// ============================================================================
// Parsing State Tests
// ============================================================================

TEST_F(InputUIFormatterTest, FormatParsingState_Progress) {
    std::vector<std::string> details = {
        "Parsed 3 tokens",
        "Matched 2 actions",
        "Building parameters..."
    };
    
    std::string formatted = formatter.formatParsingState("parsing", 0.50f, details);
    
    EXPECT_NE(formatted.find("parsing"), std::string::npos);
    EXPECT_NE(formatted.find("50%"), std::string::npos);
    EXPECT_NE(formatted.find("tokens"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatParsingStats) {
    std::string formatted = formatter.formatParsingStats(
        25,      // inputLength
        5,       // tokensExtracted
        0.87f,   // averageConfidence
        3        // matchesFound
    );
    
    EXPECT_NE(formatted.find("STATISTICS"), std::string::npos);
    EXPECT_NE(formatted.find("25"), std::string::npos);
    EXPECT_NE(formatted.find("87"), std::string::npos);
    EXPECT_NE(formatted.find("3"), std::string::npos);
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST_F(InputUIFormatterTest, GetConfidenceDescription_VeryHigh) {
    std::string desc = formatter.getConfidenceDescription(0.98f);
    EXPECT_EQ(desc, "Very High");
}

TEST_F(InputUIFormatterTest, GetConfidenceDescription_High) {
    std::string desc = formatter.getConfidenceDescription(0.85f);
    EXPECT_EQ(desc, "High");
}

TEST_F(InputUIFormatterTest, GetConfidenceDescription_Medium) {
    std::string desc = formatter.getConfidenceDescription(0.75f);
    EXPECT_EQ(desc, "Medium");
}

TEST_F(InputUIFormatterTest, GetConfidenceDescription_Low) {
    std::string desc = formatter.getConfidenceDescription(0.55f);
    EXPECT_EQ(desc, "Low");
}

TEST_F(InputUIFormatterTest, FormatConfidenceBar_Half) {
    std::string bar = formatter.formatConfidenceBar(0.50f, 10);
    
    // Should contain brackets and content
    EXPECT_NE(bar.find("["), std::string::npos);
    EXPECT_NE(bar.find("]"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatSeverityIndicator_Info) {
    std::string formatted = formatter.formatSeverityIndicator(0);
    EXPECT_NE(formatted.find("INFO"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatSeverityIndicator_Warning) {
    std::string formatted = formatter.formatSeverityIndicator(1);
    EXPECT_NE(formatted.find("WARNING"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatSeverityIndicator_Error) {
    std::string formatted = formatter.formatSeverityIndicator(2);
    EXPECT_NE(formatted.find("ERROR"), std::string::npos);
}

TEST_F(InputUIFormatterTest, FormatInputFlowBreadcrumb) {
    std::vector<std::string> stages = {"Input", "Parse", "Extract", "Validate"};
    std::string formatted = formatter.formatInputFlowBreadcrumb(stages);
    
    EXPECT_NE(formatted.find("Input"), std::string::npos);
    EXPECT_NE(formatted.find("Validate"), std::string::npos);
}

// ============================================================================
// Structured Response Builder Tests
// ============================================================================

TEST_F(InputUIFormatterTest, BuildInputPrompt_Confirm) {
    auto result = createPassedValidation();
    ParseResult parseResult;
    parseResult.action = "allocate";
    
    auto prompt = formatter.buildInputPrompt("confirm", result, parseResult);
    
    EXPECT_EQ(prompt.title, "CONFIRM COMMAND");
    EXPECT_FALSE(prompt.options.empty());
    EXPECT_NE(prompt.hint.find("Y"), std::string::npos);
}

TEST_F(InputUIFormatterTest, BuildInputPrompt_Choose) {
    auto result = createPassedValidation();
    ParseResult parseResult;
    
    auto prompt = formatter.buildInputPrompt("choose", result, parseResult);
    
    EXPECT_EQ(prompt.title, "CHOOSE ACTION");
    EXPECT_TRUE(prompt.requiresNumericInput);
    // Check that prompt was created (other properties may vary based on context)
    EXPECT_FALSE(prompt.title.empty());
}

TEST_F(InputUIFormatterTest, FormatInputPrompt_CreatesDisplayable) {
    auto prompt = formatter.buildInputPrompt("confirm", createPassedValidation(), ParseResult());
    std::string formatted = formatter.formatInputPrompt(prompt);
    
    EXPECT_NE(formatted.find("CONFIRM"), std::string::npos);
    EXPECT_NE(formatted.find(">"), std::string::npos);  // User input prefix
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(InputUIFormatterTest, IntegrationTest_FullValidationFlow) {
    auto failedResult = createFailedValidation();
    std::string validationDisplay = formatter.formatValidationResult(failedResult);
    
    EXPECT_GT(validationDisplay.length(), 50);  // Should be substantial text
    EXPECT_NE(validationDisplay.find("ERRORS"), std::string::npos);
    EXPECT_NE(validationDisplay.find("Confidence"), std::string::npos);
}

TEST_F(InputUIFormatterTest, IntegrationTest_FullErrorRecoveryFlow) {
    auto result = createFailedValidation();
    std::vector<std::string> suggestions = {
        "Fix the NPC name",
        "Use help command",
        "Cancel and try again"
    };
    
    std::string recovery = formatter.formatErrorRecoveryMenu(result, suggestions);
    
    EXPECT_GT(recovery.length(), 50);
    EXPECT_NE(recovery.find("solutions"), std::string::npos);
}
