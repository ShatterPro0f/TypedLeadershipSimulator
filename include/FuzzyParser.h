#pragma once

#include "ActionRegistry.h"
#include <string>
#include <vector>
#include <map>

/**
 * Phase 5.2: Fuzzy Parser - Input Parsing with Levenshtein Distance
 * 
 * Converts player text input into ranked action matches using:
 *  1. Levenshtein distance (edit distance for typo tolerance)
 *  2. Exact keyword matching
 *  3. Hybrid confidence scoring
 * 
 * Returns sorted list of ParseResults by confidence (highest first)
 */

/**
 * Result of parsing player input
 */
struct FuzzyParseResult {
    std::string actionName;              // Primary action name matched
    std::vector<std::string> rawParams;  // Unvalidated extracted parameters
    float confidence;                    // 0.0-1.0 confidence score
    std::string matchType;               // "exact", "fuzzy", "alias"
    int editDistance;                    // Levenshtein distance (if fuzzy match)
    std::string reasoning;               // Debug: why this score?
};

/**
 * Fuzzy Parser - converts typed input to action matches with tolerance for typos
 * 
 * Responsibilities:
 *  - Calculate Levenshtein distance with caching
 *  - Score all actions by hybrid confidence model
 *  - Extract parameters from input
 *  - Return ranked list of matches
 */
class FuzzyParser {
private:
    // Caching for Levenshtein distances to avoid recomputation
    static std::map<std::pair<std::string, std::string>, int> levenshteinCache_;
    
    // Statistics for confidence scoring refinement
    static std::map<std::string, int> actionUsageFrequency_;
    
public:
    /**
     * Parse player input and return ranked list of action matches
     * @param input Raw player text input (e.g., "allocate 50 food to farmers")
     * @param registry Action registry with all valid actions
     * @return Vector of FuzzyParseResults sorted by confidence (highest first)
     */
    static std::vector<FuzzyParseResult> parseInput(
        const std::string& input,
        const ActionRegistry& registry
    );
    
    /**
     * Calculate Levenshtein distance between two strings
     * Uses cached results to avoid recomputation
     * Implementation: dynamic programming with early termination optimization
     * 
     * @param a First string
     * @param b Second string
     * @param maxDistance Stop calculation if distance exceeds this (optimization)
     * @return Edit distance (number of character edits required)
     * 
     * Example: levenshteinDistance("allocate", "alokate") = 1 (typo: missing c)
     */
    static int levenshteinDistance(
        const std::string& a,
        const std::string& b,
        int maxDistance = 3
    );
    
    /**
     * Calculate hybrid confidence score for a match
     * Formula: confidence = w_exact*C_exact + w_fuzzy*C_fuzzy + w_semantic*C_semantic
     * 
     * Components:
     *  - C_exact = 1.0 if full match, 0.95 if substring, 0.0 otherwise
     *  - C_fuzzy = 1.0 - (levenshteinDistance / maxDist) for typo tolerance
     *  - C_semantic = character overlap ratio (proxy for semantic similarity)
     * 
     * Weights (adjustable):
     *  - w_exact = 0.3 (prefer exact matches)
     *  - w_fuzzy = 0.4 (heavy weighting for typo tolerance)
     *  - w_semantic = 0.3 (light weighting, may add word embeddings later)
     * 
     * @param input Player input text (e.g., "feed farmers")
     * @param actionName Known action name or alias
     * @param w_exact Weight for exact match (default 0.3)
     * @param w_fuzzy Weight for fuzzy match (default 0.4)
     * @param w_semantic Weight for semantic (default 0.3)
     * @return Confidence score 0.0-1.0
     */
    static float calculateConfidence(
        const std::string& input,
        const std::string& actionName,
        float w_exact = 0.3f,
        float w_fuzzy = 0.4f,
        float w_semantic = 0.3f
    );
    
    /**
     * Extract parameters from player input (basic version)
     * Splits input by spaces and filters out known keywords
     * More sophisticated parameter extraction in ParameterExtractor.cpp
     * 
     * @param input Player input (e.g., "allocate 50 food to farmers")
     * @param actionName Matched action name
     * @return Vector of extracted parameter strings (e.g., ["50", "food", "farmers"])
     */
    static std::vector<std::string> extractParametersBasic(
        const std::string& input,
        const std::string& actionName
    );
    
    /**
     * Clear the Levenshtein cache
     * Call periodically to manage memory
     */
    static void clearLevenshteinCache();
    
    /**
     * Get cache size (for debugging/monitoring)
     */
    static size_t getCachSize();
    
    /**
     * Helper: convert string to lowercase
     */
    static std::string toLowercase(const std::string& str);
    
    /**
     * Helper: trim whitespace from string
     */
    static std::string trim(const std::string& str);
    
    /**
     * Helper: split string by delimiter
     */
    static std::vector<std::string> split(
        const std::string& str,
        char delimiter = ' '
    );
    
    /**
     * Helper: calculate character overlap between two strings
     * Used for semantic similarity proxy
     */
    static float calculateCharacterOverlap(
        const std::string& a,
        const std::string& b
    );
};
