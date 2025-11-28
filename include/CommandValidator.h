#pragma once

#include <string>
#include <vector>
#include <memory>
#include "InputParser.h"           // Includes ParseResult
#include "ParameterExtractor.h"    // Includes ExtractedParameters

namespace TLS {

// Forward declarations
class NPC;
class Faction;
class Resource;
class NPCRegistry;
class FactionRegistry;
class ResourceRegistry;
// struct ParseResult;         // Now included from InputParser.h
// struct ExtractedParameters; // Now included from ParameterExtractor.h

/**
 * @struct ValidationError
 * @brief Describes a single validation failure with context and suggestions
 */
struct ValidationError {
    enum Severity { INFO, WARNING, ERROR, CRITICAL };
    
    Severity severity;
    std::string code;              // Machine-readable error code (e.g., "NPC_NOT_FOUND")
    std::string message;           // Human-readable error description
    std::string field;             // Which field failed (e.g., "target_npc", "quantity")
    std::string invalidValue;      // The problematic value
    std::vector<std::string> suggestions;  // Suggestions for correction
};

/**
 * @struct ValidationResult
 * @brief Complete validation outcome with errors and warnings
 */
struct ValidationResult {
    bool isValid;                  // True if all critical errors cleared
    bool hasWarnings;              // True if non-critical issues found
    int errorCount;
    int warningCount;
    
    std::vector<ValidationError> errors;
    std::vector<ValidationError> warnings;
    
    // Additional context
    std::string summary;           // Brief human-readable validation status
    float confidence;              // 0.0-1.0: overall command confidence
};

/**
 * @class CommandValidator
 * @brief Validates parsed commands against world state and game rules
 * 
 * Responsibilities:
 *  - Verify all entities (NPCs, factions, resources) exist in registries
 *  - Check quantity bounds and resource availability
 *  - Validate permission/authorization (player can execute this action)
 *  - Check for logical conflicts (e.g., can't allocate resource that doesn't exist)
 *  - Generate helpful error messages and correction suggestions
 *  - Support dry-run validation (check without executing)
 * 
 * Workflow:
 *  1. Receive ParseResult from FuzzyParser + ExtractedParameters from ParameterExtractor
 *  2. Check each parameter type against corresponding registry
 *  3. Validate quantity bounds and special rules
 *  4. Collect errors, warnings, suggestions
 *  5. Return ValidationResult with actionable feedback
 */
class CommandValidator {
public:
    /**
     * Validate a complete parsed command
     * 
     * @param action Action name (e.g., "allocate", "delegate")
     * @param parameters Extracted parameters from parser
     * @param npcRegistry Registry for NPC entity validation
     * @param factionRegistry Registry for faction entity validation
     * @param resourceRegistry Registry for resource entity validation
     * @return ValidationResult with errors/warnings
     */
    static ValidationResult validateCommand(
        const std::string& action,
        const ExtractedParameters& parameters,
        const NPCRegistry& npcRegistry,
        const FactionRegistry& factionRegistry,
        const ResourceRegistry& resourceRegistry
    );
    
    /**
     * Validate a single NPC entity exists and is accessible
     * 
     * @param npcId NPC ID to validate
     * @param registry NPCRegistry
     * @return ValidationError if invalid, error with severity=INFO if valid
     */
    static ValidationError validateNPCExists(int npcId, const NPCRegistry& registry);
    
    /**
     * Validate a single faction exists and is accessible
     * 
     * @param factionId Faction ID to validate
     * @param registry FactionRegistry
     * @return ValidationError if invalid
     */
    static ValidationError validateFactionExists(int factionId, const FactionRegistry& registry);
    
    /**
     * Validate a resource exists and has minimum availability
     * 
     * @param resourceId Resource ID to validate
     * @param minQuantity Minimum required quantity (default 1)
     * @param registry ResourceRegistry
     * @return ValidationError if invalid or insufficient
     */
    static ValidationError validateResourceAvailable(
        int resourceId,
        int minQuantity,
        const ResourceRegistry& registry
    );
    
    /**
     * Validate quantity is within acceptable bounds
     * 
     * @param quantity The quantity to validate
     * @param actionName Action context (for action-specific bounds)
     * @param minBound Minimum acceptable value
     * @param maxBound Maximum acceptable value
     * @return ValidationError if out of bounds
     */
    static ValidationError validateQuantityBounds(
        int quantity,
        const std::string& actionName,
        int minBound = 1,
        int maxBound = 1000
    );
    
    /**
     * Validate that target NPC is not player (can't target self)
     * 
     * @param npcId NPC ID to check
     * @return ValidationError if NPC is player
     */
    static ValidationError validateNotSelf(int npcId);
    
    /**
     * Validate that player has permission to execute action on target
     * (Future: faction alignment, player authority, prerequisites)
     * 
     * @param action Action name
     * @param targetType "NPC" or "Faction"
     * @param targetId Entity ID
     * @return ValidationError if permission denied
     */
    static ValidationError validatePermission(
        const std::string& action,
        const std::string& targetType,
        int targetId
    );
    
    /**
     * Generate correction suggestions for common errors
     * 
     * @param errorCode Machine-readable error code (e.g., "NPC_NOT_FOUND")
     * @param invalidValue The value that caused error
     * @param candidates Vector of valid alternatives to suggest
     * @return Vector of 2-3 best suggestions
     */
    static std::vector<std::string> generateSuggestions(
        const std::string& errorCode,
        const std::string& invalidValue,
        const std::vector<std::string>& candidates
    );
    
    /**
     * Create human-readable error message with context
     * 
     * @param error ValidationError to format
     * @return Formatted message: "{message} [{field}] {suggestions}"
     */
    static std::string formatErrorMessage(const ValidationError& error);
    
    /**
     * Dry-run validation without side effects
     * Returns what would happen if command executed, without actual changes
     * 
     * @param action Action name
     * @param parameters Parameters
     * @param npcRegistry NPC registry (const, no mutations)
     * @param factionRegistry Faction registry (const, no mutations)
     * @param resourceRegistry Resource registry (const, no mutations)
     * @return ValidationResult (same as validateCommand, but read-only check)
     */
    static ValidationResult dryRun(
        const std::string& action,
        const ExtractedParameters& parameters,
        const NPCRegistry& npcRegistry,
        const FactionRegistry& factionRegistry,
        const ResourceRegistry& resourceRegistry
    );
    
    // Error code constants (public for testing and logging)
    static const std::string ERR_NPC_NOT_FOUND;
    static const std::string ERR_FACTION_NOT_FOUND;
    static const std::string ERR_RESOURCE_NOT_FOUND;
    static const std::string ERR_INSUFFICIENT_RESOURCES;
    static const std::string ERR_QUANTITY_OUT_OF_BOUNDS;
    static const std::string ERR_INVALID_TARGET;
    static const std::string ERR_PERMISSION_DENIED;
    static const std::string ERR_SELF_TARGET;
    static const std::string ERR_INVALID_ACTION;

private:
    // Constants for validation bounds
    static constexpr int MIN_QUANTITY = 0;
    static constexpr int MAX_QUANTITY = 10000;
    
    // Helper methods
    static void addError(
        ValidationResult& result,
        ValidationError::Severity severity,
        const std::string& code,
        const std::string& message,
        const std::string& field,
        const std::string& invalidValue
    );
    
    static void addWarning(
        ValidationResult& result,
        const std::string& code,
        const std::string& message,
        const std::string& field
    );
    
    static void updateSummary(ValidationResult& result);
};

}  // namespace TLS
