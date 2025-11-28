#include "CommandValidator.h"
#include "ParameterExtractor.h"
#include "Registries.h"
#include "Core.h"
#include <algorithm>
#include <sstream>

namespace TLS {

// ============================================================================
// Static error code constants
// ============================================================================

const std::string CommandValidator::ERR_NPC_NOT_FOUND = "NPC_NOT_FOUND";
const std::string CommandValidator::ERR_FACTION_NOT_FOUND = "FACTION_NOT_FOUND";
const std::string CommandValidator::ERR_RESOURCE_NOT_FOUND = "RESOURCE_NOT_FOUND";
const std::string CommandValidator::ERR_INSUFFICIENT_RESOURCES = "INSUFFICIENT_RESOURCES";
const std::string CommandValidator::ERR_QUANTITY_OUT_OF_BOUNDS = "QUANTITY_OUT_OF_BOUNDS";
const std::string CommandValidator::ERR_INVALID_TARGET = "INVALID_TARGET";
const std::string CommandValidator::ERR_PERMISSION_DENIED = "PERMISSION_DENIED";
const std::string CommandValidator::ERR_SELF_TARGET = "SELF_TARGET";
const std::string CommandValidator::ERR_INVALID_ACTION = "INVALID_ACTION";

// ============================================================================
// Main validation entry point
// ============================================================================

ValidationResult CommandValidator::validateCommand(
    const std::string& action,
    const ExtractedParameters& parameters,
    const NPCRegistry& npcRegistry,
    const FactionRegistry& factionRegistry,
    const ResourceRegistry& resourceRegistry
) {
    ValidationResult result;
    result.isValid = true;
    result.hasWarnings = false;
    result.errorCount = 0;
    result.warningCount = 0;
    result.confidence = parameters.confidenceScore;
    result.summary = "Validation passed";
    
    // Validate each extracted parameter
    for (const auto& param : parameters.parameters) {
        switch (param.type) {
            case ExtractedParameter::NPC_ENTITY: {
                if (param.entityPtr == nullptr) {
                    addError(result, ValidationError::ERROR, ERR_NPC_NOT_FOUND,
                            "NPC '" + param.rawValue + "' not found in settlement",
                            "target_npc", param.rawValue);
                } else {
                    // Validate NPC is accessible
                    auto npc = reinterpret_cast<NPC*>(param.entityPtr);
                    if (npc == nullptr) {
                        addError(result, ValidationError::ERROR, ERR_INVALID_TARGET,
                                "NPC reference is invalid",
                                "target_npc", param.rawValue);
                    }
                }
                break;
            }
            
            case ExtractedParameter::FACTION_ENTITY: {
                if (param.entityPtr == nullptr) {
                    addError(result, ValidationError::ERROR, ERR_FACTION_NOT_FOUND,
                            "Faction '" + param.rawValue + "' not found",
                            "target_faction", param.rawValue);
                } else {
                    // Validate faction is accessible
                    auto faction = reinterpret_cast<Faction*>(param.entityPtr);
                    if (faction == nullptr) {
                        addError(result, ValidationError::ERROR, ERR_INVALID_TARGET,
                                "Faction reference is invalid",
                                "target_faction", param.rawValue);
                    }
                }
                break;
            }
            
            case ExtractedParameter::RESOURCE_ENTITY: {
                if (param.entityPtr == nullptr) {
                    addError(result, ValidationError::ERROR, ERR_RESOURCE_NOT_FOUND,
                            "Resource '" + param.rawValue + "' not found",
                            "resource", param.rawValue);
                }
                break;
            }
            
            case ExtractedParameter::QUANTITY: {
                int quantity = 0;
                try {
                    quantity = std::stoi(param.rawValue);
                } catch (...) {
                    addError(result, ValidationError::ERROR, ERR_QUANTITY_OUT_OF_BOUNDS,
                            "Quantity '" + param.rawValue + "' is not a valid number",
                            "quantity", param.rawValue);
                    break;
                }
                
                if (quantity < MIN_QUANTITY || quantity > MAX_QUANTITY) {
                    addError(result, ValidationError::ERROR, ERR_QUANTITY_OUT_OF_BOUNDS,
                            "Quantity must be between " + std::to_string(MIN_QUANTITY) + 
                            " and " + std::to_string(MAX_QUANTITY),
                            "quantity", param.rawValue);
                }
                break;
            }
            
            case ExtractedParameter::TONE: {
                // Tone validation: should be one of known tones
                if (param.rawValue != "positive" && param.rawValue != "neutral" &&
                    param.rawValue != "negative" && param.rawValue != "aggressive" &&
                    param.rawValue != "diplomatic") {
                    addWarning(result, "UNKNOWN_TONE",
                              "Tone '" + param.rawValue + "' is not recognized",
                              "tone");
                }
                break;
            }
            
            case ExtractedParameter::LOCATION:
            case ExtractedParameter::UNKNOWN:
            default:
                // Location and unknown types don't have strict validation
                break;
        }
    }
    
    // Update validation status
    result.isValid = (result.errorCount == 0);
    updateSummary(result);
    
    return result;
}

// ============================================================================
// Individual entity validators
// ============================================================================

ValidationError CommandValidator::validateNPCExists(int npcId, const NPCRegistry& registry) {
    ValidationError error;
    
    if (registry.hasNPC(npcId)) {
        error.severity = ValidationError::INFO;
        error.code = "NPC_VALID";
        error.message = "NPC exists and is valid";
        error.field = "npc_id";
        error.invalidValue = std::to_string(npcId);
    } else {
        error.severity = ValidationError::ERROR;
        error.code = ERR_NPC_NOT_FOUND;
        error.message = "NPC with ID " + std::to_string(npcId) + " not found";
        error.field = "npc_id";
        error.invalidValue = std::to_string(npcId);
        error.suggestions = {"Check NPC ID is correct", "Verify NPC is still in settlement"};
    }
    
    return error;
}

ValidationError CommandValidator::validateFactionExists(int factionId, const FactionRegistry& registry) {
    ValidationError error;
    
    if (registry.hasFaction(factionId)) {
        error.severity = ValidationError::INFO;
        error.code = "FACTION_VALID";
        error.message = "Faction exists and is valid";
        error.field = "faction_id";
        error.invalidValue = std::to_string(factionId);
    } else {
        error.severity = ValidationError::ERROR;
        error.code = ERR_FACTION_NOT_FOUND;
        error.message = "Faction with ID " + std::to_string(factionId) + " not found";
        error.field = "faction_id";
        error.invalidValue = std::to_string(factionId);
        error.suggestions = {"Check faction name spelling", "List all factions with 'factions' command"};
    }
    
    return error;
}

ValidationError CommandValidator::validateResourceAvailable(
    int resourceId,
    int minQuantity,
    const ResourceRegistry& registry
) {
    ValidationError error;
    
    auto resource = registry.getResourceById(resourceId);
    if (!resource) {
        error.severity = ValidationError::ERROR;
        error.code = ERR_RESOURCE_NOT_FOUND;
        error.message = "Resource with ID " + std::to_string(resourceId) + " not found";
        error.field = "resource_id";
        error.invalidValue = std::to_string(resourceId);
        error.suggestions = {"Check resource name", "List all resources with 'resources' command"};
    } else if (resource->getQuantity() < minQuantity) {
        error.severity = ValidationError::ERROR;
        error.code = ERR_INSUFFICIENT_RESOURCES;
        error.message = "Insufficient " + resource->getName() + " available (" + 
                       std::to_string(resource->getQuantity()) + " / " + 
                       std::to_string(minQuantity) + " required)";
        error.field = "resource_quantity";
        error.invalidValue = std::to_string(resource->getQuantity());
        error.suggestions = {"Reduce quantity requested", "Wait for resource production", 
                            "Find alternative resource"};
    } else {
        error.severity = ValidationError::INFO;
        error.code = "RESOURCE_VALID";
        error.message = "Resource available in sufficient quantity";
        error.field = "resource_id";
        error.invalidValue = std::to_string(resourceId);
    }
    
    return error;
}

ValidationError CommandValidator::validateQuantityBounds(
    int quantity,
    const std::string& actionName,
    int minBound,
    int maxBound
) {
    ValidationError error;
    
    if (quantity < minBound || quantity > maxBound) {
        error.severity = ValidationError::ERROR;
        error.code = ERR_QUANTITY_OUT_OF_BOUNDS;
        error.message = "Quantity " + std::to_string(quantity) + " out of bounds [" +
                       std::to_string(minBound) + ", " + std::to_string(maxBound) + "]";
        error.field = "quantity";
        error.invalidValue = std::to_string(quantity);
        error.suggestions = {"Use value between " + std::to_string(minBound) + 
                            " and " + std::to_string(maxBound)};
    } else {
        error.severity = ValidationError::INFO;
        error.code = "QUANTITY_VALID";
        error.message = "Quantity is within acceptable bounds";
        error.field = "quantity";
        error.invalidValue = std::to_string(quantity);
    }
    
    return error;
}

ValidationError CommandValidator::validateNotSelf(int npcId) {
    ValidationError error;
    
    // Player is always ID 0 (future: make configurable)
    if (npcId == 0) {
        error.severity = ValidationError::ERROR;
        error.code = ERR_SELF_TARGET;
        error.message = "Cannot execute action on yourself";
        error.field = "target_npc";
        error.invalidValue = std::to_string(npcId);
        error.suggestions = {"Choose a different NPC as target"};
    } else {
        error.severity = ValidationError::INFO;
        error.code = "TARGET_VALID";
        error.message = "Target is not self";
        error.field = "target_npc";
        error.invalidValue = std::to_string(npcId);
    }
    
    return error;
}

ValidationError CommandValidator::validatePermission(
    const std::string& action,
    const std::string& targetType,
    int targetId
) {
    ValidationError error;
    
    // For now, player always has permission (future: add faction alignment checks)
    error.severity = ValidationError::INFO;
    error.code = "PERMISSION_GRANTED";
    error.message = "Player has permission to execute '" + action + "' on " + targetType;
    error.field = "permission";
    error.invalidValue = action;
    
    return error;
}

// ============================================================================
// Suggestion generation
// ============================================================================

std::vector<std::string> CommandValidator::generateSuggestions(
    const std::string& errorCode,
    const std::string& invalidValue,
    const std::vector<std::string>& candidates
) {
    std::vector<std::string> suggestions;
    
    if (candidates.empty()) {
        return suggestions;
    }
    
    // Return top 2-3 closest matches using simple alphabetic distance
    if (candidates.size() <= 2) {
        suggestions = candidates;
    } else {
        // Simple heuristic: return first 3 candidates
        suggestions.assign(candidates.begin(), 
                          candidates.begin() + std::min(size_t(3), candidates.size()));
    }
    
    return suggestions;
}

// ============================================================================
// Error message formatting
// ============================================================================

std::string CommandValidator::formatErrorMessage(const ValidationError& error) {
    std::ostringstream oss;
    
    oss << error.message;
    
    if (!error.field.empty()) {
        oss << " [" << error.field << "]";
    }
    
    if (!error.suggestions.empty()) {
        oss << " Suggestions: ";
        for (size_t i = 0; i < error.suggestions.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << error.suggestions[i];
        }
    }
    
    return oss.str();
}

// ============================================================================
// Dry-run validation
// ============================================================================

ValidationResult CommandValidator::dryRun(
    const std::string& action,
    const ExtractedParameters& parameters,
    const NPCRegistry& npcRegistry,
    const FactionRegistry& factionRegistry,
    const ResourceRegistry& resourceRegistry
) {
    // Same as validateCommand - no side effects in either case
    return validateCommand(action, parameters, npcRegistry, factionRegistry, resourceRegistry);
}

// ============================================================================
// Helper methods
// ============================================================================

void CommandValidator::addError(
    ValidationResult& result,
    ValidationError::Severity severity,
    const std::string& code,
    const std::string& message,
    const std::string& field,
    const std::string& invalidValue
) {
    ValidationError error;
    error.severity = severity;
    error.code = code;
    error.message = message;
    error.field = field;
    error.invalidValue = invalidValue;
    
    result.errors.push_back(error);
    result.errorCount++;
    
    if (severity != ValidationError::INFO) {
        result.isValid = false;
    }
}

void CommandValidator::addWarning(
    ValidationResult& result,
    const std::string& code,
    const std::string& message,
    const std::string& field
) {
    ValidationError warning;
    warning.severity = ValidationError::WARNING;
    warning.code = code;
    warning.message = message;
    warning.field = field;
    warning.invalidValue = "";
    
    result.warnings.push_back(warning);
    result.warningCount++;
    result.hasWarnings = true;
}

void CommandValidator::updateSummary(ValidationResult& result) {
    std::ostringstream oss;
    
    if (result.errorCount == 0 && result.warningCount == 0) {
        oss << "✓ Validation passed: Command is ready to execute";
    } else if (result.errorCount > 0) {
        oss << "✗ Validation failed: " << result.errorCount << " error(s) found";
    } else if (result.warningCount > 0) {
        oss << "⚠ Validation passed with " << result.warningCount << " warning(s)";
    }
    
    result.summary = oss.str();
}

}  // namespace TLS
