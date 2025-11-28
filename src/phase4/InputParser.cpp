#include "InputParser.h"
#include "Registries.h"
#include "Core.h"
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace TLS;

// ============================================================================
// ActionRegistry Implementation
// ============================================================================

ActionRegistry& ActionRegistry::getInstance()
{
    static ActionRegistry instance;
    return instance;
}

ActionRegistry::ActionRegistry()
{
    initializeBuiltins();
}

void ActionRegistry::initializeBuiltins()
{
    // Action: ALLOCATE (give resources to NPCs/factions)
    ActionDefinition allocateAction;
    allocateAction.name = "allocate";
    allocateAction.aliases = {"give", "distribute", "provide", "help", "support"};
    allocateAction.parameters = {
        {" resource", Parameter::RESOURCE, true},
        {"target", Parameter::NPC, false},
        {"amount", Parameter::QUANTITY, false}
    };
    allocateAction.formula = "loyalty_delta = 0.05 * (amount / threshold)";
    allocateAction.description = "Allocate resources to NPCs or factions to improve morale and loyalty";
    allocateAction.minQuantity = 1.0f;
    allocateAction.maxQuantity = 500.0f;
    registerAction(allocateAction);

    // Action: DELEGATE
    ActionDefinition delegateAction;
    delegateAction.name = "delegate";
    delegateAction.aliases = {"assign", "task", "command"};
    delegateAction.parameters = {
        {"target", Parameter::NPC, true},
        {"task", Parameter::STRING, true}
    };
    delegateAction.formula = "mood_delta based on task type and NPC competence";
    delegateAction.description = "Delegate a task to an NPC or faction";
    registerAction(delegateAction);

    // Action: INSPIRE
    ActionDefinition inspireAction;
    inspireAction.name = "inspire";
    inspireAction.aliases = {"rally", "motivate", "encourage"};
    inspireAction.parameters = {
        {"target", Parameter::NPC, false}
    };
    inspireAction.formula = "immediate_emotion += 0.3; mood smoothing toward high state";
    inspireAction.description = "Inspire NPCs or entire factions to increase morale";
    registerAction(inspireAction);

    // Action: NEGOTIATE
    ActionDefinition negotiateAction;
    negotiateAction.name = "negotiate";
    negotiateAction.aliases = {"discuss", "parley", "diplomacy", "peace"};
    negotiateAction.parameters = {
        {"target", Parameter::FACTION, true},
        {"offer", Parameter::STRING, false}
    };
    negotiateAction.formula = "faction_alignment shift + loyalty adjustment";
    negotiateAction.description = "Negotiate with factions to resolve conflicts";
    registerAction(negotiateAction);

    // Action: SUPPRESS
    ActionDefinition suppressAction;
    suppressAction.name = "suppress";
    suppressAction.aliases = {"control", "manage", "contain"};
    suppressAction.parameters = {
        {"target", Parameter::FACTION, true}
    };
    suppressAction.formula = "mood_delta = -0.2; loyalty_delta = -0.1; increases rebellion probability";
    suppressAction.description = "Suppress faction activities or dissent";
    registerAction(suppressAction);

    actions_["allocate"] = allocateAction;
    actions_["delegate"] = delegateAction;
    actions_["inspire"] = inspireAction;
    actions_["negotiate"] = negotiateAction;
    actions_["suppress"] = suppressAction;
}

bool ActionRegistry::loadFromFile(const std::string& filename)
{
    // TODO: Implement JSON loading
    return false;
}

const ActionRegistry::ActionDefinition* ActionRegistry::getAction(const std::string& name) const
{
    auto it = actions_.find(name);
    if (it != actions_.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> ActionRegistry::getAllActionNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : actions_)
    {
        names.push_back(pair.first);
    }
    return names;
}

void ActionRegistry::registerAction(const ActionDefinition& def)
{
    actions_[def.name] = def;

    // Also register aliases
    for (const auto& alias : def.aliases)
    {
        actions_[alias] = def;
    }
}

// ============================================================================
// InputParser Implementation
// ============================================================================

InputParser& InputParser::getInstance()
{
    static InputParser instance;
    return instance;
}

InputParser::InputParser()
{
    initializeToneKeywords();
}

void InputParser::initializeToneKeywords()
{
    toneKeywords_["please"] = "positive";
    toneKeywords_["thank"] = "positive";
    toneKeywords_["support"] = "positive";
    toneKeywords_["help"] = "positive";
    toneKeywords_["urgent"] = "aggressive";
    toneKeywords_["now"] = "aggressive";
    toneKeywords_["immediately"] = "aggressive";
    toneKeywords_["must"] = "aggressive";
    toneKeywords_["cannot"] = "negative";
    toneKeywords_["never"] = "negative";
    toneKeywords_["perhaps"] = "neutral";
    toneKeywords_["maybe"] = "neutral";
}

ParseResult InputParser::parseInput(const std::string& input, const ActionRegistry& registry)
{
    ParseResult result;
    result.action = "";
    result.confidence = 0.0f;
    result.usageFrequency = 0;

    // Normalize input
    std::string normalized = input;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    // Find best matching action
    float bestConfidence = 0.0f;
    std::string bestAction;

    auto allActions = registry.getAllActionNames();
    for (const auto& actionName : allActions)
    {
        float confidence = calculateConfidence(normalized, actionName);
        if (confidence > bestConfidence)
        {
            bestConfidence = confidence;
            bestAction = actionName;
        }
    }

    result.action = bestAction;
    result.confidence = bestConfidence;

    // Extract parameters
    result.parameters.push_back(extractQuantity(input) > 0 ? std::to_string(static_cast<int>(extractQuantity(input))) : "");

    return result;
}

bool InputParser::validateParameters(const ParseResult& parseResult,
                                     const NPCRegistry& npcRegistry,
                                     const FactionRegistry& factionRegistry,
                                     const ResourceRegistry& resourceRegistry)
{
    // Basic validation - action must exist
    return !parseResult.action.empty() && parseResult.confidence >= 0.6f;
}

float InputParser::calculateConfidence(const std::string& input, const std::string& knownAction)
{
    float ld_confidence = 1.0f - (static_cast<float>(levenshteinDistance(input, knownAction)) / LEVENSHTEIN_MAX_DIST);
    ld_confidence = std::max(0.0f, ld_confidence);

    float exact_confidence = exactMatchScore(input, knownAction);

    // Weighted combination
    float combined = LEVENSHTEIN_WEIGHT * ld_confidence + EXACT_WEIGHT * exact_confidence;
    return std::min(1.0f, combined);
}

std::vector<ParseResult> InputParser::findAmbiguousMatches(const std::string& input,
                                                            const ActionRegistry& registry,
                                                            float threshold)
{
    std::vector<ParseResult> matches;

    auto allActions = registry.getAllActionNames();
    for (const auto& actionName : allActions)
    {
        float confidence = calculateConfidence(input, actionName);
        if (confidence >= threshold)
        {
            ParseResult result;
            result.action = actionName;
            result.confidence = confidence;
            matches.push_back(result);
        }
    }

    // Sort by confidence descending
    std::sort(matches.begin(), matches.end(),
              [](const ParseResult& a, const ParseResult& b) {
                  return a.confidence > b.confidence;
              });

    return matches;
}

float InputParser::extractQuantity(const std::string& input)
{
    std::istringstream iss(input);
    std::string word;
    float lastNumber = 0.0f;

    while (iss >> word)
    {
        try
        {
            float num = std::stof(word);
            lastNumber = num;
        }
        catch (...)
        {
            // Not a number, continue
        }
    }

    return lastNumber;
}

std::vector<std::string> InputParser::extractNPCNames(const std::string& input,
                                                      const NPCRegistry& registry)
{
    std::vector<std::string> names;
    // TODO: Implement NPC name extraction
    return names;
}

std::vector<std::string> InputParser::extractFactionNames(const std::string& input,
                                                          const FactionRegistry& registry)
{
    std::vector<std::string> names;
    // TODO: Implement faction name extraction
    return names;
}

std::vector<std::string> InputParser::extractResourceNames(const std::string& input,
                                                           const ResourceRegistry& registry)
{
    std::vector<std::string> names;
    // TODO: Implement resource name extraction
    return names;
}

std::string InputParser::detectTone(const std::string& input)
{
    std::string normalized = input;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    int positiveCount = 0, negativeCount = 0, aggressiveCount = 0;

    for (const auto& pair : toneKeywords_)
    {
        if (normalized.find(pair.first) != std::string::npos)
        {
            if (pair.second == "positive") positiveCount++;
            else if (pair.second == "negative") negativeCount++;
            else if (pair.second == "aggressive") aggressiveCount++;
        }
    }

    if (positiveCount > negativeCount && positiveCount > aggressiveCount)
        return "positive";
    if (negativeCount > positiveCount && negativeCount > aggressiveCount)
        return "negative";
    if (aggressiveCount > positiveCount && aggressiveCount > negativeCount)
        return "aggressive";

    return "neutral";
}

int InputParser::levenshteinDistance(const std::string& a, const std::string& b) const
{
    size_t m = a.length();
    size_t n = b.length();

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (size_t i = 0; i <= m; i++)
        dp[i][0] = i;
    for (size_t j = 0; j <= n; j++)
        dp[0][j] = j;

    for (size_t i = 1; i <= m; i++)
    {
        for (size_t j = 1; j <= n; j++)
        {
            if (a[i - 1] == b[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}

float InputParser::wordEmbeddingSimilarity(const std::string& input, const std::string& known) const
{
    // Placeholder for ML-based embedding similarity
    // For now, return 0.0 (not implemented)
    return 0.0f;
}

float InputParser::exactMatchScore(const std::string& input, const std::string& known) const
{
    if (input == known)
        return 1.0f;
    if (input.find(known) != std::string::npos || known.find(input) != std::string::npos)
        return 0.95f;
    return 0.0f;
}
