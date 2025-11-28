#pragma once

#include "Core.h"
#include "Registries.h"
#include "LLMProvider.h"
#include <string>
#include <memory>
#include <fstream>

namespace TLS {

/**
 * @class GameDataLoader
 * @brief Loads game data from JSON files into registries
 * 
 * Handles loading of:
 * - NPCs from data/npcs.json
 * - Factions from data/factions.json
 * - Resources from data/resources.json
 * - Advisors from data/advisors.json
 * - LLM configuration from data/llm_config.json
 */
class GameDataLoader {
public:
    /**
     * Construct loader with base data directory path
     * @param dataPath Path to data directory (e.g., "../data")
     */
    explicit GameDataLoader(const std::string& dataPath = "data");
    
    /**
     * Load all game data into provided registries
     * @param npcRegistry Registry to populate with NPCs
     * @param factionRegistry Registry to populate with factions
     * @param resourceRegistry Registry to populate with resources
     * @param advisorRegistry Registry to populate with advisors
     * @return true if all data loaded successfully
     */
    bool loadAllData(
        NPCRegistry& npcRegistry,
        FactionRegistry& factionRegistry,
        ResourceRegistry& resourceRegistry,
        AdvisorRegistry& advisorRegistry
    );
    
    /**
     * Load NPCs from npcs.json
     * @param registry NPCRegistry to populate
     * @return Number of NPCs loaded, or -1 on error
     */
    int loadNPCs(NPCRegistry& registry);
    
    /**
     * Load factions from factions.json
     * @param registry FactionRegistry to populate
     * @return Number of factions loaded, or -1 on error
     */
    int loadFactions(FactionRegistry& registry);
    
    /**
     * Load resources from resources.json
     * @param registry ResourceRegistry to populate
     * @return Number of resources loaded, or -1 on error
     */
    int loadResources(ResourceRegistry& registry);
    
    /**
     * Load advisors from advisors.json
     * @param registry AdvisorRegistry to populate
     * @param npcRegistry NPCRegistry to reference for base NPC data
     * @return Number of advisors loaded, or -1 on error
     */
    int loadAdvisors(AdvisorRegistry& registry, NPCRegistry& npcRegistry);
    
    /**
     * Load LLM configuration from llm_config.json
     * @param config LLMConfig to populate
     * @return true if loaded successfully
     */
    bool loadLLMConfig(LLMConfig& config);
    
    /**
     * Get last error message
     * @return Error message string
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    std::string m_dataPath;
    std::string m_lastError;
    
    /**
     * Read file contents to string
     * @param filename File to read
     * @return File contents or empty string on error
     */
    std::string readFile(const std::string& filename);
    
    /**
     * Simple JSON string value extraction
     * @param json JSON string
     * @param key Key to find
     * @return Value as string, or empty string if not found
     */
    std::string getJsonString(const std::string& json, const std::string& key);
    
    /**
     * Simple JSON integer value extraction
     * @param json JSON string
     * @param key Key to find
     * @param defaultVal Default value if not found
     * @return Integer value
     */
    int getJsonInt(const std::string& json, const std::string& key, int defaultVal = 0);
    
    /**
     * Simple JSON float value extraction
     * @param json JSON string
     * @param key Key to find
     * @param defaultVal Default value if not found
     * @return Float value
     */
    float getJsonFloat(const std::string& json, const std::string& key, float defaultVal = 0.0f);
    
    /**
     * Extract Vector3 from JSON object
     * @param json JSON object string
     * @return Vector3 value
     */
    Vector3 getJsonVector3(const std::string& json);
    
    /**
     * Find next JSON object in array
     * @param json JSON string
     * @param startPos Position to start searching from
     * @param endPos Output: position after object ends
     * @return Object string or empty if not found
     */
    std::string findNextObject(const std::string& json, size_t startPos, size_t& endPos);
    
    /**
     * Convert alignment string to enum
     * @param str Alignment string ("PLAYER_FRIENDLY", "NEUTRAL", "HOSTILE")
     * @return Alignment enum value
     */
    Alignment stringToAlignment(const std::string& str);
    
    /**
     * Convert specialty string to enum
     * @param str Specialty string
     * @return Specialty enum value
     */
    Specialty stringToSpecialty(const std::string& str);
    
    /**
     * Convert agenda string to enum
     * @param str Agenda string
     * @return Agenda enum value
     */
    Agenda stringToAgenda(const std::string& str);
    
    /**
     * Convert strategy style string to enum
     * @param str Strategy style string
     * @return StrategyStyle enum value
     */
    StrategyStyle stringToStrategyStyle(const std::string& str);
};

}  // namespace TLS
