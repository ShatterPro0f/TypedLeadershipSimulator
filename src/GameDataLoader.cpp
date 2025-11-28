#include "GameDataLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace TLS {

// ============================================================================
// Constructor
// ============================================================================

GameDataLoader::GameDataLoader(const std::string& dataPath)
    : m_dataPath(dataPath)
{
    // Ensure path ends with separator
    if (!m_dataPath.empty() && 
        m_dataPath.back() != '/' && 
        m_dataPath.back() != '\\') {
        m_dataPath += "/";
    }
}

// ============================================================================
// Main load methods
// ============================================================================

bool GameDataLoader::loadAllData(
    NPCRegistry& npcRegistry,
    FactionRegistry& factionRegistry,
    ResourceRegistry& resourceRegistry,
    AdvisorRegistry& advisorRegistry
) {
    bool success = true;
    
    int factionCount = loadFactions(factionRegistry);
    if (factionCount < 0) {
        success = false;
    }
    
    int npcCount = loadNPCs(npcRegistry);
    if (npcCount < 0) {
        success = false;
    }
    
    int advisorCount = loadAdvisors(advisorRegistry, npcRegistry);
    if (advisorCount < 0) {
        success = false;
    }
    
    int resourceCount = loadResources(resourceRegistry);
    if (resourceCount < 0) {
        success = false;
    }
    
    return success;
}

int GameDataLoader::loadNPCs(NPCRegistry& registry) {
    std::string json = readFile(m_dataPath + "npcs.json");
    if (json.empty()) {
        m_lastError = "Failed to read npcs.json";
        return -1;
    }
    
    // Find the "npcs" array
    size_t arrayStart = json.find("\"npcs\"");
    if (arrayStart == std::string::npos) {
        m_lastError = "No 'npcs' array found in npcs.json";
        return -1;
    }
    
    arrayStart = json.find('[', arrayStart);
    if (arrayStart == std::string::npos) {
        m_lastError = "Malformed npcs.json - no array start";
        return -1;
    }
    
    int count = 0;
    size_t pos = arrayStart + 1;
    size_t endPos;
    
    while (true) {
        std::string objJson = findNextObject(json, pos, endPos);
        if (objJson.empty()) break;
        
        // Parse NPC from JSON object
        NPC npc;
        npc.setId(getJsonInt(objJson, "id"));
        npc.setName(getJsonString(objJson, "name"));
        npc.setAge(getJsonInt(objJson, "age"));
        npc.setGender(getJsonString(objJson, "gender"));
        npc.setRole(getJsonString(objJson, "role"));
        npc.setFactionId(getJsonInt(objJson, "factionId"));
        npc.setLoyalty(getJsonFloat(objJson, "loyalty", 0.5f));
        
        // Parse position
        size_t posStart = objJson.find("\"position\"");
        if (posStart != std::string::npos) {
            size_t braceStart = objJson.find('{', posStart);
            size_t braceEnd = objJson.find('}', braceStart);
            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                std::string posJson = objJson.substr(braceStart, braceEnd - braceStart + 1);
                npc.setPosition(getJsonVector3(posJson));
            }
        }
        
        // Parse home location
        size_t homeStart = objJson.find("\"homeLocation\"");
        if (homeStart != std::string::npos) {
            size_t braceStart = objJson.find('{', homeStart);
            size_t braceEnd = objJson.find('}', braceStart);
            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                std::string homeJson = objJson.substr(braceStart, braceEnd - braceStart + 1);
                npc.setHomeLocation(getJsonVector3(homeJson));
            }
        }
        
        // Parse personality traits
        size_t personalityStart = objJson.find("\"personality\"");
        if (personalityStart != std::string::npos) {
            size_t arrayBegin = objJson.find('[', personalityStart);
            size_t arrayEnd = objJson.find(']', arrayBegin);
            if (arrayBegin != std::string::npos && arrayEnd != std::string::npos) {
                std::string traitArray = objJson.substr(arrayBegin, arrayEnd - arrayBegin + 1);
                size_t traitPos = 0;
                while (true) {
                    size_t quoteStart = traitArray.find('"', traitPos);
                    if (quoteStart == std::string::npos) break;
                    size_t quoteEnd = traitArray.find('"', quoteStart + 1);
                    if (quoteEnd == std::string::npos) break;
                    std::string trait = traitArray.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    npc.addPersonalityTrait(trait);
                    traitPos = quoteEnd + 1;
                }
            }
        }
        
        // Set default activity
        npc.setActivity(Activity::IDLE);
        npc.setSpeed(1.0f);
        npc.setImmediateEmotion(0.5f);
        npc.setShortTermMood(0.5f);
        npc.setLongTermAttitude(0.5f);
        
        registry.addNPC(npc);
        count++;
        pos = endPos;
    }
    
    return count;
}

int GameDataLoader::loadFactions(FactionRegistry& registry) {
    std::string json = readFile(m_dataPath + "factions.json");
    if (json.empty()) {
        m_lastError = "Failed to read factions.json";
        return -1;
    }
    
    // Find the "factions" array
    size_t arrayStart = json.find("\"factions\"");
    if (arrayStart == std::string::npos) {
        m_lastError = "No 'factions' array found in factions.json";
        return -1;
    }
    
    arrayStart = json.find('[', arrayStart);
    if (arrayStart == std::string::npos) {
        m_lastError = "Malformed factions.json - no array start";
        return -1;
    }
    
    int count = 0;
    size_t pos = arrayStart + 1;
    size_t endPos;
    
    while (true) {
        std::string objJson = findNextObject(json, pos, endPos);
        if (objJson.empty()) break;
        
        Faction faction;
        faction.setId(getJsonInt(objJson, "id"));
        faction.setName(getJsonString(objJson, "name"));
        faction.setStrength(getJsonFloat(objJson, "strength", 0.5f));
        faction.setAlignment(stringToAlignment(getJsonString(objJson, "alignment")));
        
        // Parse home location
        size_t homeStart = objJson.find("\"homeLocation\"");
        if (homeStart != std::string::npos) {
            size_t braceStart = objJson.find('{', homeStart);
            size_t braceEnd = objJson.find('}', braceStart);
            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                std::string homeJson = objJson.substr(braceStart, braceEnd - braceStart + 1);
                faction.setHomeLocation(getJsonVector3(homeJson));
            }
        }
        
        // Parse leaders array
        size_t leadersStart = objJson.find("\"leaders\"");
        if (leadersStart != std::string::npos) {
            size_t arrayBegin = objJson.find('[', leadersStart);
            size_t arrayEnd = objJson.find(']', arrayBegin);
            if (arrayBegin != std::string::npos && arrayEnd != std::string::npos) {
                std::string leaderArray = objJson.substr(arrayBegin + 1, arrayEnd - arrayBegin - 1);
                std::istringstream ss(leaderArray);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    // Trim whitespace
                    token.erase(0, token.find_first_not_of(" \t\n\r"));
                    token.erase(token.find_last_not_of(" \t\n\r") + 1);
                    if (!token.empty()) {
                        try {
                            faction.addLeaderId(std::stoi(token));
                        } catch (...) {}
                    }
                }
            }
        }
        
        registry.addFaction(faction);
        count++;
        pos = endPos;
    }
    
    return count;
}

int GameDataLoader::loadResources(ResourceRegistry& registry) {
    std::string json = readFile(m_dataPath + "resources.json");
    if (json.empty()) {
        m_lastError = "Failed to read resources.json";
        return -1;
    }
    
    // Find the "resources" array
    size_t arrayStart = json.find("\"resources\"");
    if (arrayStart == std::string::npos) {
        m_lastError = "No 'resources' array found in resources.json";
        return -1;
    }
    
    arrayStart = json.find('[', arrayStart);
    if (arrayStart == std::string::npos) {
        m_lastError = "Malformed resources.json - no array start";
        return -1;
    }
    
    int count = 0;
    size_t pos = arrayStart + 1;
    size_t endPos;
    
    while (true) {
        std::string objJson = findNextObject(json, pos, endPos);
        if (objJson.empty()) break;
        
        Resource resource;
        resource.setId(getJsonInt(objJson, "id"));
        resource.setName(getJsonString(objJson, "name"));
        resource.setQuantity(getJsonInt(objJson, "quantity"));
        resource.setProductionRate(getJsonInt(objJson, "productionRate"));
        resource.setConsumptionRate(getJsonInt(objJson, "consumptionRate"));
        resource.setScarcityThreshold(getJsonInt(objJson, "scarcityThreshold"));
        
        // Parse location
        size_t locStart = objJson.find("\"location\"");
        if (locStart != std::string::npos) {
            size_t braceStart = objJson.find('{', locStart);
            size_t braceEnd = objJson.find('}', braceStart);
            if (braceStart != std::string::npos && braceEnd != std::string::npos) {
                std::string locJson = objJson.substr(braceStart, braceEnd - braceStart + 1);
                resource.setLocation(getJsonVector3(locJson));
            }
        }
        
        registry.addResource(resource);
        count++;
        pos = endPos;
    }
    
    return count;
}

int GameDataLoader::loadAdvisors(AdvisorRegistry& registry, NPCRegistry& npcRegistry) {
    std::string json = readFile(m_dataPath + "advisors.json");
    if (json.empty()) {
        m_lastError = "Failed to read advisors.json";
        return -1;
    }
    
    // Find the "advisors" array
    size_t arrayStart = json.find("\"advisors\"");
    if (arrayStart == std::string::npos) {
        m_lastError = "No 'advisors' array found in advisors.json";
        return -1;
    }
    
    arrayStart = json.find('[', arrayStart);
    if (arrayStart == std::string::npos) {
        m_lastError = "Malformed advisors.json - no array start";
        return -1;
    }
    
    int count = 0;
    size_t pos = arrayStart + 1;
    size_t endPos;
    
    while (true) {
        std::string objJson = findNextObject(json, pos, endPos);
        if (objJson.empty()) break;
        
        auto advisor = std::make_shared<Advisor>();
        advisor->setId(getJsonInt(objJson, "id"));
        advisor->setName(getJsonString(objJson, "name"));
        advisor->setSpecialty(stringToSpecialty(getJsonString(objJson, "specialty")));
        advisor->setTrustLevel(getJsonFloat(objJson, "trustLevel", 0.5f));
        advisor->setFactionAlignment(getJsonFloat(objJson, "factionAlignment", 0.0f));
        advisor->setAgenda(stringToAgenda(getJsonString(objJson, "agenda")));
        advisor->setRiskTolerance(getJsonFloat(objJson, "riskTolerance", 0.5f));
        advisor->setStrategyStyle(stringToStrategyStyle(getJsonString(objJson, "strategyStyle")));
        
        // Copy some attributes from base NPC if basedOnNpcId exists
        int baseNpcId = getJsonInt(objJson, "basedOnNpcId");
        if (baseNpcId > 0) {
            auto baseNpc = npcRegistry.getNPCById(baseNpcId);
            if (baseNpc) {
                advisor->setPosition(baseNpc->getPosition());
                advisor->setHomeLocation(baseNpc->getHomeLocation());
                advisor->setFactionId(baseNpc->getFactionId());
            }
        }
        
        advisor->setActivity(Activity::IDLE);
        advisor->setSpeed(1.0f);
        
        registry.registerAdvisor(advisor);
        count++;
        pos = endPos;
    }
    
    return count;
}

bool GameDataLoader::loadLLMConfig(LLMConfig& config) {
    std::string json = readFile(m_dataPath + "llm_config.json");
    if (json.empty()) {
        m_lastError = "Failed to read llm_config.json";
        return false;
    }
    
    std::string provider = getJsonString(json, "provider");
    if (provider == "ollama") {
        config.preferredProvider = LLMProviderType::OLLAMA;
    } else if (provider == "openai") {
        config.preferredProvider = LLMProviderType::OPENAI;
    } else if (provider == "local" || provider == "llama") {
        config.preferredProvider = LLMProviderType::LLAMA_LOCAL;
    } else {
        config.preferredProvider = LLMProviderType::OFFLINE_FALLBACK;
    }
    
    // Find Ollama config object
    size_t ollamaStart = json.find("\"ollama\"");
    if (ollamaStart != std::string::npos) {
        size_t braceStart = json.find('{', ollamaStart);
        size_t braceEnd = json.find('}', braceStart);
        if (braceStart != std::string::npos && braceEnd != std::string::npos) {
            std::string ollamaJson = json.substr(braceStart, braceEnd - braceStart + 1);
            config.ollamaServerUrl = getJsonString(ollamaJson, "serverUrl");
            config.ollamaModel = getJsonString(ollamaJson, "model");
            config.ollamaTimeoutSeconds = getJsonInt(ollamaJson, "timeoutSeconds", 60);
        }
    }
    
    // Check fallback settings
    size_t fallbackStart = json.find("\"fallback\"");
    if (fallbackStart != std::string::npos) {
        size_t braceStart = json.find('{', fallbackStart);
        size_t braceEnd = json.find('}', braceStart);
        if (braceStart != std::string::npos && braceEnd != std::string::npos) {
            std::string fallbackJson = json.substr(braceStart, braceEnd - braceStart + 1);
            std::string enabled = getJsonString(fallbackJson, "enabled");
            config.fallbackEnabled = (enabled == "true");
        }
    }
    
    return true;
}

// ============================================================================
// Helper methods
// ============================================================================

std::string GameDataLoader::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string GameDataLoader::getJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    // Skip whitespace
    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && std::isspace(json[valueStart])) {
        valueStart++;
    }
    
    if (valueStart >= json.size()) {
        return "";
    }
    
    // Check if it's a string value
    if (json[valueStart] == '"') {
        size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) {
            return "";
        }
        return json.substr(valueStart + 1, valueEnd - valueStart - 1);
    }
    
    // It's a number or other value - find end
    size_t valueEnd = valueStart;
    while (valueEnd < json.size() && 
           !std::isspace(json[valueEnd]) && 
           json[valueEnd] != ',' && 
           json[valueEnd] != '}' && 
           json[valueEnd] != ']') {
        valueEnd++;
    }
    
    return json.substr(valueStart, valueEnd - valueStart);
}

int GameDataLoader::getJsonInt(const std::string& json, const std::string& key, int defaultVal) {
    std::string value = getJsonString(json, key);
    if (value.empty()) {
        return defaultVal;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultVal;
    }
}

float GameDataLoader::getJsonFloat(const std::string& json, const std::string& key, float defaultVal) {
    std::string value = getJsonString(json, key);
    if (value.empty()) {
        return defaultVal;
    }
    try {
        return std::stof(value);
    } catch (...) {
        return defaultVal;
    }
}

Vector3 GameDataLoader::getJsonVector3(const std::string& json) {
    float x = getJsonFloat(json, "x", 0.0f);
    float y = getJsonFloat(json, "y", 0.0f);
    float z = getJsonFloat(json, "z", 0.0f);
    return Vector3(x, y, z);
}

std::string GameDataLoader::findNextObject(const std::string& json, size_t startPos, size_t& endPos) {
    // Find opening brace
    size_t braceStart = json.find('{', startPos);
    if (braceStart == std::string::npos) {
        endPos = std::string::npos;
        return "";
    }
    
    // Find matching closing brace (handle nested objects)
    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '{') {
            depth++;
        } else if (json[pos] == '}') {
            depth--;
        }
        pos++;
    }
    
    if (depth != 0) {
        endPos = std::string::npos;
        return "";
    }
    
    endPos = pos;
    return json.substr(braceStart, pos - braceStart);
}

Alignment GameDataLoader::stringToAlignment(const std::string& str) {
    if (str == "PLAYER_FRIENDLY") {
        return Alignment::PLAYER_FRIENDLY;
    } else if (str == "HOSTILE") {
        return Alignment::HOSTILE;
    }
    return Alignment::NEUTRAL;
}

Specialty GameDataLoader::stringToSpecialty(const std::string& str) {
    if (str == "POLITICS") {
        return Specialty::POLITICS;
    } else if (str == "MILITARY") {
        return Specialty::MILITARY;
    } else if (str == "CULTURE") {
        return Specialty::CULTURE;
    } else if (str == "RELIGION") {
        return Specialty::RELIGION;
    }
    return Specialty::POLITICS;
}

Agenda GameDataLoader::stringToAgenda(const std::string& str) {
    if (str == "LONG_TERM") {
        return Agenda::LONG_TERM;
    }
    return Agenda::SHORT_TERM;
}

StrategyStyle GameDataLoader::stringToStrategyStyle(const std::string& str) {
    if (str == "HONEST") {
        return StrategyStyle::HONEST;
    } else if (str == "PERSUASIVE") {
        return StrategyStyle::PERSUASIVE;
    }
    return StrategyStyle::MANIPULATIVE;
}

}  // namespace TLS
