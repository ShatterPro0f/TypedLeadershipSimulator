#include "DecisionInterpreter.h"
#include "Registries.h"
#include "LLMProvider.h"
#include "Core.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iostream>
#include <cctype>
#include <numeric>
#include <regex>

namespace TLS {

// ==================== UTILITY FUNCTIONS ====================

static std::string toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

static std::string trimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

static std::vector<std::string> tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        token.erase(std::remove_if(token.begin(), token.end(),
            [](char c) { return std::ispunct(c); }), token.end());
        if (!token.empty()) {
            tokens.push_back(toLowercase(token));
        }
    }
    return tokens;
}

// Get registries
NPCRegistry& getNPCRegistry() {
    return NPCRegistry::getInstance();
}

FactionRegistry& getFactionRegistry() {
    return FactionRegistry::getInstance();
}

ResourceRegistry& getResourceRegistry() {
    return ResourceRegistry::getInstance();
}

// ==================== INPUT PARSER ====================

const std::map<std::string, ActionType> ACTION_ALIASES = {
    {"allocate", ActionType::ALLOCATE},
    {"give", ActionType::ALLOCATE},
    {"distribute", ActionType::ALLOCATE},
    {"provide", ActionType::ALLOCATE},
    {"feed", ActionType::ALLOCATE},
    {"assign", ActionType::ALLOCATE},
    
    {"delegate", ActionType::DELEGATE},
    {"assign_task", ActionType::DELEGATE},
    {"task", ActionType::DELEGATE},
    {"order", ActionType::DELEGATE},
    {"command", ActionType::COMMAND},
    
    {"negotiate", ActionType::NEGOTIATE},
    {"discuss", ActionType::NEGOTIATE},
    {"talk", ActionType::NEGOTIATE},
    {"persuade", ActionType::NEGOTIATE},
    {"reason", ActionType::NEGOTIATE},
    
    {"inspire", ActionType::INSPIRE},
    {"encourage", ActionType::INSPIRE},
    {"motivate", ActionType::INSPIRE},
    {"praise", ActionType::INSPIRE},
    {"support", ActionType::INSPIRE},
    
    {"suppress", ActionType::SUPPRESS},
    {"punish", ActionType::SUPPRESS},
    {"discipline", ActionType::SUPPRESS},
    {"force", ActionType::SUPPRESS},
    {"demand", ActionType::SUPPRESS}
};

std::string InputParser::normalizeInput(const std::string& input) {
    std::string normalized = toLowercase(input);
    normalized = trimWhitespace(normalized);
    return normalized;
}

float InputParser::calculateLevenshteinDistance(const std::string& a, const std::string& b) {
    size_t lenA = a.length();
    size_t lenB = b.length();
    
    if (lenA == 0) return (float)lenB;
    if (lenB == 0) return (float)lenA;
    
    std::vector<std::vector<size_t>> dp(lenA + 1, std::vector<size_t>(lenB + 1));
    
    for (size_t i = 0; i <= lenA; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= lenB; ++j) dp[0][j] = j;
    
    for (size_t i = 1; i <= lenA; ++i) {
        for (size_t j = 1; j <= lenB; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    
    return (float)dp[lenA][lenB];
}

float InputParser::calculateConfidenceScore(
    float levenshteinScore,
    float exactMatchScore,
    float semanticScore
) {
    // Hybrid model: 40% Levenshtein, 30% semantic, 30% exact
    float conf_ld = std::max(0.0f, 1.0f - (levenshteinScore / 3.0f));
    float conf_semantic = semanticScore;
    float conf_exact = exactMatchScore;
    
    float confidence = 0.4f * conf_ld + 0.3f * conf_semantic + 0.3f * conf_exact;
    return std::max(0.0f, std::min(1.0f, confidence));
}

ActionType InputParser::extractActionType(const std::string& normalizedInput) {
    std::vector<std::string> tokens = tokenize(normalizedInput);
    
    // Try exact matches first
    for (const auto& token : tokens) {
        auto it = ACTION_ALIASES.find(token);
        if (it != ACTION_ALIASES.end()) {
            return it->second;
        }
    }
    
    // Try fuzzy matches
    float bestDistance = 3.0f;
    ActionType bestAction = ActionType::UNKNOWN;
    
    for (const auto& token : tokens) {
        for (const auto& [alias, action] : ACTION_ALIASES) {
            float dist = calculateLevenshteinDistance(token, alias);
            if (dist < bestDistance) {
                bestDistance = dist;
                bestAction = action;
            }
        }
    }
    
    return bestAction;
}

std::string InputParser::findClosestMatch(
    const std::string& input,
    const std::vector<std::string>& candidates,
    float& bestDistance
) {
    bestDistance = std::numeric_limits<float>::max();
    std::string bestMatch;
    
    std::string normalizedInput = normalizeInput(input);
    
    for (const auto& candidate : candidates) {
        std::string normalizedCandidate = normalizeInput(candidate);
        float dist = calculateLevenshteinDistance(normalizedInput, normalizedCandidate);
        
        if (dist < bestDistance) {
            bestDistance = dist;
            bestMatch = candidate;
        }
    }
    
    return bestMatch;
}

ParseResult InputParser::parsePlayerInput(const std::string& input) {
    ParseResult result;
    
    if (input.empty()) {
        result.isValid = false;
        result.errors.push_back("Input cannot be empty");
        return result;
    }
    
    std::string normalized = normalizeInput(input);
    result.actionType = extractActionType(normalized);
    
    if (result.actionType == ActionType::UNKNOWN) {
        result.isValid = false;
        result.errors.push_back("Unknown action: " + input);
        result.suggestions = {"Try: allocate, delegate, negotiate, inspire, suppress"};
        return result;
    }
    
    result.isValid = true;
    result.confidence = 0.9f;
    result.parameters = ParameterExtractor::extractParameters(input, result.actionType);
    
    return result;
}

ParseResult InputParser::parsePlayerInputWithContext(
    const std::string& input,
    const std::vector<std::string>& contextNPCs,
    const std::vector<std::string>& contextFactions,
    const std::vector<std::string>& contextResources
) {
    ParseResult result = parsePlayerInput(input);
    
    // Enhance with context-aware matching if available
    if (!contextNPCs.empty() || !contextFactions.empty() || !contextResources.empty()) {
        // Additional logic could be added here to boost confidence
        // based on whether NPCs/factions/resources mentioned in input are in context
    }
    
    return result;
}

// ==================== PARAMETER EXTRACTOR ====================

std::vector<ExtractedParameter> ParameterExtractor::extractParameters(
    const std::string& input,
    ActionType /* actionType */
) {
    std::vector<ExtractedParameter> params;
    
    // Try to extract each parameter type
    int qty = extractQuantity(input);
    if (qty > 0) {
        params.emplace_back(ParameterType::QUANTITY, std::to_string(qty), 0.8f);
    }
    
    // Try to extract NPC/Faction name
    std::string namePattern = "[a-zA-Z]+";
    std::regex nameRegex(namePattern);
    std::smatch match;
    
    if (std::regex_search(input, match, nameRegex)) {
        std::string name = match.str();
        params.emplace_back(ParameterType::NPC_OR_FACTION, name, 0.7f);
    }
    
    return params;
}

std::vector<ExtractedParameter> ParameterExtractor::extractParametersWithWorldState(
    const std::string& input,
    ActionType actionType
) {
    std::vector<ExtractedParameter> params = extractParameters(input, actionType);
    
    // Resolve IDs from world state
    for (auto& param : params) {
        if (param.type == ParameterType::NPC_OR_FACTION) {
            // Try NPC lookup
            auto npc = NPCRegistry::getInstance().getNPCById(std::stoi(param.value));
            if (npc) {
                param.id = npc->getId();
                param.confidence = 0.95f;
            }
        }
    }
    
    return params;
}

int ParameterExtractor::extractQuantity(const std::string& input) {
    std::regex numRegex("\\b(\\d+)\\b");
    std::smatch match;
    
    if (std::regex_search(input, match, numRegex)) {
        return std::stoi(match[1].str());
    }
    
    return 0;
}

int ParameterExtractor::extractNPCId(const std::string& input) {
    auto& registry = NPCRegistry::getInstance();
    auto allNPCs = registry.getAllNPCs();
    
    for (const auto& npc : allNPCs) {
        if (input.find(npc->getName()) != std::string::npos) {
            return npc->getId();
        }
    }
    
    return -1;
}

int ParameterExtractor::extractFactionId(const std::string& input) {
    auto& registry = FactionRegistry::getInstance();
    auto allFactions = registry.getAllFactions();
    
    for (const auto& faction : allFactions) {
        if (input.find(faction->getName()) != std::string::npos) {
            return faction->getId();
        }
    }
    
    return -1;
}

int ParameterExtractor::extractResourceId(const std::string& input) {
    auto& registry = ResourceRegistry::getInstance();
    auto allResources = registry.getAllResources();
    
    for (const auto& resource : allResources) {
        if (input.find(resource->getName()) != std::string::npos) {
            return resource->getId();
        }
    }
    
    return -1;
}

TargetType ParameterExtractor::detectTargetType(const std::vector<ExtractedParameter>& params) {
    for (const auto& param : params) {
        if (param.type == ParameterType::NPC_OR_FACTION) {
            if (ParameterExtractor::extractNPCId(param.value) >= 0) {
                return TargetType::NPC;
            }
            if (ParameterExtractor::extractFactionId(param.value) >= 0) {
                return TargetType::FACTION;
            }
        }
    }
    
    return TargetType::UNKNOWN_TARGET;
}

// ==================== TONE DETECTOR ====================

const std::vector<std::string> ToneDetector::POSITIVE_KEYWORDS = {
    "please", "help", "support", "thank", "improve", "increase",
    "reward", "praise", "encourage", "cooperate", "kindly",
    "assist", "aid", "strengthen", "promote", "develop",
    "enhance", "elevate", "boost", "uplift", "facilitate",
    "grateful", "appreciate", "honor", "celebrate"
};

const std::vector<std::string> ToneDetector::NEGATIVE_KEYWORDS = {
    "force", "demand", "hate", "punish", "suppress", "refuse",
    "destroy", "eliminate", "reduce", "decrease", "deny",
    "immediately", "now", "urgent", "pressure", "coerce",
    "harsh", "cruel", "brutal", "aggressive", "violent",
    "despise", "infuriate", "disgust", "outrage"
};

float ToneDetector::scoreTone(
    const std::string& input,
    const std::vector<std::string>& keywords
) {
    std::vector<std::string> tokens = tokenize(input);
    float score = 0.0f;
    
    for (const auto& token : tokens) {
        for (const auto& keyword : keywords) {
            if (token == keyword || token.find(keyword) != std::string::npos) {
                score += 1.0f;
            }
        }
    }
    
    return std::min(1.0f, score / static_cast<float>(tokens.size() + 1));
}

ToneResult ToneDetector::detectToneRuleBased(const std::string& input) {
    float positiveScore = scoreTone(input, POSITIVE_KEYWORDS);
    float negativeScore = scoreTone(input, NEGATIVE_KEYWORDS);
    
    // Check for emphasis markers
    if (input.find('!') != std::string::npos) {
        if (positiveScore > negativeScore) {
            positiveScore += 0.2f;
        } else {
            negativeScore += 0.2f;
        }
    }
    
    if (input.find('?') != std::string::npos) {
        positiveScore += 0.1f;  // Questions tend toward curiosity/seeking help
    }
    
    // Check for ALL CAPS
    int capsCount = 0;
    for (char c : input) {
        if (std::isupper(c)) capsCount++;
    }
    if (capsCount > static_cast<int>(input.length() / 2)) {
        negativeScore += 0.3f;  // ALL CAPS = aggressive/angry
    }
    
    ToneResult result;
    result.confidence = std::max(positiveScore, negativeScore);
    
    if (positiveScore > negativeScore + 0.1f) {
        result.tone = ToneType::POSITIVE;
        result.reasoning = "Detected positive sentiment from keywords";
    } else if (negativeScore > positiveScore + 0.1f) {
        result.tone = ToneType::NEGATIVE;
        result.reasoning = "Detected negative sentiment from keywords";
    } else {
        result.tone = ToneType::NEUTRAL;
        result.reasoning = "Neutral tone detected";
    }
    
    result.usedLLM = false;
    return result;
}

ToneResult ToneDetector::detectTone(
    const std::string& input,
    LLMProvider* llmProvider,
    bool allowFallback
) {
    // Try LLM first if available
    if (llmProvider && llmProvider->isAvailable()) {
        std::string prompt = "Classify the tone of this message as POSITIVE, NEUTRAL, or NEGATIVE:\n\"" + input + "\"";
        
        LLMResponse response = llmProvider->callLLM(prompt);
        if (response.wasSuccessful) {
            ToneResult result;
            
            std::string responseText = toLowercase(response.text);
            if (responseText.find("positive") != std::string::npos) {
                result.tone = ToneType::POSITIVE;
                result.reasoning = "LLM detected positive tone";
            } else if (responseText.find("negative") != std::string::npos) {
                result.tone = ToneType::NEGATIVE;
                result.reasoning = "LLM detected negative tone";
            } else {
                result.tone = ToneType::NEUTRAL;
                result.reasoning = "LLM detected neutral tone";
            }
            
            result.confidence = 0.95f;
            result.usedLLM = true;
            return result;
        }
    }
    
    // Fallback to rule-based detection
    if (allowFallback) {
        return detectToneRuleBased(input);
    }
    
    return ToneResult(ToneType::NEUTRAL, 0.5f, "Fallback disabled", false);
}

ToneResult ToneDetector::detectToneWithContext(
    const std::string& input,
    int /* targetNPCId */,
    int /* targetFactionId */,
    LLMProvider* llmProvider
) {
    ToneResult result = detectTone(input, llmProvider, true);
    
    // Could enhance tone based on NPC/faction relationships
    // For now, return the base tone detection
    
    return result;
}

// ==================== DECISION VALIDATOR ====================

ValidationResult DecisionValidator::validate(const Decision& decision) {
    ValidationResult result;
    
    if (decision.actionType == ActionType::UNKNOWN) {
        result.isValid = false;
        result.errors.push_back("Invalid action type");
        return result;
    }
    
    // Basic validation passed
    result.isValid = true;
    result.targetType = decision.targetType;
    
    return result;
}

ValidationResult DecisionValidator::validateWithWorldState(const Decision& decision) {
    ValidationResult result = validate(decision);
    
    if (!result.isValid) return result;
    
    // Validate targets exist in world state
    if (decision.targetNPCId >= 0) {
        if (!validateNPCExists(decision.targetNPCId)) {
            result.isValid = false;
            result.errors.push_back("Target NPC does not exist");
            return result;
        }
    }
    
    if (decision.targetFactionId >= 0) {
        if (!validateFactionExists(decision.targetFactionId)) {
            result.isValid = false;
            result.errors.push_back("Target faction does not exist");
            return result;
        }
    }
    
    if (decision.targetResourceId >= 0) {
        if (!validateResourceExists(decision.targetResourceId)) {
            result.isValid = false;
            result.errors.push_back("Target resource does not exist");
            return result;
        }
    }
    
    // Validate quantity
    if (decision.actionType == ActionType::ALLOCATE && decision.targetQuantity > 0) {
        auto resource = ResourceRegistry::getInstance().getResourceById(decision.targetResourceId);
        if (resource && !validateQuantity(decision.targetQuantity, resource->getQuantity())) {
            result.isValid = false;
            result.errors.push_back("Insufficient resources available");
            return result;
        }
    }
    
    return result;
}

bool DecisionValidator::validateNPCExists(int npcId) {
    return NPCRegistry::getInstance().hasNPC(npcId);
}

bool DecisionValidator::validateFactionExists(int factionId) {
    return FactionRegistry::getInstance().hasFaction(factionId);
}

bool DecisionValidator::validateResourceExists(int resourceId) {
    return ResourceRegistry::getInstance().hasResource(resourceId);
}

bool DecisionValidator::validateQuantity(int quantity, int available) {
    return quantity > 0 && quantity <= available;
}

// ==================== DECISION EXECUTOR ====================

float DecisionExecutor::getToneMultiplier(ToneType tone) {
    switch (tone) {
        case ToneType::POSITIVE:
            return 1.2f;  // 20% bonus
        case ToneType::NEGATIVE:
            return 0.8f;  // 20% penalty
        case ToneType::NEUTRAL:
        default:
            return 1.0f;
    }
}

void DecisionExecutor::executeDecision(const Decision& decision) {
    // Validate first
    ValidationResult validation = DecisionValidator::validateWithWorldState(const_cast<Decision&>(decision));
    if (!validation.isValid) {
        std::cerr << "Decision validation failed" << std::endl;
        return;
    }
    
    // Apply consequences based on action type
    std::vector<SimulationImpact> impacts;
    
    switch (decision.actionType) {
        case ActionType::ALLOCATE: {
            if (decision.targetResourceId >= 0) {
                applyResourceConsequences(decision.targetResourceId, -decision.targetQuantity);
            }
            
            if (decision.targetNPCId >= 0) {
                auto impact = applyLoyaltyDeltaEquation(
                    decision.targetNPCId,
                    decision.actionType,
                    decision.toneMultiplier,
                    decision.targetQuantity
                );
                impacts.push_back(impact);
            }
            break;
        }
        
        case ActionType::INSPIRE: {
            if (decision.targetNPCId >= 0) {
                auto impact = applyLoyaltyDeltaEquation(
                    decision.targetNPCId,
                    decision.actionType,
                    decision.toneMultiplier
                );
                impacts.push_back(impact);
            }
            break;
        }
        
        case ActionType::SUPPRESS: {
            if (decision.targetNPCId >= 0) {
                auto impact = applyLoyaltyDeltaEquation(
                    decision.targetNPCId,
                    decision.actionType,
                    decision.toneMultiplier
                );
                impacts.push_back(impact);
            }
            break;
        }
        
        default:
            // Handle other actions
            break;
    }
    
    // Apply impacts to world state
    applyNPCConsequences(impacts);
    
    // Process cascading effects
    processCascades(decision, impacts);
}

SimulationImpact DecisionExecutor::applyLoyaltyDeltaEquation(
    int npcId,
    ActionType action,
    float toneMultiplier,
    int quantity
) {
    SimulationImpact impact;
    impact.affectedNPCId = npcId;
    
    // From Phase 2 equations (simplified)
    float baseLoyaltyDelta = 0.0f;
    
    switch (action) {
        case ActionType::ALLOCATE:
            baseLoyaltyDelta = 0.05f * (quantity / 100.0f);  // Scale with quantity
            break;
        case ActionType::INSPIRE:
            baseLoyaltyDelta = 0.1f;
            break;
        case ActionType::SUPPRESS:
            baseLoyaltyDelta = -0.15f;
            break;
        case ActionType::NEGOTIATE:
            baseLoyaltyDelta = 0.08f;
            break;
        default:
            baseLoyaltyDelta = 0.0f;
    }
    
    impact.loyaltyDelta = baseLoyaltyDelta * toneMultiplier;
    impact.moodMultiplier = toneMultiplier;
    impact.impactDescription = "Loyalty changed by " + std::to_string(impact.loyaltyDelta);
    
    return impact;
}

SimulationImpact DecisionExecutor::applyMoodDeltaEquation(
    int npcId,
    float loyaltyDelta,
    ToneType tone
) {
    SimulationImpact impact;
    impact.affectedNPCId = npcId;
    
    // Mood shifts based on loyalty change and tone
    float moodDelta = loyaltyDelta * 0.5f;  // Mood slightly lags loyalty
    
    if (tone == ToneType::NEGATIVE && loyaltyDelta < 0) {
        moodDelta *= 1.5f;  // Amplify negative effects on mood
    }
    
    impact.moodDelta = moodDelta;
    impact.impactDescription = "Mood changed by " + std::to_string(impact.moodDelta);
    
    return impact;
}

void DecisionExecutor::applyNPCConsequences(const std::vector<SimulationImpact>& impacts) {
    for (const auto& impact : impacts) {
        if (impact.affectedNPCId >= 0) {
            auto npc = NPCRegistry::getInstance().getNPCById(impact.affectedNPCId);
            if (npc) {
                // Update loyalty
                if (impact.loyaltyDelta != 0) {
                    float newLoyalty = npc->getLoyalty() + impact.loyaltyDelta;
                    npc->setLoyalty(std::max(0.0f, std::min(1.0f, newLoyalty)));
                }
                
                // Update mood
                if (impact.moodDelta != 0) {
                    float newMood = npc->getShortTermMood() + impact.moodDelta;
                    npc->setShortTermMood(std::max(0.0f, std::min(1.0f, newMood)));
                }
            }
        }
    }
}

void DecisionExecutor::applyFactionConsequences(int factionId, const SimulationImpact& impact) {
    if (factionId >= 0) {
        auto faction = FactionRegistry::getInstance().getFactionById(factionId);
        if (faction && impact.factionAlignmentChange != 0) {
            // Update faction properties based on impact
            // This would integrate with faction dynamics from Phase 1
        }
    }
}

void DecisionExecutor::applyResourceConsequences(int resourceId, int quantityChange) {
    if (resourceId >= 0) {
        auto resource = ResourceRegistry::getInstance().getResourceById(resourceId);
        if (resource) {
            int newQuantity = resource->getQuantity() + quantityChange;
            resource->setQuantity(std::max(0, newQuantity));
        }
    }
}

void DecisionExecutor::processCascades(const Decision& /* decision */, const std::vector<SimulationImpact>& impacts) {
    // Implement cascade mechanics
    // E.g., if faction loyalty drops below threshold, trigger rebellion
    // If resource depletes, trigger migration, etc.
    
    for (const auto& impact : impacts) {
        // Check cascade conditions...
        // This integrates with Phase 1 cascade system
        (void)impact;  // Suppress unused variable warning
    }
}

// ==================== RESULT DISPLAY ====================

std::string ResultDisplay::formatResult(const Decision& decision) {
    std::stringstream ss;
    
    ss << "[DECISION] You chose to: ";
    
    switch (decision.actionType) {
        case ActionType::ALLOCATE:
            ss << "Allocate " << decision.targetQuantity << " resources";
            break;
        case ActionType::INSPIRE:
            ss << "Inspire and motivate";
            break;
        case ActionType::SUPPRESS:
            ss << "Suppress and enforce";
            break;
        case ActionType::NEGOTIATE:
            ss << "Negotiate";
            break;
        case ActionType::DELEGATE:
            ss << "Delegate";
            break;
        default:
            ss << "Take action";
    }
    
    ss << "\n[TONE] " << (decision.tone == ToneType::POSITIVE ? "Positive" : 
                           decision.tone == ToneType::NEGATIVE ? "Negative" : "Neutral");
    
    ss << "\n[IMPACTS]\n";
    for (const auto& impact : decision.impacts) {
        ss << formatImpact(impact) << "\n";
    }
    
    if (!decision.narrativeFlavor.empty()) {
        ss << "\n[NARRATIVE]\n" << decision.narrativeFlavor;
    }
    
    return ss.str();
}

std::string ResultDisplay::formatImpact(const SimulationImpact& impact) {
    std::stringstream ss;
    
    if (impact.affectedNPCId >= 0) {
        auto npc = NPCRegistry::getInstance().getNPCById(impact.affectedNPCId);
        if (npc) {
            ss << "  " << npc->getName() << ": ";
            if (impact.loyaltyDelta > 0) {
                ss << "Loyalty +" << impact.loyaltyDelta;
            } else if (impact.loyaltyDelta < 0) {
                ss << "Loyalty " << impact.loyaltyDelta;
            }
        }
    }
    
    if (impact.affectedResourceId >= 0) {
        auto resource = ResourceRegistry::getInstance().getResourceById(impact.affectedResourceId);
        if (resource) {
            ss << "  " << resource->getName() << ": " << impact.resourceQuantityChange << " units";
        }
    }
    
    return ss.str();
}

std::string ResultDisplay::formatMultipleImpacts(const std::vector<SimulationImpact>& impacts) {
    std::stringstream ss;
    for (const auto& impact : impacts) {
        ss << formatImpact(impact) << "\n";
    }
    return ss.str();
}

std::string ResultDisplay::buildNPCReaction(int npcId, const SimulationImpact& impact) {
    auto npc = NPCRegistry::getInstance().getNPCById(npcId);
    if (!npc) return "";
    
    // Generate NPC reaction based on personality and impact
    std::string reaction;
    
    if (impact.loyaltyDelta > 0.1f) {
        reaction = npc->getName() + " looks pleased with your decision.";
    } else if (impact.loyaltyDelta < -0.1f) {
        reaction = npc->getName() + " looks displeased.";
    } else {
        reaction = npc->getName() + " acknowledges your decision.";
    }
    
    return reaction;
}

std::string ResultDisplay::buildFactionReaction(int factionId, const SimulationImpact& impact) {
    auto faction = FactionRegistry::getInstance().getFactionById(factionId);
    if (!faction) return "";
    
    std::string reaction = faction->getName() + " faction: ";
    if (impact.factionAlignmentChange > 0) {
        reaction += "Alignment improved.";
    } else if (impact.factionAlignmentChange < 0) {
        reaction += "Alignment worsened.";
    } else {
        reaction += "No change.";
    }
    
    return reaction;
}

std::string ResultDisplay::buildResourceReport(int resourceId, int quantityChange) {
    auto resource = ResourceRegistry::getInstance().getResourceById(resourceId);
    if (!resource) return "";
    
    std::stringstream ss;
    ss << resource->getName() << ": " << resource->getQuantity();
    if (quantityChange != 0) {
        ss << " (" << (quantityChange > 0 ? "+" : "") << quantityChange << ")";
    }
    
    return ss.str();
}

// ==================== DECISION PROCESSOR ====================

Decision DecisionProcessor::processPlayerInput(
    const std::string& input,
    LLMProvider* llmProvider,
    bool executeImmediately
) {
    Decision decision;
    decision.playerInput = input;
    
    // Step 1: Parse input
    ParseResult parseResult = InputParser::parsePlayerInput(input);
    if (!parseResult.isValid) {
        decision.actionType = ActionType::UNKNOWN;
        return decision;
    }
    
    decision.actionType = parseResult.actionType;
    decision.confidence = parseResult.confidence;
    decision.parameters = parseResult.parameters;
    
    // Step 2: Extract parameters with world state
    std::vector<ExtractedParameter> extractedParams = 
        ParameterExtractor::extractParametersWithWorldState(input, decision.actionType);
    decision.parameters = extractedParams;
    
    // Identify target
    decision.targetType = ParameterExtractor::detectTargetType(extractedParams);
    if (decision.targetType == TargetType::NPC) {
        decision.targetNPCId = ParameterExtractor::extractNPCId(input);
    } else if (decision.targetType == TargetType::FACTION) {
        decision.targetFactionId = ParameterExtractor::extractFactionId(input);
    }
    decision.targetResourceId = ParameterExtractor::extractResourceId(input);
    decision.targetQuantity = ParameterExtractor::extractQuantity(input);
    
    // Step 3: Detect tone
    ToneResult toneResult = ToneDetector::detectTone(input, llmProvider, true);
    decision.tone = toneResult.tone;
    decision.toneMultiplier = DecisionExecutor::getToneMultiplier(toneResult.tone);
    decision.usedLLM = toneResult.usedLLM;
    
    // Step 4: Validate
    ValidationResult validationResult = DecisionValidator::validateWithWorldState(decision);
    if (!validationResult.isValid) {
        decision.actionType = ActionType::UNKNOWN;
        return decision;
    }
    
    // Step 5: Execute if requested
    if (executeImmediately) {
        DecisionExecutor::executeDecision(decision);
    }
    
    // Step 6: Build narrative
    decision.narrativeFlavor = ResultDisplay::formatResult(decision);
    
    return decision;
}

Decision DecisionProcessor::processPlayerInputWithContext(
    const std::string& input,
    const std::vector<std::string>& contextNPCs,
    const std::vector<std::string>& contextFactions,
    const std::vector<std::string>& contextResources,
    LLMProvider* llmProvider,
    bool executeImmediately
) {
    Decision decision;
    decision.playerInput = input;
    
    // Use enhanced parsing with context
    ParseResult parseResult = InputParser::parsePlayerInputWithContext(
        input, contextNPCs, contextFactions, contextResources
    );
    
    if (!parseResult.isValid) {
        decision.actionType = ActionType::UNKNOWN;
        return decision;
    }
    
    decision.actionType = parseResult.actionType;
    decision.confidence = parseResult.confidence;
    
    // Continue with normal processing
    decision = processPlayerInput(input, llmProvider, executeImmediately);
    
    return decision;
}

std::string DecisionProcessor::getDecisionResultString(const Decision& decision) {
    return ResultDisplay::formatResult(decision);
}

std::string DecisionProcessor::buildLLMContext(const Decision& decision) {
    std::stringstream ss;
    
    ss << "Action: ";
    switch (decision.actionType) {
        case ActionType::ALLOCATE:
            ss << "ALLOCATE";
            break;
        case ActionType::INSPIRE:
            ss << "INSPIRE";
            break;
        case ActionType::SUPPRESS:
            ss << "SUPPRESS";
            break;
        case ActionType::NEGOTIATE:
            ss << "NEGOTIATE";
            break;
        default:
            ss << "UNKNOWN";
    }
    
    ss << "\nTone: " << (decision.tone == ToneType::POSITIVE ? "POSITIVE" : 
                        decision.tone == ToneType::NEGATIVE ? "NEGATIVE" : "NEUTRAL");
    ss << "\nInput: " << decision.playerInput;
    
    if (decision.targetNPCId >= 0) {
        auto npc = NPCRegistry::getInstance().getNPCById(decision.targetNPCId);
        if (npc) {
            ss << "\nTarget NPC: " << npc->getName();
        }
    }
    
    return ss.str();
}

}  // namespace TLS
