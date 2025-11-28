#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace TLS {

// Forward declarations
class NPC;
class NPCRegistry;
class FactionRegistry;
class ResourceRegistry;

/**
 * @struct ParseResult
 * @brief Result of fuzzy matching confidence scoring
 */
struct ParseResult {
    std::string action;           // Matched action name
    float confidence;             // 0.0-1.0 confidence score
    int usageFrequency;          // How often this action is used
    std::vector<std::string> parameters;  // Extracted parameters
};

/**
 * @struct InterpretedDecision
 * @brief Fully interpreted player decision ready for simulation execution
 */
struct InterpretedDecision {
    std::string action;          // allocate, delegate, inspire, suppress, negotiate, command, etc.
    int targetNpcId;             // Primary target NPC (-1 if faction/resource only)
    int targetFactionId;         // Target faction (-1 if NPC only)
    std::string targetResource;  // Resource name if applicable
    std::string tone;            // positive, neutral, negative, aggressive, diplomatic
    int priority;                // 1-10
    float quantity;              // Numeric parameter if applicable
    std::string context;         // Contextual narrative
    
    // For LLM callback tracking
    int requestId;
    bool success;
    std::string error;
};

/**
 * @class ActionRegistry
 * @brief Central definition of all allowed simulation actions
 *
 * Extensible registry mapping action names to parameters and consequence formulas.
 * Used for both validation and documentation of player-executable commands.
 */
class ActionRegistry {
public:
    struct Parameter {
        std::string name;
        enum Type { NPC, FACTION, RESOURCE, QUANTITY, STRING } type;
        bool required = true;
    };

    struct ActionDefinition {
        std::string name;
        std::vector<std::string> aliases;     // "give", "distribute", "help" all map to "allocate"
        std::vector<Parameter> parameters;
        std::string formula;                  // Reference to Equations.txt formula
        std::string description;
        float minQuantity = 0.0f;
        float maxQuantity = 1000.0f;
    };

    static ActionRegistry& getInstance();

    // Load action definitions from JSON file
    bool loadFromFile(const std::string& filename);

    // Get action definition by name
    const ActionDefinition* getAction(const std::string& name) const;

    // Get all actions
    std::vector<std::string> getAllActionNames() const;

    // Register custom action
    void registerAction(const ActionDefinition& def);

private:
    ActionRegistry();
    ActionRegistry(const ActionRegistry&) = delete;
    ActionRegistry& operator=(const ActionRegistry&) = delete;

    std::map<std::string, ActionDefinition> actions_;
    
    // Initialize with built-in actions
    void initializeBuiltins();
};

/**
 * @class InputParser
 * @brief Parses freeform player input into deterministic simulation commands
 *
 * Responsibility: Convert natural language player decisions into structured commands
 * that can be executed by the simulation. Uses fuzzy matching and confidence scoring
 * to disambiguate ambiguous inputs.
 *
 * Workflow:
 * 1. Player types: "give food to farmers"
 * 2. Fuzzy match to known actions: "allocate" (0.95 confidence)
 * 3. Extract parameters: target=farmers (faction), resource=food, amount=10
 * 4. Validate parameters exist in world
 * 5. Return ParseResult or prompt for disambiguation
 * 6. (Optional) Send to LLM for tone/context interpretation
 * 7. Return InterpretedDecision ready for simulation execution
 */
class InputParser {
public:
    static InputParser& getInstance();

    // Main parsing entry point
    ParseResult parseInput(const std::string& input, const ActionRegistry& registry);

    // Validate parsed parameters exist in game world
    bool validateParameters(const ParseResult& parseResult,
                           const NPCRegistry& npcRegistry,
                           const FactionRegistry& factionRegistry,
                           const ResourceRegistry& resourceRegistry);

    // Fuzzy match confidence scoring
    float calculateConfidence(const std::string& input, const std::string& knownAction);

    // Handle ambiguous inputs (multiple matches)
    std::vector<ParseResult> findAmbiguousMatches(const std::string& input,
                                                   const ActionRegistry& registry,
                                                   float threshold = 0.7f);

    // Extract numeric quantities from text
    float extractQuantity(const std::string& input);

    // Extract NPC names from text
    std::vector<std::string> extractNPCNames(const std::string& input,
                                             const NPCRegistry& registry);

    // Extract faction names from text
    std::vector<std::string> extractFactionNames(const std::string& input,
                                                 const FactionRegistry& registry);

    // Extract resource names from text
    std::vector<std::string> extractResourceNames(const std::string& input,
                                                  const ResourceRegistry& registry);

    // Detect tone/intent from player input
    std::string detectTone(const std::string& input);

private:
    InputParser();
    InputParser(const InputParser&) = delete;
    InputParser& operator=(const InputParser&) = delete;

    // Hybrid confidence model (three components)
    static constexpr float LEVENSHTEIN_WEIGHT = 0.4f;
    static constexpr float EMBEDDING_WEIGHT = 0.3f;
    static constexpr float EXACT_WEIGHT = 0.3f;
    static constexpr int LEVENSHTEIN_MAX_DIST = 3;

    // Levenshtein distance calculation
    int levenshteinDistance(const std::string& a, const std::string& b) const;

    // Word embedding similarity (placeholder for future ML integration)
    float wordEmbeddingSimilarity(const std::string& input, const std::string& known) const;

    // Exact match scoring
    float exactMatchScore(const std::string& input, const std::string& known) const;

    // Tone detection keywords
    std::map<std::string, std::string> toneKeywords_;
    void initializeToneKeywords();
};

}  // namespace TLS
