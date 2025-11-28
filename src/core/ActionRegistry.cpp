#include "ActionRegistry.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <sstream>

// Manual JSON parsing - simple approach for action registry
// Format: JSON arrays and objects parsed by hand

// Static initialization
ActionRegistry* ActionRegistry::instance_ = nullptr;

/**
 * Constructor (private for singleton)
 */
ActionRegistry::ActionRegistry() {
    actions_.reserve(50);  // Reserve space upfront to avoid reallocations
}

/**
 * Destructor
 */
ActionRegistry::~ActionRegistry() {
    actions_.clear();
    actionsByName_.clear();
    actionsByAlias_.clear();
}

/**
 * Get singleton instance
 */
ActionRegistry& ActionRegistry::getInstance() {
    if (!instance_) {
        instance_ = new ActionRegistry();
    }
    return *instance_;
}

/**
 * Initialize singleton
 */
void ActionRegistry::initialize() {
    if (!instance_) {
        instance_ = new ActionRegistry();
    }
}

/**
 * Shutdown singleton
 */
void ActionRegistry::shutdown() {
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

/**
 * Helper: convert string to lowercase
 */
static std::string toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * Helper: trim whitespace
 */
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

/**
 * Helper: extract string value from JSON (very basic)
 */
static std::string extractJsonString(const std::string& line, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if (keyPos == std::string::npos) return "";
    
    size_t colonPos = line.find(":", keyPos);
    if (colonPos == std::string::npos) return "";
    
    size_t quoteStart = line.find("\"", colonPos);
    if (quoteStart == std::string::npos) return "";
    
    size_t quoteEnd = line.find("\"", quoteStart + 1);
    if (quoteEnd == std::string::npos) return "";
    
    return line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

/**
 * Helper: extract boolean value
 */
static bool extractJsonBool(const std::string& line, const std::string& key, bool defaultVal = false) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;
    
    size_t colonPos = line.find(":", keyPos);
    if (colonPos == std::string::npos) return defaultVal;
    
    std::string rest = line.substr(colonPos + 1);
    rest = trim(rest);
    
    return (rest.find("true") == 0);
}

/**
 * Helper: extract number value
 */
static int extractJsonInt(const std::string& line, const std::string& key, int defaultVal = 0) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;
    
    size_t colonPos = line.find(":", keyPos);
    if (colonPos == std::string::npos) return defaultVal;
    
    std::string rest = line.substr(colonPos + 1);
    rest = trim(rest);
    
    // Remove trailing comma if present
    if (!rest.empty() && rest.back() == ',') {
        rest.pop_back();
    }
    
    try {
        return std::stoi(rest);
    } catch (...) {
        return defaultVal;
    }
}

/**
 * Helper: extract float value
 */
static float extractJsonFloat(const std::string& line, const std::string& key, float defaultVal = 0.0f) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if (keyPos == std::string::npos) return defaultVal;
    
    size_t colonPos = line.find(":", keyPos);
    if (colonPos == std::string::npos) return defaultVal;
    
    std::string rest = line.substr(colonPos + 1);
    rest = trim(rest);
    
    // Remove trailing comma if present
    if (!rest.empty() && rest.back() == ',') {
        rest.pop_back();
    }
    
    try {
        return std::stof(rest);
    } catch (...) {
        return defaultVal;
    }
}

/**
 * Helper: extract array of strings
 */
static std::vector<std::string> extractJsonArrayOfStrings(const std::string& jsonContent, const std::string& key) {
    std::vector<std::string> result;
    
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = jsonContent.find(searchKey);
    if (keyPos == std::string::npos) return result;
    
    size_t bracketStart = jsonContent.find("[", keyPos);
    if (bracketStart == std::string::npos) return result;
    
    size_t bracketEnd = jsonContent.find("]", bracketStart);
    if (bracketEnd == std::string::npos) return result;
    
    std::string arrayContent = jsonContent.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
    
    // Split by commas and extract strings
    std::istringstream iss(arrayContent);
    std::string item;
    while (std::getline(iss, item, ',')) {
        item = trim(item);
        // Remove quotes
        if (!item.empty() && item.front() == '"' && item.back() == '"') {
            item = item.substr(1, item.length() - 2);
        }
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

/**
 * Load action registry from JSON file
 * Very simplified JSON parser - handles basic structure only
 */
bool ActionRegistry::loadFromJSON(const std::string& filename) {
    try {
        // Try primary path first
        std::ifstream file(filename);
        std::string actualPath = filename;
        
        // If file not found, try relative paths for different working directories
        if (!file.is_open()) {
            // Try ../data/ prefix (for tests running from build/tests)
            std::string altPath = "../" + filename;
            file.open(altPath);
            if (file.is_open()) {
                actualPath = altPath;
            } else {
                // Try ../../data/ prefix (additional fallback)
                altPath = "../../" + filename;
                file.open(altPath);
                if (file.is_open()) {
                    actualPath = altPath;
                }
            }
        }
        
        if (!file.is_open()) {
            std::cerr << "ERROR: Failed to open action registry file: " << filename << std::endl;
            return false;
        }
        
        // Read entire file into memory
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        file.close();
        
        // Find "actions" array
        size_t actionsPos = fileContent.find("\"actions\"");
        if (actionsPos == std::string::npos) {
            std::cerr << "ERROR: 'actions' field not found in JSON" << std::endl;
            return false;
        }
        
        // Find the [ after "actions"
        size_t arrayStart = fileContent.find("[", actionsPos);
        if (arrayStart == std::string::npos) {
            std::cerr << "ERROR: actions array start not found" << std::endl;
            return false;
        }
        
        // Find matching ]
        size_t arrayEnd = fileContent.rfind("]");
        if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
            std::cerr << "ERROR: actions array end not found" << std::endl;
            return false;
        }
        
        std::string arrayContent = fileContent.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Split into individual action objects (very basic - finds {...} patterns)
        int objectCount = 0;
        size_t objectStart = 0;
        int braceDepth = 0;
        
        for (size_t i = 0; i < arrayContent.length(); ++i) {
            if (arrayContent[i] == '{') {
                if (braceDepth == 0) {
                    objectStart = i;
                }
                braceDepth++;
            } else if (arrayContent[i] == '}') {
                braceDepth--;
                if (braceDepth == 0) {
                    std::string objectStr = arrayContent.substr(objectStart, i - objectStart + 1);
                    
                    // Parse single action
                    ActionDefinition action;
                    action.id = extractJsonInt(objectStr, "id", 0);
                    action.name = trim(extractJsonString(objectStr, "name"));
                    
                    if (action.name.empty()) {
                        continue;
                    }
                    
                    action.aliases = extractJsonArrayOfStrings(objectStr, "aliases");
                    action.tags = extractJsonArrayOfStrings(objectStr, "tags");
                    
                    // Parse parameterTypes
                    auto paramTypeStrs = extractJsonArrayOfStrings(objectStr, "parameterTypes");
                    for (const auto& typeStr : paramTypeStrs) {
                        if (typeStr == "RESOURCE") {
                            action.parameterTypes.push_back(ParameterType::RESOURCE);
                        } else if (typeStr == "NPC") {
                            action.parameterTypes.push_back(ParameterType::NPC);
                        } else if (typeStr == "FACTION") {
                            action.parameterTypes.push_back(ParameterType::FACTION);
                        } else if (typeStr == "NPC_OR_FACTION") {
                            action.parameterTypes.push_back(ParameterType::NPC_OR_FACTION);
                        } else if (typeStr == "QUANTITY") {
                            action.parameterTypes.push_back(ParameterType::QUANTITY);
                        } else if (typeStr == "STRING") {
                            action.parameterTypes.push_back(ParameterType::STRING);
                        } else if (typeStr == "FLOAT") {
                            action.parameterTypes.push_back(ParameterType::FLOAT);
                        } else if (typeStr == "BOOLEAN") {
                            action.parameterTypes.push_back(ParameterType::BOOLEAN);
                        } else if (typeStr == "TONE") {
                            action.parameterTypes.push_back(ParameterType::TONE);
                        } else if (typeStr == "LOCATION") {
                            action.parameterTypes.push_back(ParameterType::LOCATION);
                        }
                    }
                    
                    action.consequenceFormula = trim(extractJsonString(objectStr, "consequenceFormula"));
                    action.description = trim(extractJsonString(objectStr, "description"));
                    action.cascadeRisk = extractJsonFloat(objectStr, "cascadeRisk", 0.0f);
                    action.priority = extractJsonInt(objectStr, "priority", 5);
                    action.requiresConfirmation = extractJsonBool(objectStr, "requiresConfirmation", false);
                    action.confidenceThreshold = extractJsonFloat(objectStr, "confidenceThreshold", 0.85f);
                    
                    // Pre-allocate vector to avoid reallocations (reserve more if needed)
                    if (actions_.capacity() <= actions_.size()) {
                        actions_.reserve(actions_.size() + 20);
                    }
                    
                    // Add to registry
                    actions_.push_back(action);
                    
                    objectCount++;
                }
            }
        }
        
        // Build indexes after all actions loaded
        buildIndexes();
        
        std::cout << "Successfully loaded " << objectCount << " actions from registry" << std::endl;
        return objectCount > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in loadFromJSON: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Get action by primary name (case-insensitive)
 */
ActionDefinition* ActionRegistry::getActionByName(const std::string& name) const {
    std::string nameLower = toLowercase(name);
    auto it = actionsByName_.find(nameLower);
    if (it != actionsByName_.end()) {
        return (ActionDefinition*)&actions_[it->second];
    }
    return nullptr;
}

/**
 * Get action by alias (case-insensitive)
 */
ActionDefinition* ActionRegistry::getActionByAlias(const std::string& alias) const {
    std::string aliasLower = toLowercase(alias);
    auto it = actionsByAlias_.find(aliasLower);
    if (it != actionsByAlias_.end()) {
        return (ActionDefinition*)&actions_[it->second];
    }
    return nullptr;
}

/**
 * Get all actions
 */
const std::vector<ActionDefinition>& ActionRegistry::getAllActions() const {
    return actions_;
}

/**
 * Check if action is valid
 */
bool ActionRegistry::isValidAction(const std::string& actionName) const {
    return getActionByName(actionName) != nullptr;
}

/**
 * Check if alias is valid
 */
bool ActionRegistry::isValidAlias(const std::string& alias) const {
    return getActionByAlias(alias) != nullptr;
}

/**
 * Get all action names
 */
std::vector<std::string> ActionRegistry::getAllActionNames() const {
    std::vector<std::string> names;
    for (const auto& action : actions_) {
        names.push_back(action.name);
    }
    return names;
}

/**
 * Get all aliases
 */
std::vector<std::string> ActionRegistry::getAllAliases() const {
    std::vector<std::string> aliases;
    for (const auto& [alias, action] : actionsByAlias_) {
        aliases.push_back(alias);
    }
    return aliases;
}

/**
 * Print registry for debugging
 */
void ActionRegistry::printRegistry() const {
    std::cout << "=== ACTION REGISTRY ===" << std::endl;
    std::cout << "Total Actions: " << actions_.size() << std::endl;
    std::cout << std::endl;
    
    for (const auto& action : actions_) {
        std::cout << "[" << action.name << "] (Priority: " << action.priority << ")" << std::endl;
        std::cout << "  Description: " << action.description << std::endl;
        std::cout << "  Aliases: ";
        for (const auto& alias : action.aliases) {
            std::cout << alias << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Parameters: ";
        for (const auto& paramType : action.parameterTypes) {
            std::cout << action.paramTypeToString(paramType) << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Confidence Threshold: " << action.confidenceThreshold << std::endl;
        std::cout << "  Requires Confirmation: " << (action.requiresConfirmation ? "YES" : "NO") << std::endl;
        std::cout << "  Formula: " << action.consequenceFormula << std::endl;
        std::cout << std::endl;
    }
}

/**
 * Levenshtein distance for fuzzy matching
 */
int ActionRegistry::levenshteinDistance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i) d[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) d[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost});
        }
    }

    return d[len1][len2];
}

/**
 * Find action by fuzzy matching (typo tolerance)
 */
std::optional<ActionDefinition*> ActionRegistry::findActionByFuzzyMatch(
    const std::string& input, int maxDistance) const {
    
    std::string inputLower = toLowercase(input);
    
    // First try exact match
    auto it = actionsByName_.find(inputLower);
    if (it != actionsByName_.end()) {
        return (ActionDefinition*)&actions_[it->second];
    }
    
    auto it2 = actionsByAlias_.find(inputLower);
    if (it2 != actionsByAlias_.end()) {
        return (ActionDefinition*)&actions_[it2->second];
    }
    
    // Fuzzy match via Levenshtein distance
    int bestDistance = maxDistance + 1;
    ActionDefinition* bestMatch = nullptr;
    
    // Check against all action names
    for (const auto& action : actions_) {
        int dist = levenshteinDistance(inputLower, toLowercase(action.name));
        if (dist < bestDistance && dist <= maxDistance) {
            bestDistance = dist;
            bestMatch = (ActionDefinition*)&action;
        }
    }
    
    // Check against all aliases
    for (const auto& action : actions_) {
        for (const auto& alias : action.aliases) {
            int dist = levenshteinDistance(inputLower, toLowercase(alias));
            if (dist < bestDistance && dist <= maxDistance) {
                bestDistance = dist;
                bestMatch = (ActionDefinition*)&action;
            }
        }
    }
    
    if (bestMatch != nullptr) {
        return bestMatch;
    }
    
    return std::nullopt;
}

/**
 * Get action by ID
 */
ActionDefinition* ActionRegistry::getActionByID(int id) const {
    auto it = actionsByID_.find(std::to_string(id));
    if (it != actionsByID_.end()) {
        return (ActionDefinition*)&actions_[it->second];
    }
    return nullptr;
}

/**
 * Get actions by tag
 */
std::vector<ActionDefinition*> ActionRegistry::getActionsByTag(const std::string& tag) const {
    std::vector<ActionDefinition*> result;
    std::string tagLower = toLowercase(tag);
    
    for (const auto& action : actions_) {
        for (const auto& actionTag : action.tags) {
            if (toLowercase(actionTag) == tagLower) {
                result.push_back((ActionDefinition*)&action);
                break;
            }
        }
    }
    
    return result;
}

/**
 * Get actions by cascade risk range
 */
std::vector<ActionDefinition*> ActionRegistry::getActionsByCascadeRisk(
    float minRisk, float maxRisk) const {
    std::vector<ActionDefinition*> result;
    
    for (const auto& action : actions_) {
        if (action.cascadeRisk >= minRisk && action.cascadeRisk <= maxRisk) {
            result.push_back((ActionDefinition*)&action);
        }
    }
    
    return result;
}

/**
 * Validate parameter value
 */
bool ActionRegistry::validateParameter(
    const std::string& actionName, 
    const std::string& paramName, 
    const std::string& paramValue) const {
    
    ActionDefinition* action = getActionByName(actionName);
    if (!action) return false;
    
    auto param = getParameter(actionName, paramName);
    if (!param) return false;
    
    // Validate based on parameter type
    if (param->type == ParameterType::FLOAT) {
        try {
            float val = std::stof(paramValue);
            return val >= param->minValue && val <= param->maxValue;
        } catch (...) {
            return false;
        }
    } else if (param->type == ParameterType::STRING) {
        if (!param->options.empty()) {
            // Check if value is in valid options
            for (const auto& opt : param->options) {
                if (toLowercase(opt) == toLowercase(paramValue)) {
                    return true;
                }
            }
            return false;
        }
        return true;  // Any string valid if no options specified
    } else if (param->type == ParameterType::BOOLEAN) {
        return (toLowercase(paramValue) == "true" || 
                toLowercase(paramValue) == "false" ||
                toLowercase(paramValue) == "1" ||
                toLowercase(paramValue) == "0");
    }
    
    return true;
}

/**
 * Get parameter by action and parameter name
 */
std::optional<ActionParameter> ActionRegistry::getParameter(
    const std::string& actionName, 
    const std::string& paramName) const {
    
    ActionDefinition* action = getActionByName(actionName);
    if (!action) return std::nullopt;
    
    for (const auto& param : action->parameters) {
        if (toLowercase(param.name) == toLowercase(paramName)) {
            return param;
        }
    }
    
    return std::nullopt;
}

/**
 * Get global parameters
 */
const std::unordered_map<std::string, std::vector<std::string>>& 
ActionRegistry::getGlobalParameters() const {
    return globalParameters_;
}

/**
 * Get tone types
 */
std::vector<std::string> ActionRegistry::getToneTypes() const {
    auto it = globalParameters_.find("toneTypes");
    if (it != globalParameters_.end()) {
        return it->second;
    }
    return {};
}

/**
 * Get scope types
 */
std::vector<std::string> ActionRegistry::getScopeTypes() const {
    auto it = globalParameters_.find("scopeTypes");
    if (it != globalParameters_.end()) {
        return it->second;
    }
    return {};
}

/**
 * Get timing types
 */
std::vector<std::string> ActionRegistry::getTimingTypes() const {
    auto it = globalParameters_.find("timingTypes");
    if (it != globalParameters_.end()) {
        return it->second;
    }
    return {};
}

/**
 * Get method types
 */
std::vector<std::string> ActionRegistry::getMethodTypes() const {
    auto it = globalParameters_.find("methodTypes");
    if (it != globalParameters_.end()) {
        return it->second;
    }
    return {};
}

/**
 * Get all tags in registry
 */
std::vector<std::string> ActionRegistry::getAllTags() const {
    std::vector<std::string> allTags;
    for (const auto& action : actions_) {
        for (const auto& tag : action.tags) {
            // Avoid duplicates
            if (std::find(allTags.begin(), allTags.end(), tag) == allTags.end()) {
                allTags.push_back(tag);
            }
        }
    }
    return allTags;
}

/**
 * Get actions sorted by priority
 */
std::vector<ActionDefinition*> ActionRegistry::getActionsByPriority() const {
    std::vector<ActionDefinition*> result;
    for (const auto& action : actions_) {
        result.push_back((ActionDefinition*)&action);
    }
    // Sort by priority descending
    std::sort(result.begin(), result.end(), 
        [](ActionDefinition* a, ActionDefinition* b) {
            return a->priority > b->priority;
        });
    return result;
}

/**
 * Print single action for debugging
 */
void ActionRegistry::printAction(const ActionDefinition& action) const {
    std::cout << "[" << action.id << "] " << action.name << std::endl;
    std::cout << "  Description: " << action.description << std::endl;
    std::cout << "  Tags: ";
    for (const auto& tag : action.tags) {
        std::cout << tag << ", ";
    }
    std::cout << std::endl;
    std::cout << "  Cascade Risk: " << action.cascadeRisk << std::endl;
    std::cout << "  Priority: " << action.priority << std::endl;
    std::cout << "  Aliases: ";
    for (const auto& alias : action.aliases) {
        std::cout << alias << ", ";
    }
    std::cout << std::endl;
    std::cout << "  Formula: " << action.consequenceFormula << std::endl;
}

/**
 * Build quick lookup indexes
 */
void ActionRegistry::buildIndexes() {
    actionsByName_.clear();
    actionsByAlias_.clear();
    actionsByID_.clear();
    
    for (size_t i = 0; i < actions_.size(); ++i) {
        const auto& action = actions_[i];
        
        // Index by name
        actionsByName_[toLowercase(action.name)] = i;
        
        // Index by ID
        actionsByID_[std::to_string(action.id)] = i;
        
        // Index by aliases
        for (const auto& alias : action.aliases) {
            actionsByAlias_[toLowercase(alias)] = i;
        }
    }
}
