#ifndef DECISION_INTERPRETER_H
#define DECISION_INTERPRETER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <limits>
#include <cctype>
#include <cmath>
#include "Enums.h"
#include "Vector3.h"

namespace TLS {

// Forward declarations
class LLMProvider;
class NPC;
class Faction;
class Resource;
class NPCRegistry;
class FactionRegistry;
class ResourceRegistry;
struct LLMResponse;

// ==================== ENUMS ====================

enum class ActionType {
    UNKNOWN = 0,
    ALLOCATE = 1,
    DELEGATE = 2,
    NEGOTIATE = 3,
    INSPIRE = 4,
    SUPPRESS = 5,
    COMMAND = 6,
    SUPPRESS_FACTION = 7
};

enum class ToneType {
    POSITIVE = 1,
    NEUTRAL = 0,
    NEGATIVE = -1
};

enum class ParameterType {
    RESOURCE = 0,
    NPC_OR_FACTION = 1,
    QUANTITY = 2,
    LOCATION = 3
};

enum class TargetType {
    UNKNOWN_TARGET = 0,
    NPC = 1,
    FACTION = 2,
    RESOURCE = 3,
    CULTURE = 4,
    RELIGION = 5
};

// ==================== STRUCTS ====================

struct ExtractedParameter {
    ParameterType type;
    std::string value;
    int id = -1;  // For NPC/Faction/Resource ID lookup
    float confidence;

    ExtractedParameter(ParameterType t, const std::string& v, float c = 1.0f)
        : type(t), value(v), confidence(c) {}
};

struct ParseResult {
    bool isValid = false;
    ActionType actionType = ActionType::UNKNOWN;
    float confidence = 0.0f;
    std::vector<ExtractedParameter> parameters;
    std::vector<std::string> errors;
    std::vector<std::string> suggestions;
    bool requiresConfirmation = false;
};

struct ToneResult {
    ToneType tone = ToneType::NEUTRAL;
    float confidence = 0.5f;
    std::string reasoning;
    bool usedLLM = false;

    ToneResult() = default;
    ToneResult(ToneType t, float c, const std::string& r, bool llm = false)
        : tone(t), confidence(c), reasoning(r), usedLLM(llm) {}
};

struct ValidationResult {
    bool isValid = false;
    std::vector<std::string> errors;
    std::vector<std::string> suggestions;
    TargetType targetType = TargetType::UNKNOWN_TARGET;
};

struct SimulationImpact {
    int affectedNPCId = -1;
    int affectedFactionId = -1;
    int affectedResourceId = -1;
    
    float loyaltyDelta = 0.0f;
    float moodDelta = 0.0f;
    float moodMultiplier = 1.0f;
    
    int resourceQuantityChange = 0;
    int factionAlignmentChange = 0;
    
    std::string impactDescription;
};

struct Decision {
    std::string playerInput;
    ActionType actionType = ActionType::UNKNOWN;
    std::vector<ExtractedParameter> parameters;
    ToneType tone = ToneType::NEUTRAL;
    float toneMultiplier = 1.0f;
    float confidence = 0.0f;
    
    // World state references
    TargetType targetType = TargetType::UNKNOWN_TARGET;
    int targetNPCId = -1;
    int targetFactionId = -1;
    int targetResourceId = -1;
    int targetQuantity = 0;
    
    // Results
    std::string narrativeFlavor;
    std::vector<SimulationImpact> impacts;
    std::vector<int> affectedNPCIds;
    std::vector<int> affectedResourceIds;
    std::vector<int> affectedFactionIds;
    
    // LLM context
    bool usedLLM = false;
    std::string llmContext;
};

// ==================== CLASS DECLARATIONS ====================

class InputParser {
public:
    // Normalization and fuzzy matching
    static std::string normalizeInput(const std::string& input);
    static float calculateLevenshteinDistance(const std::string& a, const std::string& b);
    static float calculateConfidenceScore(
        float levenshteinScore,
        float exactMatchScore,
        float semanticScore = 0.5f
    );
    static ActionType extractActionType(const std::string& normalizedInput);
    static std::string findClosestMatch(
        const std::string& input,
        const std::vector<std::string>& candidates,
        float& bestDistance
    );
    
    // Full parsing with world state context
    static ParseResult parsePlayerInput(const std::string& input);
    static ParseResult parsePlayerInputWithContext(
        const std::string& input,
        const std::vector<std::string>& contextNPCs,
        const std::vector<std::string>& contextFactions,
        const std::vector<std::string>& contextResources
    );
};

class ParameterExtractor {
public:
    // Extract parameters from input text
    static std::vector<ExtractedParameter> extractParameters(
        const std::string& input,
        ActionType actionType
    );
    
    // World state-aware extraction with registry lookups
    static std::vector<ExtractedParameter> extractParametersWithWorldState(
        const std::string& input,
        ActionType actionType
    );
    
    // Specific extractors
    static int extractQuantity(const std::string& input);
    static int extractNPCId(const std::string& input);
    static int extractFactionId(const std::string& input);
    static int extractResourceId(const std::string& input);
    static TargetType detectTargetType(const std::vector<ExtractedParameter>& params);
};

class ToneDetector {
public:
    static const std::vector<std::string> POSITIVE_KEYWORDS;
    static const std::vector<std::string> NEGATIVE_KEYWORDS;

    // Rule-based tone detection
    static float scoreTone(
        const std::string& input,
        const std::vector<std::string>& keywords
    );
    static ToneResult detectToneRuleBased(const std::string& input);
    
    // LLM-based tone detection with fallback
    static ToneResult detectTone(
        const std::string& input,
        LLMProvider* llmProvider = nullptr,
        bool allowFallback = true
    );
    
    // Tone detection with context (NPC relationships, faction alignment)
    static ToneResult detectToneWithContext(
        const std::string& input,
        int targetNPCId = -1,
        int targetFactionId = -1,
        LLMProvider* llmProvider = nullptr
    );
};

class DecisionValidator {
public:
    // Basic validation
    static ValidationResult validate(const Decision& decision);
    
    // World state-aware validation
    static ValidationResult validateWithWorldState(const Decision& decision);
    
    // Specific validators
    static bool validateNPCExists(int npcId);
    static bool validateFactionExists(int factionId);
    static bool validateResourceExists(int resourceId);
    static bool validateQuantity(int quantity, int available);
};

class DecisionExecutor {
public:
    // Get tone-based multiplier
    static float getToneMultiplier(ToneType tone);
    
    // Execute decision with simulation parameter application
    static void executeDecision(const Decision& decision);
    
    // Apply Phase 2 equations (loyalty/mood calculations)
    static SimulationImpact applyLoyaltyDeltaEquation(
        int npcId,
        ActionType action,
        float toneMultiplier,
        int quantity = 0
    );
    static SimulationImpact applyMoodDeltaEquation(
        int npcId,
        float loyaltyDelta,
        ToneType tone
    );
    
    // Apply consequences to world state
    static void applyNPCConsequences(const std::vector<SimulationImpact>& impacts);
    static void applyFactionConsequences(int factionId, const SimulationImpact& impact);
    static void applyResourceConsequences(int resourceId, int quantityChange);
    
    // Cascade effects (faction reactions, migrations, etc.)
    static void processCascades(const Decision& decision, const std::vector<SimulationImpact>& impacts);
};

class ResultDisplay {
public:
    // Format decision results for player display
    static std::string formatResult(const Decision& decision);
    static std::string formatImpact(const SimulationImpact& impact);
    static std::string formatMultipleImpacts(const std::vector<SimulationImpact>& impacts);
    
    // Build context for display
    static std::string buildNPCReaction(int npcId, const SimulationImpact& impact);
    static std::string buildFactionReaction(int factionId, const SimulationImpact& impact);
    static std::string buildResourceReport(int resourceId, int quantityChange);
};

class DecisionProcessor {
public:
    // Full pipeline: parse → extract → detect tone → validate → execute → display
    static Decision processPlayerInput(
        const std::string& input,
        LLMProvider* llmProvider = nullptr,
        bool executeImmediately = true
    );
    
    // Process with world state context
    static Decision processPlayerInputWithContext(
        const std::string& input,
        const std::vector<std::string>& contextNPCs,
        const std::vector<std::string>& contextFactions,
        const std::vector<std::string>& contextResources,
        LLMProvider* llmProvider = nullptr,
        bool executeImmediately = true
    );
    
    // Generate result string
    static std::string getDecisionResultString(const Decision& decision);
    
    // Build LLM context from world state
    static std::string buildLLMContext(const Decision& decision);
};

// ==================== UTILITY FUNCTIONS ====================

// Get registries for world state access
NPCRegistry& getNPCRegistry();
FactionRegistry& getFactionRegistry();
ResourceRegistry& getResourceRegistry();

}  // namespace TLS

#endif // DECISION_INTERPRETER_H
