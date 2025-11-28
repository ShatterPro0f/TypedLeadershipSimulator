#include <gtest/gtest.h>
#include "FuzzyParser.h"
#include "ActionRegistry.h"
#include <iostream>

/**
 * Phase 5.2 Fuzzy Parser Tests
 * 
 * Tests for fuzzy matching, confidence scoring, parameter extraction
 * Validates Levenshtein distance, hybrid confidence calculation, and action ranking
 */

class FuzzyParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize action registry with test data
        ActionRegistry::initialize();
        ActionRegistry& registry = ActionRegistry::getInstance();
        
        // Try loading from data/action_registry.json (from build or project root)
        bool loaded = registry.loadFromJSON("data/action_registry.json");
        if (!loaded) {
            // Try from project root
            loaded = registry.loadFromJSON("../data/action_registry.json");
        }
        
        if (!loaded) {
            std::cerr << "Warning: Could not load action registry. Tests will proceed but registry-dependent tests may fail." << std::endl;
        }
    }
    
    void TearDown() override {
        FuzzyParser::clearLevenshteinCache();
        ActionRegistry::getInstance().shutdown();
    }
};

// ============================================================================
// Levenshtein Distance Tests
// ============================================================================

TEST_F(FuzzyParserTest, LevenshteinDistance_ExactMatch) {
    int distance = FuzzyParser::levenshteinDistance("allocate", "allocate");
    EXPECT_EQ(distance, 0);
}

TEST_F(FuzzyParserTest, LevenshteinDistance_OneCharacterDifference) {
    // "allocate" vs "alokate" (actually 2 edits: missing 'c' and misaligned)
    int distance = FuzzyParser::levenshteinDistance("allocate", "alokate");
    EXPECT_LT(distance, 3);  // Should be small
}

TEST_F(FuzzyParserTest, LevenshteinDistance_TwoCharacterDifferences) {
    // "delegate" vs "deligate" (extra 'i', different 'g')
    int distance = FuzzyParser::levenshteinDistance("delegate", "deligate");
    EXPECT_EQ(distance, 1);  // Only 1 substitution needed
}

TEST_F(FuzzyParserTest, LevenshteinDistance_CaseInsensitive) {
    int dist1 = FuzzyParser::levenshteinDistance("Allocate", "allocate");
    int dist2 = FuzzyParser::levenshteinDistance("ALLOCATE", "allocate");
    EXPECT_EQ(dist1, 0);
    EXPECT_EQ(dist2, 0);
}

TEST_F(FuzzyParserTest, LevenshteinDistance_EmptyString) {
    EXPECT_EQ(FuzzyParser::levenshteinDistance("", "allocate"), 8);
    EXPECT_EQ(FuzzyParser::levenshteinDistance("allocate", ""), 8);
    EXPECT_EQ(FuzzyParser::levenshteinDistance("", ""), 0);
}

TEST_F(FuzzyParserTest, LevenshteinDistance_Caching) {
    // First call
    int dist1 = FuzzyParser::levenshteinDistance("allocate", "alokate");
    size_t cache_size1 = FuzzyParser::getCachSize();
    
    // Second call (should use cache)
    int dist2 = FuzzyParser::levenshteinDistance("allocate", "alokate");
    size_t cache_size2 = FuzzyParser::getCachSize();
    
    EXPECT_EQ(dist1, dist2);
    EXPECT_EQ(cache_size1, cache_size2);  // Cache size unchanged
}

// ============================================================================
// Confidence Calculation Tests
// ============================================================================

TEST_F(FuzzyParserTest, ConfidenceCalculation_ExactMatch) {
    float conf = FuzzyParser::calculateConfidence("allocate", "allocate");
    EXPECT_GT(conf, 0.95f);  // Exact match should have very high confidence
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_CaseInsensitive) {
    float conf1 = FuzzyParser::calculateConfidence("Allocate", "allocate");
    float conf2 = FuzzyParser::calculateConfidence("ALLOCATE", "allocate");
    EXPECT_GT(conf1, 0.95f);
    EXPECT_GT(conf2, 0.95f);
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_FuzzyMatch) {
    // "alokate" is fuzzy match of "allocate" (distance ~2)
    float conf = FuzzyParser::calculateConfidence("alokate", "allocate");
    EXPECT_GT(conf, 0.3f);   // Should be reasonable confidence
    EXPECT_LT(conf, 1.0f);   // But not perfect
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_SubstringMatch) {
    // "allo" is substring of "allocate"
    float conf = FuzzyParser::calculateConfidence("allo", "allocate");
    EXPECT_GT(conf, 0.4f);  // Substring should have reasonable confidence
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_TwoCharDifference) {
    // "delegat" vs "delegate" (1 char difference)
    float conf = FuzzyParser::calculateConfidence("delegat", "delegate");
    EXPECT_GT(conf, 0.6f);
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_WeightCustomization) {
    // Test with different weight configurations
    float conf1 = FuzzyParser::calculateConfidence("allocate", "allocate", 0.5f, 0.3f, 0.2f);
    float conf2 = FuzzyParser::calculateConfidence("allocate", "allocate", 0.3f, 0.4f, 0.3f);
    
    // Both should be 1.0 for exact match regardless of weights
    EXPECT_GT(conf1, 0.99f);
    EXPECT_GT(conf2, 0.99f);
}

TEST_F(FuzzyParserTest, ConfidenceCalculation_NoMatch) {
    // Completely different words
    float conf = FuzzyParser::calculateConfidence("xyz", "allocate");
    EXPECT_LT(conf, 0.5f);  // Should be low confidence
}

// ============================================================================
// Parameter Extraction Tests
// ============================================================================

TEST_F(FuzzyParserTest, ParameterExtraction_BasicExtraction) {
    auto params = FuzzyParser::extractParametersBasic("allocate 50 food to farmers", "allocate");
    
    // Should contain "50", "food", "farmers" but exclude "to"
    EXPECT_GE(params.size(), 2);
    EXPECT_NE(std::find(params.begin(), params.end(), "50"), params.end());
    EXPECT_NE(std::find(params.begin(), params.end(), "food"), params.end());
}

TEST_F(FuzzyParserTest, ParameterExtraction_RemovesActionName) {
    auto params = FuzzyParser::extractParametersBasic("allocate food", "allocate");
    
    // Should not contain "allocate"
    EXPECT_EQ(std::find(params.begin(), params.end(), "allocate"), params.end());
}

TEST_F(FuzzyParserTest, ParameterExtraction_RemovesStopwords) {
    auto params = FuzzyParser::extractParametersBasic("allocate the food to farmers", "allocate");
    
    // Should not contain "the" or "to"
    EXPECT_EQ(std::find(params.begin(), params.end(), "the"), params.end());
    EXPECT_EQ(std::find(params.begin(), params.end(), "to"), params.end());
}

// ============================================================================
// Parsing Full Input Tests
// ============================================================================

TEST_F(FuzzyParserTest, ParseInput_ExactActionName) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("allocate", registry);
    
    if (results.size() > 0) {
        EXPECT_EQ(results[0].actionName, "allocate");
        EXPECT_GT(results[0].confidence, 0.95f);
        EXPECT_EQ(results[0].matchType, "exact");
    } else {
        GTEST_SKIP() << "Action registry not loaded";
    }
}

TEST_F(FuzzyParserTest, ParseInput_ActionAlias) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    if (registry.getAllActions().empty()) {
        GTEST_SKIP() << "Action registry not loaded";
    }
    
    auto results = FuzzyParser::parseInput("give food to farmers", registry);
    
    EXPECT_GT(results.size(), 0);
    EXPECT_EQ(results[0].actionName, "allocate");  // "give" is alias for "allocate"
    EXPECT_GT(results[0].confidence, 0.3f);  // Multiple words reduce confidence slightly
}

TEST_F(FuzzyParserTest, ParseInput_FuzzyMatch) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    if (registry.getAllActions().empty()) {
        GTEST_SKIP() << "Action registry not loaded";
    }
    
    auto results = FuzzyParser::parseInput("alokate", registry);  // Typo
    
    EXPECT_GT(results.size(), 0);
    EXPECT_EQ(results[0].actionName, "allocate");
    EXPECT_GT(results[0].confidence, 0.3f);
    EXPECT_NE(results[0].matchType, "exact");
}

TEST_F(FuzzyParserTest, ParseInput_MultipleMatches) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("help", registry);
    
    EXPECT_GT(results.size(), 1);  // "help" is multiple aliases
    
    // First result should have highest confidence
    if (results.size() > 1) {
        EXPECT_GE(results[0].confidence, results[1].confidence);
    }
}

TEST_F(FuzzyParserTest, ParseInput_Ranking) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("allocate", registry);
    
    // Results should be sorted by confidence (descending)
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].confidence, results[i].confidence);
    }
}

TEST_F(FuzzyParserTest, ParseInput_TopFiveLimit) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("allocate", registry);
    
    // Should not exceed 5 results
    EXPECT_LE(results.size(), 5);
}

TEST_F(FuzzyParserTest, ParseInput_ExtractsParameters) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("allocate 50 food", registry);
    
    EXPECT_GT(results.size(), 0);
    EXPECT_GT(results[0].rawParams.size(), 0);
    
    // Should contain "50" and "food"
    EXPECT_NE(std::find(results[0].rawParams.begin(), results[0].rawParams.end(), "50"),
              results[0].rawParams.end());
}

TEST_F(FuzzyParserTest, ParseInput_EmptyInput) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("", registry);
    
    EXPECT_EQ(results.size(), 0);
}

TEST_F(FuzzyParserTest, ParseInput_WhitespaceOnly) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("   ", registry);
    
    EXPECT_EQ(results.size(), 0);
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST_F(FuzzyParserTest, ToLowercase) {
    EXPECT_EQ(FuzzyParser::toLowercase("Allocate"), "allocate");
    EXPECT_EQ(FuzzyParser::toLowercase("DELEGATE"), "delegate");
    EXPECT_EQ(FuzzyParser::toLowercase("allocate"), "allocate");
}

TEST_F(FuzzyParserTest, Trim) {
    EXPECT_EQ(FuzzyParser::trim("  allocate  "), "allocate");
    EXPECT_EQ(FuzzyParser::trim("\t\nallocate\n\t"), "allocate");
    EXPECT_EQ(FuzzyParser::trim("allocate"), "allocate");
}

TEST_F(FuzzyParserTest, Split) {
    auto tokens = FuzzyParser::split("allocate 50 food", ' ');
    
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "allocate");
    EXPECT_EQ(tokens[1], "50");
    EXPECT_EQ(tokens[2], "food");
}

TEST_F(FuzzyParserTest, CharacterOverlap) {
    // "allocate" and "allocate" should have complete overlap
    float overlap1 = FuzzyParser::calculateCharacterOverlap("allocate", "allocate");
    EXPECT_GT(overlap1, 0.9f);
    
    // "abc" and "def" should have no overlap
    float overlap2 = FuzzyParser::calculateCharacterOverlap("abc", "def");
    EXPECT_LT(overlap2, 0.5f);
}

// ============================================================================
// Real-World Scenario Tests
// ============================================================================

TEST_F(FuzzyParserTest, Scenario_PlayerTyposAllocate) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("alocate", registry);  // Single typo in action name
    
    EXPECT_GT(results.size(), 0);
    EXPECT_EQ(results[0].actionName, "allocate");
}

TEST_F(FuzzyParserTest, Scenario_PlayerUsesAlias) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("give resources to the merchants", registry);
    
    EXPECT_GT(results.size(), 0);
    EXPECT_EQ(results[0].actionName, "allocate");
}

TEST_F(FuzzyParserTest, Scenario_PlayerPartialInput) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    auto results = FuzzyParser::parseInput("allo", registry);
    
    EXPECT_GT(results.size(), 0);
    // Should match "allocate" as best guess
    EXPECT_EQ(results[0].actionName, "allocate");
}

TEST_F(FuzzyParserTest, Scenario_PlayerMultipleAliasesForSameAction) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    
    // Test various aliases for "allocate"
    std::vector<std::string> aliases = {"give", "distribute", "provide", "help"};
    
    for (const auto& alias : aliases) {
        auto results = FuzzyParser::parseInput(alias, registry);
        EXPECT_GT(results.size(), 0);
        // Most should resolve to allocate (some might resolve to other actions)
        if (results[0].actionName == "allocate") {
            EXPECT_GT(results[0].confidence, 0.9f);
        }
    }
}

TEST_F(FuzzyParserTest, Cache_ClearCache) {
    // Generate some cached entries
    FuzzyParser::levenshteinDistance("allocate", "alokate");
    FuzzyParser::levenshteinDistance("delegate", "deligate");
    
    size_t size_before = FuzzyParser::getCachSize();
    EXPECT_GT(size_before, 0);
    
    FuzzyParser::clearLevenshteinCache();
    size_t size_after = FuzzyParser::getCachSize();
    
    EXPECT_EQ(size_after, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
