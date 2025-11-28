#include "FuzzyParser.h"
#include <algorithm>
#include <cctype>
#include <set>

// Initialize static members
std::map<std::pair<std::string, std::string>, int> FuzzyParser::levenshteinCache_;
std::map<std::string, int> FuzzyParser::actionUsageFrequency_;

std::string FuzzyParser::toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string FuzzyParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> FuzzyParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string token;
    for (char c : str) {
        if (c == delimiter) {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        result.push_back(token);
    }
    return result;
}

int FuzzyParser::levenshteinDistance(const std::string& a, const std::string& b, int maxDistance) {
    // Check cache first
    auto cacheKey = std::make_pair(toLowercase(a), toLowercase(b));
    auto it = levenshteinCache_.find(cacheKey);
    if (it != levenshteinCache_.end()) {
        return it->second;
    }
    
    std::string a_lower = toLowercase(a);
    std::string b_lower = toLowercase(b);
    
    size_t len_a = a_lower.length();
    size_t len_b = b_lower.length();
    
    // Handle edge cases
    if (len_a == 0) return len_b;
    if (len_b == 0) return len_a;
    
    // Dynamic programming table
    std::vector<std::vector<int>> dp(len_a + 1, std::vector<int>(len_b + 1, 0));
    
    // Initialize first row and column
    for (size_t i = 0; i <= len_a; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len_b; ++j) dp[0][j] = j;
    
    // Fill DP table
    for (size_t i = 1; i <= len_a; ++i) {
        for (size_t j = 1; j <= len_b; ++j) {
            if (a_lower[i - 1] == b_lower[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];  // No operation needed
            } else {
                dp[i][j] = 1 + std::min({
                    dp[i - 1][j],      // deletion
                    dp[i][j - 1],      // insertion
                    dp[i - 1][j - 1]   // substitution
                });
            }
            
            // Early termination if distance exceeds max
            if (dp[i][j] > maxDistance) {
                dp[i][j] = maxDistance + 1;
            }
        }
    }
    
    int distance = dp[len_a][len_b];
    
    // Cache result
    levenshteinCache_[cacheKey] = distance;
    
    return distance;
}

float FuzzyParser::calculateCharacterOverlap(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return 0.0f;
    
    std::set<char> a_chars(a.begin(), a.end());
    std::set<char> b_chars(b.begin(), b.end());
    
    int overlap = 0;
    for (char c : a_chars) {
        if (b_chars.count(c)) {
            overlap++;
        }
    }
    
    int union_size = a_chars.size() + b_chars.size() - overlap;
    return static_cast<float>(overlap) / static_cast<float>(union_size);
}

float FuzzyParser::calculateConfidence(
    const std::string& input,
    const std::string& actionName,
    float w_exact,
    float w_fuzzy,
    float w_semantic
) {
    std::string input_lower = toLowercase(input);
    std::string action_lower = toLowercase(actionName);
    
    // Component 1: Exact match scoring
    float c_exact = 0.0f;
    if (input_lower == action_lower) {
        c_exact = 1.0f;  // Perfect match
    } else if (input_lower.find(action_lower) != std::string::npos ||
               action_lower.find(input_lower) != std::string::npos) {
        c_exact = 0.95f;  // Substring match
    }
    
    // Component 2: Fuzzy match scoring (Levenshtein distance)
    int distance = levenshteinDistance(input_lower, action_lower, 3);
    float c_fuzzy = std::max(0.0f, 1.0f - (static_cast<float>(distance) / 3.0f));
    
    // Component 3: Semantic scoring (character overlap)
    float c_semantic = calculateCharacterOverlap(input_lower, action_lower);
    
    // Normalize semantic score to 0-1
    c_semantic = (c_semantic + 0.5f) / 1.5f;  // Shift and scale to make it more useful
    
    // Hybrid score
    float confidence = (w_exact * c_exact) + (w_fuzzy * c_fuzzy) + (w_semantic * c_semantic);
    
    return std::max(0.0f, std::min(1.0f, confidence));  // Clamp to [0, 1]
}

std::vector<std::string> FuzzyParser::extractParametersBasic(
    const std::string& input,
    const std::string& actionName
) {
    // Split input by spaces
    std::vector<std::string> tokens = split(input, ' ');
    
    // Remove the action name and known keywords
    std::vector<std::string> params;
    std::string action_lower = toLowercase(actionName);
    
    std::set<std::string> stopwords = {
        "the", "a", "an", "to", "from", "for", "by", "in", "on", "at",
        "and", "or", "but", "if", "then", "as", "is", "are", "was", "were"
    };
    
    for (const auto& token : tokens) {
        std::string token_lower = toLowercase(token);
        
        // Skip if token is the action name or a stopword
        if (token_lower == action_lower || stopwords.count(token_lower)) {
            continue;
        }
        
        params.push_back(token);
    }
    
    return params;
}

std::vector<FuzzyParseResult> FuzzyParser::parseInput(
    const std::string& input,
    const ActionRegistry& registry
) {
    std::vector<FuzzyParseResult> results;
    
    std::string input_trimmed = trim(input);
    if (input_trimmed.empty()) {
        return results;  // Empty input
    }
    
    // Get all available actions and aliases
    const auto& allActionNames = registry.getAllActionNames();
    const auto& allAliases = registry.getAllAliases();
    
    // Score each action by name
    for (const auto& actionName : allActionNames) {
        float confidence = calculateConfidence(input_trimmed, actionName);
        
        // Only include matches above minimum confidence
        if (confidence > 0.3f) {
            FuzzyParseResult result;
            result.actionName = actionName;
            result.confidence = confidence;
            result.matchType = (confidence >= 0.95f) ? "exact" : "fuzzy";
            result.editDistance = levenshteinDistance(input_trimmed, actionName, 3);
            result.rawParams = extractParametersBasic(input_trimmed, actionName);
            result.reasoning = "Matched action name '" + actionName + "' with confidence " +
                              std::to_string(confidence).substr(0, 4);
            
            results.push_back(result);
        }
    }
    
    // Score each alias
    for (const auto& alias : allAliases) {
        float confidence = calculateConfidence(input_trimmed, alias);
        
        // Only include matches above minimum confidence
        if (confidence > 0.3f) {
            // Find the action this alias belongs to
            const ActionDefinition* action = registry.getActionByAlias(alias);
            if (action) {
                FuzzyParseResult result;
                result.actionName = action->name;
                result.confidence = confidence;
                result.matchType = (confidence >= 0.95f) ? "exact" : "fuzzy";
                result.editDistance = levenshteinDistance(input_trimmed, alias, 3);
                result.rawParams = extractParametersBasic(input_trimmed, action->name);
                result.reasoning = "Matched alias '" + alias + "' for action '" + 
                                  action->name + "' with confidence " +
                                  std::to_string(confidence).substr(0, 4);
                
                // Check if this action is already in results (avoid duplicates)
                bool alreadyExists = false;
                for (const auto& r : results) {
                    if (r.actionName == action->name) {
                        // Keep the higher confidence match
                        if (result.confidence > r.confidence) {
                            // Replace it
                            auto it = std::find_if(results.begin(), results.end(),
                                [&](const FuzzyParseResult& r) { return r.actionName == action->name; });
                            if (it != results.end()) {
                                *it = result;
                            }
                        }
                        alreadyExists = true;
                        break;
                    }
                }
                
                if (!alreadyExists) {
                    results.push_back(result);
                }
            }
        }
    }
    
    // Sort by confidence (highest first)
    std::sort(results.begin(), results.end(),
              [](const FuzzyParseResult& a, const FuzzyParseResult& b) {
                  if (a.confidence != b.confidence) {
                      return a.confidence > b.confidence;
                  }
                  // Tie-breaker: prefer exact matches and shorter edit distance
                  if (a.matchType != b.matchType) {
                      return a.matchType == "exact";
                  }
                  return a.editDistance < b.editDistance;
              });
    
    // Limit results to top 5 matches
    if (results.size() > 5) {
        results.resize(5);
    }
    
    return results;
}

void FuzzyParser::clearLevenshteinCache() {
    levenshteinCache_.clear();
}

size_t FuzzyParser::getCachSize() {
    return levenshteinCache_.size();
}
