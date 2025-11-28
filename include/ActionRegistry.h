#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <optional>

/**
 * Phase 5: Expanded Action Registry System (v2.0)
 * 
 * Centralized, data-driven registry of 25+ valid player actions.
 * Actions are defined in data/action_registry.json and loaded at startup.
 * 
 * Design Goals:
 *  1. Support large action space (25+ semantic actions)
 *  2. Enable parameter variation (intensity, scope, timing, method)
 *  3. Constrain LLM output to valid actions only (three-layer validation)
 *  4. Allow fuzzy matching for typos and synonyms
 *  5. Provide consequence formulas for deterministic simulation updates
 *  6. Cascade risk ratings for event generation
 * 
 * Each action specifies:
 *  - ID (unique identifier)
 *  - Name and aliases (synonyms for fuzzy matching)
 *  - Tags (for categorization: economic, military, diplomatic, etc.)
 *  - Parameters with options (intensity, scope, timing, method)
 *  - Consequence formula (reference to Equations.txt)
 *  - Cascade risk rating (0.0-1.0)
 *  - Priority (for tie-breaking in ambiguous matches)
 *  - Confidence threshold (min confidence to auto-execute)
 */

/**
 * Parameter types for action parameters
 */
enum class ParameterType {
    RESOURCE = 0,           // Food, wood, water, etc.
    NPC = 1,                // Specific NPC by name
    FACTION = 2,            // Specific faction
    NPC_OR_FACTION = 3,     // Either NPC or faction
    QUANTITY = 4,           // Number value
    STRING = 5,             // Free-form text
    FLOAT = 6,              // Floating-point value
    BOOLEAN = 7,            // True/false flag
    TONE = 8,               // Implicit tone from input
    LOCATION = 9            // 3D location (future)
};

/**
 * Individual parameter specification with validation constraints
 */
struct ActionParameter {
    std::string name;                           // "intensity", "scope", "method", etc.
    ParameterType type;                         // Type of parameter
    bool required;                              // Must be provided?
    float minValue;                             // For FLOAT type
    float maxValue;                             // For FLOAT type
    float defaultValue;                         // Default if not specified
    std::vector<std::string> options;           // Valid enum values for STRING type
    
    std::string typeToString() const {
        switch (type) {
            case ParameterType::RESOURCE: return "RESOURCE";
            case ParameterType::NPC: return "NPC";
            case ParameterType::FACTION: return "FACTION";
            case ParameterType::NPC_OR_FACTION: return "NPC_OR_FACTION";
            case ParameterType::QUANTITY: return "QUANTITY";
            case ParameterType::STRING: return "STRING";
            case ParameterType::FLOAT: return "FLOAT";
            case ParameterType::BOOLEAN: return "BOOLEAN";
            case ParameterType::TONE: return "TONE";
            case ParameterType::LOCATION: return "LOCATION";
            default: return "UNKNOWN";
        }
    }
};

/**
 * Expanded Action definition: specifies what a player action is
 */
struct ActionDefinition {
    int id;                                     // Unique action ID (1-25+)
    std::string name;                           // Primary action name: "allocate"
    std::vector<std::string> aliases;           // Synonyms: ["give", "distribute", "provide"]
    std::vector<std::string> tags;              // Categories: ["economic", "positive", "morale"]
    std::vector<ParameterType> parameterTypes;  // Main parameter types (for backward compat)
    std::vector<ActionParameter> parameters;    // Expanded parameter specifications
    std::string consequenceFormula;             // Reference: "loyalty_delta = 0.05 * (amount / population) * tone_multiplier * intensity"
    std::string description;                    // User-facing description
    float cascadeRisk;                          // Probability of cascading events (0.0-1.0)
    int priority;                               // 1-10, for tie-breaking (higher = preferred)
    bool requiresConfirmation;                  // Ask Y/N before execution?
    float confidenceThreshold;                  // Minimum confidence to auto-execute (0.0-1.0)
    
    // Helper to convert parameter type to string
    std::string paramTypeToString(ParameterType type) const {
        switch (type) {
            case ParameterType::RESOURCE: return "RESOURCE";
            case ParameterType::NPC: return "NPC";
            case ParameterType::FACTION: return "FACTION";
            case ParameterType::NPC_OR_FACTION: return "NPC_OR_FACTION";
            case ParameterType::QUANTITY: return "QUANTITY";
            case ParameterType::STRING: return "STRING";
            case ParameterType::FLOAT: return "FLOAT";
            case ParameterType::BOOLEAN: return "BOOLEAN";
            case ParameterType::TONE: return "TONE";
            case ParameterType::LOCATION: return "LOCATION";
            default: return "UNKNOWN";
        }
    }
};

/**
 * Central ActionRegistry singleton
 * 
 * Responsibilities:
 *  1. Load action definitions from JSON (25+ actions)
 *  2. Provide O(1) lookup by name and alias
 *  3. Support fuzzy matching for typo tolerance
 *  4. Validate action parameters against specifications
 *  5. Constrain LLM output to valid actions (three-layer defense)
 *  6. Provide cascade risk ratings for event generation
 */
class ActionRegistry {
private:
    static ActionRegistry* instance_;
    std::vector<ActionDefinition> actions_;
    std::unordered_map<std::string, size_t> actionsByName_;      // name -> actions_ index
    std::unordered_map<std::string, size_t> actionsByAlias_;     // alias -> actions_ index
    std::unordered_map<std::string, size_t> actionsByID_;        // id -> actions_ index
    std::unordered_map<std::string, std::vector<std::string>> globalParameters_;  // tone types, scope types, etc.
    std::string version_;
    std::string description_;
    
    // Private constructor for singleton
    ActionRegistry();
    
    // Index building
    void buildIndexes();
    
public:
    // Singleton access
    static ActionRegistry& getInstance();
    static void initialize();
    static void shutdown();
    
    // Initialization
    bool loadFromJSON(const std::string& filename);
    
    // Lookup operations (O(1))
    ActionDefinition* getActionByName(const std::string& name) const;
    ActionDefinition* getActionByAlias(const std::string& alias) const;
    ActionDefinition* getActionByID(int id) const;
    
    // Fuzzy matching for typo tolerance
    std::optional<ActionDefinition*> findActionByFuzzyMatch(const std::string& input, int maxDistance = 3) const;
    
    // List operations
    const std::vector<ActionDefinition>& getAllActions() const;
    std::vector<std::string> getAllActionNames() const;
    std::vector<std::string> getAllAliases() const;
    std::vector<ActionDefinition*> getActionsByTag(const std::string& tag) const;
    std::vector<ActionDefinition*> getActionsByCascadeRisk(float minRisk, float maxRisk) const;
    
    // Parameter validation
    bool isValidAction(const std::string& actionName) const;
    bool isValidAlias(const std::string& alias) const;
    bool validateParameter(const std::string& actionName, const std::string& paramName, const std::string& paramValue) const;
    std::optional<ActionParameter> getParameter(const std::string& actionName, const std::string& paramName) const;
    
    // Global parameters
    const std::unordered_map<std::string, std::vector<std::string>>& getGlobalParameters() const;
    std::vector<std::string> getToneTypes() const;
    std::vector<std::string> getScopeTypes() const;
    std::vector<std::string> getTimingTypes() const;
    std::vector<std::string> getMethodTypes() const;
    
    // Utility
    std::vector<ActionDefinition*> getActionsByPriority() const;
    std::vector<std::string> getAllTags() const;
    static int levenshteinDistance(const std::string& s1, const std::string& s2);
    size_t getActionCount() const { return actions_.size(); }
    std::string getVersion() const { return version_; }
    
    // Debugging
    void printRegistry() const;
    void printAction(const ActionDefinition& action) const;
    
    // Prevent copy/move
    ActionRegistry(const ActionRegistry&) = delete;
    ActionRegistry& operator=(const ActionRegistry&) = delete;
    
    ~ActionRegistry();
};
