#include "ParameterExtractor.h"
#include "Registries.h"
#include "FuzzyParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstdlib>

namespace TLS {

// Helper: convert string to lowercase
static std::string toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Helper: check if all characters are uppercase
static bool isAllUppercase(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (std::isalpha(c) && std::islower(c)) {
            return false;
        }
    }
    return true;
}

// Helper: check if string starts with uppercase (proper noun)
static bool isProperNoun(const std::string& str) {
    if (str.empty()) return false;
    return std::isupper(str[0]);
}

bool ParameterExtractor::isNumeric(const std::string& str) {
    if (str.empty()) return false;
    
    // Handle hex (0x prefix)
    if (str.length() > 2 && str.substr(0, 2) == "0x") {
        return true;
    }
    
    // Handle scientific notation (e.g., "5e1")
    bool hasE = false;
    bool hasDot = false;
    bool hasDigit = false;
    
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        
        if (std::isdigit(c)) {
            hasDigit = true;
        } else if ((c == 'e' || c == 'E') && !hasE && i > 0 && i < str.length() - 1) {
            hasE = true;
        } else if (c == '.' && !hasDot) {
            hasDot = true;
        } else if ((c == '+' || c == '-') && (i == 0 || str[i-1] == 'e' || str[i-1] == 'E')) {
            // Sign at start or after 'e'
            continue;
        } else {
            return false;
        }
    }
    
    return hasDigit;
}

int ParameterExtractor::parseQuantity(const std::string& quantityStr) {
    if (quantityStr.empty()) return -1;
    
    std::string clean = quantityStr;
    
    // Remove whitespace
    clean.erase(std::remove_if(clean.begin(), clean.end(), 
                               [](unsigned char c) { return std::isspace(c); }),
                clean.end());
    
    // Try parsing as hex
    if (clean.length() > 2 && clean.substr(0, 2) == "0x") {
        try {
            return std::stoi(clean, nullptr, 16);
        } catch (...) {
            return -1;
        }
    }
    
    // Extract leading digits
    std::string numPart;
    for (char c : clean) {
        if (std::isdigit(c) || c == '-' || c == '+') {
            numPart += c;
        } else {
            break;  // Stop at first non-numeric character
        }
    }
    
    if (numPart.empty()) return -1;
    
    try {
        return std::stoi(numPart);
    } catch (...) {
        return -1;
    }
}

ExtractedParameter::Type ParameterExtractor::classifyParameter(
    const std::string& rawParam,
    const std::string& actionName
) {
    std::string lower = toLowercase(rawParam);
    
    // Check for numeric
    if (isNumeric(rawParam)) {
        return ExtractedParameter::QUANTITY;
    }
    
    // Check for tone keywords
    std::vector<std::string> positiveKeywords = {"please", "thank", "thanks", "kindly", "pls"};
    std::vector<std::string> aggressiveKeywords = {"now", "immediately", "urgent", "asap", "immediately"};
    std::vector<std::string> negativeKeywords = {"must", "demand", "refuse", "refuse", "no", "not", "never"};
    
    for (const auto& kw : positiveKeywords) {
        if (lower.find(kw) != std::string::npos) {
            return ExtractedParameter::TONE;
        }
    }
    for (const auto& kw : aggressiveKeywords) {
        if (lower.find(kw) != std::string::npos) {
            return ExtractedParameter::TONE;
        }
    }
    for (const auto& kw : negativeKeywords) {
        if (lower.find(kw) != std::string::npos) {
            return ExtractedParameter::TONE;
        }
    }
    
    // Check for faction (all uppercase convention, common faction names)
    if (isAllUppercase(rawParam) && rawParam.length() > 1) {
        return ExtractedParameter::FACTION_ENTITY;
    }
    
    // Check for NPC (proper noun - starts with uppercase)
    if (isProperNoun(rawParam) && rawParam.length() > 1) {
        return ExtractedParameter::NPC_ENTITY;
    }
    
    // Default: treat as resource (lowercase nouns)
    return ExtractedParameter::RESOURCE_ENTITY;
}

std::pair<std::string, float> ParameterExtractor::fuzzyMatchName(
    const std::string& inputName,
    const std::vector<std::string>& registryNames,
    float confidenceThreshold
) {
    if (registryNames.empty()) {
        return {"", 0.0f};
    }
    
    float bestConfidence = 0.0f;
    std::string bestMatch;
    
    std::string inputLower = toLowercase(inputName);
    
    for (const auto& registryName : registryNames) {
        std::string regLower = toLowercase(registryName);
        
        // Exact match
        if (inputLower == regLower) {
            return {registryName, 1.0f};
        }
        
        // Levenshtein distance
        int distance = FuzzyParser::levenshteinDistance(inputLower, regLower, 3);
        float confidence = std::max(0.0f, 1.0f - (static_cast<float>(distance) / 3.0f));
        
        // Substring match boost
        if (inputLower.find(regLower) != std::string::npos ||
            regLower.find(inputLower) != std::string::npos) {
            confidence = std::max(confidence, 0.9f);
        }
        
        if (confidence > bestConfidence) {
            bestConfidence = confidence;
            bestMatch = registryName;
        }
    }
    
    if (bestConfidence >= confidenceThreshold) {
        return {bestMatch, bestConfidence};
    }
    
    return {"", bestConfidence};
}

std::string ParameterExtractor::extractTone(const std::vector<std::string>& params) {
    int positiveScore = 0;
    int negativeScore = 0;
    int aggressiveScore = 0;
    
    std::vector<std::string> positiveKeywords = {"please", "thank", "thanks", "kindly", "pls", "would", "could"};
    std::vector<std::string> negativeKeywords = {"must", "demand", "refuse", "no", "never", "not"};
    std::vector<std::string> aggressiveKeywords = {"now", "immediately", "urgent", "asap", "command", "force"};
    
    for (const auto& param : params) {
        std::string lower = toLowercase(param);
        
        for (const auto& kw : positiveKeywords) {
            if (lower.find(kw) != std::string::npos) {
                positiveScore++;
            }
        }
        for (const auto& kw : negativeKeywords) {
            if (lower.find(kw) != std::string::npos) {
                negativeScore++;
            }
        }
        for (const auto& kw : aggressiveKeywords) {
            if (lower.find(kw) != std::string::npos) {
                aggressiveScore++;
            }
        }
    }
    
    if (aggressiveScore > positiveScore && aggressiveScore > negativeScore) {
        return "aggressive";
    } else if (negativeScore > positiveScore) {
        return "negative";
    } else if (positiveScore > 0) {
        return "positive";
    }
    
    return "neutral";
}

ExtractedParameter ParameterExtractor::extractQuantity(const std::string& quantityStr) {
    ExtractedParameter result;
    result.type = ExtractedParameter::QUANTITY;
    result.rawValue = quantityStr;
    result.entityPtr = nullptr;
    
    int qty = parseQuantity(quantityStr);
    
    if (qty < 0) {
        result.isValid = false;
        result.validationError = "Could not parse quantity: " + quantityStr;
        result.confidence = 0.0f;
        return result;
    }
    
    result.resolvedName = std::to_string(qty);
    result.confidence = 1.0f;
    result.isValid = true;
    
    // Validate bounds (0-1,000,000)
    if (qty < 0 || qty > 1000000) {
        result.isValid = false;
        result.validationError = "Quantity out of bounds: " + std::to_string(qty);
        result.confidence = 0.5f;
    }
    
    return result;
}

ExtractedParameter ParameterExtractor::extractNPC(
    const std::string& npcName,
    NPCRegistry& registry,
    float confidenceThreshold
) {
    ExtractedParameter result;
    result.type = ExtractedParameter::NPC_ENTITY;
    result.rawValue = npcName;
    result.entityPtr = nullptr;
    
    // Get all NPC names from registry
    const auto& allNPCs = registry.getAllNPCs();
    std::vector<std::string> npcNames;
    
    for (const auto& npc : allNPCs) {
        if (npc) {
            // Assume NPC has a name attribute - adjust if interface differs
            // For now, we'll construct a reasonable name
            npcNames.push_back(npc->getName());  // Use getter method
        }
    }
    
    // Fuzzy match
    auto [matchedName, confidence] = fuzzyMatchName(npcName, npcNames, confidenceThreshold);
    
    result.confidence = confidence;
    result.resolvedName = matchedName;
    
    if (confidence < confidenceThreshold) {
        result.isValid = false;
        result.validationError = "NPC not found: " + npcName + " (confidence: " + 
                                std::to_string(confidence).substr(0, 4) + ")";
        return result;
    }
    
    // Find and return the NPC pointer
    for (const auto& npc : allNPCs) {
        if (npc && npc->getName() == matchedName) {
            result.entityPtr = reinterpret_cast<void*>(npc.get());
            result.isValid = true;
            return result;
        }
    }
    
    result.isValid = false;
    result.validationError = "NPC lookup failed for: " + matchedName;
    return result;
}

ExtractedParameter ParameterExtractor::extractFaction(
    const std::string& factionName,
    FactionRegistry& registry,
    float confidenceThreshold
) {
    ExtractedParameter result;
    result.type = ExtractedParameter::FACTION_ENTITY;
    result.rawValue = factionName;
    result.entityPtr = nullptr;
    
    // Get all faction names
    const auto& allFactions = registry.getAllFactions();
    std::vector<std::string> factionNames;
    
    for (const auto& faction : allFactions) {
        if (faction) {
            factionNames.push_back(faction->getName());
        }
    }
    
    // Fuzzy match
    auto [matchedName, confidence] = fuzzyMatchName(factionName, factionNames, confidenceThreshold);
    
    result.confidence = confidence;
    result.resolvedName = matchedName;
    
    if (confidence < confidenceThreshold) {
        result.isValid = false;
        result.validationError = "Faction not found: " + factionName + " (confidence: " +
                                std::to_string(confidence).substr(0, 4) + ")";
        return result;
    }
    
    // Find and return the Faction pointer
    for (const auto& faction : allFactions) {
        if (faction && faction->getName() == matchedName) {
            result.entityPtr = reinterpret_cast<void*>(faction.get());
            result.isValid = true;
            return result;
        }
    }
    
    result.isValid = false;
    result.validationError = "Faction lookup failed for: " + matchedName;
    return result;
}

ExtractedParameter ParameterExtractor::extractResource(
    const std::string& resourceName,
    ResourceRegistry& registry,
    float confidenceThreshold
) {
    ExtractedParameter result;
    result.type = ExtractedParameter::RESOURCE_ENTITY;
    result.rawValue = resourceName;
    result.entityPtr = nullptr;
    
    // Get all resource names
    const auto& allResources = registry.getAllResources();
    std::vector<std::string> resourceNames;
    
    for (const auto& resource : allResources) {
        if (resource) {
            resourceNames.push_back(resource->getName());
        }
    }
    
    // Fuzzy match
    auto [matchedName, confidence] = fuzzyMatchName(resourceName, resourceNames, confidenceThreshold);
    
    result.confidence = confidence;
    result.resolvedName = matchedName;
    
    if (confidence < confidenceThreshold) {
        result.isValid = false;
        result.validationError = "Resource not found: " + resourceName + " (confidence: " +
                                std::to_string(confidence).substr(0, 4) + ")";
        return result;
    }
    
    // Find and return the Resource pointer
    for (const auto& resource : allResources) {
        if (resource && resource->getName() == matchedName) {
            result.entityPtr = reinterpret_cast<void*>(resource.get());
            result.isValid = true;
            return result;
        }
    }
    
    result.isValid = false;
    result.validationError = "Resource lookup failed for: " + matchedName;
    return result;
}

std::vector<ExtractedParameter> ParameterExtractor::extractAllNPCs(
    const std::vector<std::string>& rawParams,
    NPCRegistry& registry,
    float confidenceThreshold
) {
    std::vector<ExtractedParameter> results;
    
    for (const auto& param : rawParams) {
        auto paramType = classifyParameter(param, "");
        if (paramType == ExtractedParameter::NPC_ENTITY) {
            results.push_back(extractNPC(param, registry, confidenceThreshold));
        }
    }
    
    return results;
}

std::vector<ExtractedParameter> ParameterExtractor::extractAllFactions(
    const std::vector<std::string>& rawParams,
    FactionRegistry& registry,
    float confidenceThreshold
) {
    std::vector<ExtractedParameter> results;
    
    for (const auto& param : rawParams) {
        auto paramType = classifyParameter(param, "");
        if (paramType == ExtractedParameter::FACTION_ENTITY) {
            results.push_back(extractFaction(param, registry, confidenceThreshold));
        }
    }
    
    return results;
}

std::vector<ExtractedParameter> ParameterExtractor::extractAllResources(
    const std::vector<std::string>& rawParams,
    ResourceRegistry& registry,
    float confidenceThreshold
) {
    std::vector<ExtractedParameter> results;
    
    for (const auto& param : rawParams) {
        auto paramType = classifyParameter(param, "");
        if (paramType == ExtractedParameter::RESOURCE_ENTITY) {
            results.push_back(extractResource(param, registry, confidenceThreshold));
        }
    }
    
    return results;
}

ExtractedParameters ParameterExtractor::extract(
    const std::string& actionName,
    const std::vector<std::string>& rawParams,
    NPCRegistry& npcRegistry,
    FactionRegistry& factionRegistry,
    ResourceRegistry& resourceRegistry
) {
    ExtractedParameters result;
    result.validCount = 0;
    result.invalidCount = 0;
    result.confidenceScore = 0.0f;
    
    if (rawParams.empty()) {
        result.extractionReasoning = "No parameters provided";
        result.tone = "neutral";
        return result;
    }
    
    // Extract tone first (from all params)
    result.tone = extractTone(rawParams);
    
    // Classify and extract each parameter
    float totalConfidence = 0.0f;
    
    for (const auto& rawParam : rawParams) {
        auto paramType = classifyParameter(rawParam, actionName);
        ExtractedParameter extracted;
        
        switch (paramType) {
            case ExtractedParameter::NPC_ENTITY:
                extracted = extractNPC(rawParam, npcRegistry, 0.6f);
                break;
            case ExtractedParameter::FACTION_ENTITY:
                extracted = extractFaction(rawParam, factionRegistry, 0.6f);
                break;
            case ExtractedParameter::RESOURCE_ENTITY:
                extracted = extractResource(rawParam, resourceRegistry, 0.6f);
                break;
            case ExtractedParameter::QUANTITY:
                extracted = extractQuantity(rawParam);
                break;
            case ExtractedParameter::TONE:
                // Already processed above
                continue;
            default:
                extracted.type = ExtractedParameter::UNKNOWN;
                extracted.rawValue = rawParam;
                extracted.isValid = false;
                extracted.validationError = "Could not classify parameter";
                extracted.confidence = 0.0f;
                break;
        }
        
        result.parameters.push_back(extracted);
        totalConfidence += extracted.confidence;
        
        if (extracted.isValid) {
            result.validCount++;
        } else {
            result.invalidCount++;
        }
    }
    
    // Calculate average confidence
    if (!rawParams.empty()) {
        result.confidenceScore = totalConfidence / static_cast<float>(rawParams.size());
    }
    
    // Build reasoning string
    result.extractionReasoning = "Extracted " + std::to_string(result.validCount) + 
                                " valid, " + std::to_string(result.invalidCount) + 
                                " invalid parameters. Average confidence: " +
                                std::to_string(result.confidenceScore).substr(0, 4);
    
    return result;
}

}  // namespace TLS
