#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations in TLS namespace to avoid circular dependencies
namespace TLS {
class NPC;
class Faction;
class Resource;
class NPCRegistry;
class FactionRegistry;
class ResourceRegistry;
}

namespace TLS {

/**
 * Phase 5.3: Parameter Extractor - Convert raw parameters to typed world state entities
 * 
 * Responsibilities:
 *  - Extract NPC names with fuzzy matching against NPCRegistry
 *  - Extract faction names with fuzzy matching against FactionRegistry
 *  - Extract resource types with fuzzy matching against ResourceRegistry
 *  - Parse quantities and validate bounds
 *  - Extract tone/style from context (positive, neutral, negative, aggressive)
 *  - Handle location references
 *  - Return structured ExtractedParameters with confidence scores and validation results
 */

/**
 * Represents a single extracted parameter with type info
 */
struct ExtractedParameter {
    enum Type {
        NPC_ENTITY,          // Resolved to NPC* from registry
        FACTION_ENTITY,      // Resolved to Faction* from registry
        RESOURCE_ENTITY,     // Resolved to Resource* from registry
        QUANTITY,            // Integer value
        TONE,                // Positive, neutral, negative, aggressive
        LOCATION,            // String location name
        UNKNOWN              // Could not classify
    };
    
    Type type;
    std::string rawValue;         // Original string from input
    float confidence;             // 0.0-1.0 match confidence
    std::string resolvedName;     // Matched entity name (after fuzzy matching)
    
    // Pointers to resolved entities (null if not found)
    void* entityPtr;              // NPC*, Faction*, or Resource* (use type to interpret)
    
    // Validation info
    bool isValid;                 // True if entity found in registry
    std::string validationError;  // If not valid, why?
};

/**
 * Result of parameter extraction
 */
struct ExtractedParameters {
    std::vector<ExtractedParameter> parameters;  // All extracted parameters
    std::string tone;                            // overall tone: "positive", "neutral", "negative", "aggressive"
    float confidenceScore;                       // 0.0-1.0 average confidence across parameters
    int validCount;                              // Number of valid (found in registry) parameters
    int invalidCount;                            // Number of invalid parameters
    std::string extractionReasoning;             // Debug: how were parameters extracted?
};

/**
 * ParameterExtractor - Converts raw parameter strings to typed world state entities
 * 
 * Workflow:
 *  1. Input: action name + vector of raw parameter strings (e.g., ["alice", "50", "farmers"])
 *  2. Classification: Identify each parameter type (NPC? Faction? Resource? Quantity?)
 *  3. Matching: Fuzzy match against registries (60%+ confidence threshold)
 *  4. Extraction: Return structured ExtractedParameters with validation
 */
class ParameterExtractor {
public:
    /**
     * Extract parameters from raw strings
     * 
     * @param actionName Action being executed (e.g., "allocate")
     * @param rawParams Vector of raw parameter strings from FuzzyParser
     * @param npcRegistry NPCRegistry for entity resolution
     * @param factionRegistry FactionRegistry for entity resolution
     * @param resourceRegistry ResourceRegistry for entity resolution
     * @return ExtractedParameters with resolved entities and confidence scores
     */
    static ExtractedParameters extract(
        const std::string& actionName,
        const std::vector<std::string>& rawParams,
        NPCRegistry& npcRegistry,
        FactionRegistry& factionRegistry,
        ResourceRegistry& resourceRegistry
    );
    
    /**
     * Extract a single NPC entity with fuzzy matching
     * 
     * @param npcName Raw NPC name from input
     * @param registry NPCRegistry to search
     * @param confidenceThreshold Minimum confidence for match (default 0.6)
     * @return ExtractedParameter with NPC resolved or validation error
     */
    static ExtractedParameter extractNPC(
        const std::string& npcName,
        NPCRegistry& registry,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Extract a single faction entity with fuzzy matching
     * 
     * @param factionName Raw faction name from input
     * @param registry FactionRegistry to search
     * @param confidenceThreshold Minimum confidence for match (default 0.6)
     * @return ExtractedParameter with Faction resolved or validation error
     */
    static ExtractedParameter extractFaction(
        const std::string& factionName,
        FactionRegistry& registry,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Extract a resource entity with fuzzy matching
     * 
     * @param resourceName Raw resource name from input
     * @param registry ResourceRegistry to search
     * @param confidenceThreshold Minimum confidence for match (default 0.6)
     * @return ExtractedParameter with Resource resolved or validation error
     */
    static ExtractedParameter extractResource(
        const std::string& resourceName,
        ResourceRegistry& registry,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Extract a quantity (integer) parameter
     * 
     * @param quantityStr Raw quantity string (e.g., "50", "100 food")
     * @return ExtractedParameter with quantity parsed and validated
     */
    static ExtractedParameter extractQuantity(const std::string& quantityStr);
    
    /**
     * Extract tone/style from parameter context
     * 
     * @param params Vector of parameter strings
     * @return Tone string: "positive", "neutral", "negative", or "aggressive"
     * 
     * Analysis:
     *  - Keywords: "please", "thank you" → positive
     *  - Keywords: "now", "immediately", "urgent" → aggressive
     *  - Keywords: "if possible", "maybe" → neutral
     *  - Keywords: "must", "demand", "refuse" → negative
     */
    static std::string extractTone(const std::vector<std::string>& params);
    
    /**
     * Classify a single raw parameter by type
     * 
     * @param rawParam Raw parameter string
     * @param actionName Action context (helps determine expected parameter types)
     * @return ExtractedParameter::Type (best guess)
     * 
     * Heuristics:
     *  - Numeric strings → QUANTITY
     *  - All-caps single word → FACTION (conventions)
     *  - Proper-cased single word → NPC (name convention)
     *  - Lowercase word → RESOURCE
     *  - Keywords like "please", "urgent" → TONE
     */
    static ExtractedParameter::Type classifyParameter(
        const std::string& rawParam,
        const std::string& actionName
    );
    
    /**
     * Match entity name against registry with fuzzy matching
     * 
     * Uses Levenshtein distance (reuses FuzzyParser cache where possible)
     * 
     * @param inputName Raw name from player input
     * @param registryNames Vector of known names in registry
     * @param confidenceThreshold Minimum confidence (default 0.6)
     * @return pair<matched_name, confidence> or {"", 0.0} if no match above threshold
     */
    static std::pair<std::string, float> fuzzyMatchName(
        const std::string& inputName,
        const std::vector<std::string>& registryNames,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Parse quantity string, handling variations
     * 
     * Examples:
     *  - "50" → 50
     *  - "50 food" → 50 (ignores resource suffix)
     *  - "0x32" → 50 (hex support)
     *  - "5e1" → 50 (scientific notation)
     *  - "invalid" → -1 (error)
     * 
     * @param quantityStr Raw quantity string
     * @return Parsed integer, or -1 if invalid
     */
    static int parseQuantity(const std::string& quantityStr);
    
    /**
     * Check if string is numeric
     */
    static bool isNumeric(const std::string& str);
    
    /**
     * Extract all NPCs in a parameter list
     * 
     * @param rawParams Vector of raw parameter strings
     * @param registry NPCRegistry for matching
     * @param confidenceThreshold Minimum confidence
     * @return Vector of ExtractedParameters with type=NPC_ENTITY
     */
    static std::vector<ExtractedParameter> extractAllNPCs(
        const std::vector<std::string>& rawParams,
        NPCRegistry& registry,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Extract all factions in a parameter list
     */
    static std::vector<ExtractedParameter> extractAllFactions(
        const std::vector<std::string>& rawParams,
        FactionRegistry& registry,
        float confidenceThreshold = 0.6f
    );
    
    /**
     * Extract all resources in a parameter list
     */
    static std::vector<ExtractedParameter> extractAllResources(
        const std::vector<std::string>& rawParams,
        ResourceRegistry& registry,
        float confidenceThreshold = 0.6f
    );
};

}  // namespace TLS
