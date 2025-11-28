/**
 * Typed Leadership Simulator - Main Entry Point
 * 
 * A 3D first-person open-world emergent leadership simulation game.
 * This text-based version uses console I/O with a mandatory Ollama LLM backend.
 * 
 * The game starts with ~10 NPCs and grows organically through:
 * - Immigration based on settlement reputation
 * - Family growth (births)
 * - Conflict outcomes (enslavement)
 * 
 * Core Features:
 * - LLM interprets player's typed commands
 * - LLM generates emergent narratives from world state
 * - Deterministic simulation with equation-based consequences
 * - Event-driven, continuous real-time architecture
 */

#include "Core.h"
#include "Registries.h"
#include "GameDataLoader.h"
#include "ConsoleRenderer.h"
#include "OllamaProvider.h"
#include "LLMProvider.h"
#include "DecisionInterpreter.h"
#include "Serialization.h"

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace TLS;

// ============================================================================
// Game Configuration
// ============================================================================

struct GameConfig {
    std::string dataPath = "data/";
    bool colorOutput = true;
    int consoleWidth = 80;
    bool debugMode = false;
    std::string llmModel = "gemma3:12b";
};

// ============================================================================
// Game State
// ============================================================================

struct GameState {
    int currentTick = 0;
    int currentDay = 1;
    int currentHour = 8;  // Start at 8 AM
    int currentYear = 1;
    std::string currentSeason = "Spring";
    bool running = true;
    bool llmAvailable = false;
    
    // Event tracking
    int lastEventDay = 0;
    int eventsSinceStartup = 0;
    
    // Settlement stats
    float settlementReputation = 0.5f;  // 0-1, affects immigration
    int immigrationCooldown = 0;        // Days until next immigration check
};

// ============================================================================
// Helper Functions
// ============================================================================

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) return false;
    return str.substr(0, prefix.length()) == prefix;
}

// ============================================================================
// Game Class
// ============================================================================

class Game {
public:
    Game(const GameConfig& config)
        : m_config(config)
        , m_renderer()
        , m_dataLoader(config.dataPath)
    {
    }

    bool initialize() {
        m_renderer.displayMessage("Initializing Typed Leadership Simulator...\n");
        
        // Initialize renderer
        if (!m_renderer.initialize()) {
            m_renderer.displayError("Failed to initialize console renderer");
            return false;
        }
        m_renderer.setColorEnabled(m_config.colorOutput);
        m_renderer.setWidth(m_config.consoleWidth);
        
        // Clear registries before loading
        RegistryManager::getInstance().clearAll();
        
        // Load game data
        m_renderer.displayMessage("Loading game data...");
        
        auto& npcRegistry = NPCRegistry::getInstance();
        auto& factionRegistry = FactionRegistry::getInstance();
        auto& resourceRegistry = ResourceRegistry::getInstance();
        auto& advisorRegistry = AdvisorRegistry::getInstance();
        
        if (!m_dataLoader.loadAllData(npcRegistry, factionRegistry, resourceRegistry, advisorRegistry)) {
            m_renderer.displayError("Failed to load game data: " + m_dataLoader.getLastError());
            return false;
        }
        
        m_renderer.displayMessage("  Loaded " + std::to_string(npcRegistry.getNPCCount()) + " NPCs");
        m_renderer.displayMessage("  Loaded " + std::to_string(factionRegistry.getFactionCount()) + " factions");
        m_renderer.displayMessage("  Loaded " + std::to_string(resourceRegistry.getResourceCount()) + " resources");
        m_renderer.displayMessage("  Loaded " + std::to_string(advisorRegistry.getAdvisorCount()) + " advisors");
        
        // Load LLM configuration
        m_renderer.displayMessage("\nInitializing LLM connection...");
        if (!m_dataLoader.loadLLMConfig(m_llmConfig)) {
            m_renderer.displayNotification("Could not load LLM config, using defaults");
            m_llmConfig.preferredProvider = LLMProviderType::OLLAMA;
            m_llmConfig.ollamaServerUrl = "http://localhost:11434";
            m_llmConfig.ollamaModel = m_config.llmModel;
        }
        
        // Initialize Ollama provider
        initializeLLM();
        
        m_renderer.displayMessage("\nGame initialized successfully!\n");
        return true;
    }

    void run() {
        m_renderer.displayWelcome();
        
        // Show initial status
        m_renderer.displayGameTime(m_state.currentDay, m_state.currentHour, 
                                   m_state.currentSeason, m_state.currentYear);
        
        if (m_state.llmAvailable) {
            m_renderer.displayMessage("LLM Connected: " + m_llmConfig.ollamaModel);
        } else {
            m_renderer.displayNotification("LLM not available - using basic command parsing");
        }
        
        m_renderer.displayMessage("\nType 'help' for instructions or 'commands' for available actions.\n");
        
        // Main game loop - Event-driven continuous simulation (Task #7)
        while (m_state.running) {
            // Process one tick: Update NPC positions, emotions, moods, check for problems, etc.
            GameTickProcessor::instance()->processTick(m_state.currentTick);
            
            // Get player input (non-blocking in real version, blocking here for console)
            std::string input = m_renderer.getPlayerInput("\n> ");
            processCommand(input);
            
            // Advance tick counter for next iteration
            m_state.currentTick++;
        }
        
        m_renderer.shutdown();
    }

private:
    GameConfig m_config;
    GameState m_state;
    ConsoleRenderer m_renderer;
    GameDataLoader m_dataLoader;
    LLMConfig m_llmConfig;
    std::unique_ptr<OllamaProvider> m_llmProvider;
    std::shared_ptr<NPC> m_activeDialogueNPC;  // NPC currently in dialogue (Task #2)

    void initializeLLM() {
        m_llmProvider = std::make_unique<OllamaProvider>(
            m_llmConfig.ollamaServerUrl,
            m_llmConfig.ollamaModel
        );
        
        if (m_llmProvider->isAvailable()) {
            m_state.llmAvailable = true;
            m_renderer.displayMessage("  Connected to Ollama at " + m_llmConfig.ollamaServerUrl);
            m_renderer.displayMessage("  Using model: " + m_llmConfig.ollamaModel);
            
            // List available models
            auto models = m_llmProvider->getAvailableModels();
            if (!models.empty()) {
                m_renderer.displayMessage("  Available models: " + std::to_string(models.size()));
            }
        } else {
            m_state.llmAvailable = false;
            m_renderer.displayNotification("Ollama not available at " + m_llmConfig.ollamaServerUrl);
            m_renderer.displayMessage("  Make sure Ollama is running: 'ollama serve'");
            m_renderer.displayMessage("  Game will use basic command parsing.");
        }
    }

    void processCommand(const std::string& rawInput) {
        std::string input = toLower(rawInput);
        
        // Trim whitespace
        size_t start = input.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return;  // Empty input
        }
        input = input.substr(start);
        size_t end = input.find_last_not_of(" \t\n\r");
        input = input.substr(0, end + 1);
        
        if (input.empty()) return;
        
        // System commands (not sent to LLM)
        if (input == "quit" || input == "exit" || input == "q") {
            m_renderer.displayMessage("\nFarewell, leader. Your settlement awaits your return.\n");
            m_state.running = false;
            return;
        }
        
        if (input == "help") {
            m_renderer.displayHelp();
            return;
        }
        
        if (startsWith(input, "help ")) {
            std::string topic = input.substr(5);
            m_renderer.displayHelp(topic);
            return;
        }
        
        if (input == "commands") {
            m_renderer.displayCommands();
            return;
        }
        
        if (input == "status") {
            showStatus();
            return;
        }
        
        if (input == "resources") {
            showResources();
            return;
        }
        
        if (input == "factions") {
            showFactions();
            return;
        }
        
        if (input == "people" || input == "npcs") {
            showPeople();
            return;
        }
        
        if (input == "advisors" || input == "advice") {
            showAdvisors();
            return;
        }
        
        if (input == "wait") {
            advanceTime(1);
            return;
        }
        
        if (startsWith(input, "wait ")) {
            try {
                int hours = std::stoi(input.substr(5));
                advanceTime(hours);
            } catch (...) {
                m_renderer.displayError("Invalid time amount. Use 'wait 3' to wait 3 hours.");
            }
            return;
        }
        
        if (startsWith(input, "speak ") || startsWith(input, "talk ")) {
            std::string npcName = rawInput.substr(input.find(' ') + 1);
            speakWithNPC(npcName);
            return;
        }
        
        if (input == "llm" || input == "llm status") {
            showLLMStatus();
            return;
        }
        
        // Task #2: Handle NPC dialogue responses
        if (m_activeDialogueNPC) {
            handleDialogueResponse(input);
            return;
        }
        
        if (startsWith(input, "save")) {
            std::string saveFile = "auto_save";
            if (startsWith(input, "save ")) {
                saveFile = input.substr(5);
            }
            saveGame(saveFile);
            return;
        }
        
        if (startsWith(input, "load")) {
            std::string loadFile = "auto_save";
            if (startsWith(input, "load ")) {
                loadFile = input.substr(5);
            }
            loadGame(loadFile);
            return;
        }
        
        // For complex commands, try to use LLM interpretation
        if (m_state.llmAvailable) {
            interpretWithLLM(rawInput);
        } else {
            // Basic fallback parsing
            m_renderer.displayNotification("Command not recognized: '" + rawInput + "'");
            m_renderer.displayMessage("Type 'commands' for a list of available actions.");
        }
    }
    
    // ========================================================================
    // Save/Load System
    // ========================================================================
    
    void saveGame(const std::string& fileName) {
        m_renderer.displayMessage("\nSaving game to " + fileName + "...");
        
        SaveGame saveData;
        saveData.header.timestamp = static_cast<uint32_t>(std::time(nullptr));
        saveData.header.tickNumber = m_state.currentTick;
        saveData.header.npcCount = NPCRegistry::getInstance().getNPCCount();
        saveData.header.advisorCount = AdvisorRegistry::getInstance().getAdvisorCount();
        saveData.header.resourceCount = ResourceRegistry::getInstance().getResourceCount();
        saveData.header.factionCount = FactionRegistry::getInstance().getFactionCount();
        saveData.header.eventCount = EventRegistry::getInstance().getEventCount();
        
        std::string desc = "Day " + std::to_string(m_state.currentDay) + ", " + m_state.currentSeason + " Year " + std::to_string(m_state.currentYear);
        strncpy(reinterpret_cast<char*>(saveData.header.playerName), "Player", 127);
        strncpy(reinterpret_cast<char*>(saveData.header.gameDescription), desc.c_str(), 255);
        
        // Gather entities
        saveData.npcs = NPCRegistry::getInstance().getAllNPCs();
        saveData.advisors = AdvisorRegistry::getInstance().getAllAdvisors();
        saveData.resources = ResourceRegistry::getInstance().getAllResources();
        saveData.factions = FactionRegistry::getInstance().getAllFactions();
        saveData.events = EventRegistry::getInstance().getAllEvents();
        
        // Save using SaveGameManager
        std::string filePath = "saves/" + fileName + ".dat";
        if (SaveGameManager::instance()->saveGameToBinary(filePath, saveData, false)) {
            m_renderer.displayNotification("✓ Game saved successfully!");
            m_renderer.displayMessage("  File: " + filePath);
            m_renderer.displayMessage("  Entities: " + std::to_string(saveData.npcs.size()) + " NPCs, " +
                                     std::to_string(saveData.advisors.size()) + " Advisors, " +
                                     std::to_string(saveData.resources.size()) + " Resources");
        } else {
            m_renderer.displayError("✗ Failed to save game!");
        }
    }
    
    void loadGame(const std::string& fileName) {
        m_renderer.displayMessage("\nLoading game from " + fileName + "...");
        
        SaveGame saveData;
        std::string filePath = "saves/" + fileName + ".dat";
        
        if (!SaveGameManager::instance()->loadGameFromBinary(filePath, saveData, false)) {
            m_renderer.displayError("✗ Failed to load game!");
            return;
        }
        
        // Apply loaded state
        if (SaveGameManager::instance()->applySaveGameToCurrentState(saveData)) {
            // Update game state from loaded data
            m_state.currentTick = saveData.header.tickNumber;
            m_renderer.displayNotification("✓ Game loaded successfully!");
            m_renderer.displayMessage("  Loaded: " + std::to_string(saveData.npcs.size()) + " NPCs, " +
                                     std::to_string(saveData.advisors.size()) + " Advisors, " +
                                     std::to_string(saveData.resources.size()) + " Resources");
            showStatus();
        } else {
            m_renderer.displayError("✗ Failed to apply loaded game state!");
        }
    }

    void showStatus() {
        m_renderer.displayGameTime(m_state.currentDay, m_state.currentHour,
                                   m_state.currentSeason, m_state.currentYear);
        
        auto& npcReg = NPCRegistry::getInstance();
        auto& factionReg = FactionRegistry::getInstance();
        auto& resourceReg = ResourceRegistry::getInstance();
        
        m_renderer.displayMessage("\nSettlement Overview:");
        m_renderer.displayMessage("  Population: " + std::to_string(npcReg.getNPCCount()));
        m_renderer.displayMessage("  Factions: " + std::to_string(factionReg.getFactionCount()));
        m_renderer.displayMessage("  Resource Types: " + std::to_string(resourceReg.getResourceCount()));
        
        // Settlement reputation
        std::string repDesc;
        if (m_state.settlementReputation >= 0.8f) repDesc = "Renowned";
        else if (m_state.settlementReputation >= 0.6f) repDesc = "Respected";
        else if (m_state.settlementReputation >= 0.4f) repDesc = "Known";
        else if (m_state.settlementReputation >= 0.2f) repDesc = "Obscure";
        else repDesc = "Unknown";
        
        std::stringstream repStream;
        repStream << std::fixed << std::setprecision(0) << (m_state.settlementReputation * 100);
        m_renderer.displayMessage("  Reputation: " + repDesc + " (" + repStream.str() + "%)");
        
        // Average mood and loyalty
        auto npcs = npcReg.getAllNPCs();
        if (!npcs.empty()) {
            float totalMood = 0.0f, totalLoyalty = 0.0f;
            int unhappyCount = 0;
            for (const auto& npc : npcs) {
                totalMood += npc->getShortTermMood();
                totalLoyalty += npc->getLoyalty();
                if (npc->getShortTermMood() < 0.3f) unhappyCount++;
            }
            float avgMood = totalMood / npcs.size();
            float avgLoyalty = totalLoyalty / npcs.size();
            
            std::string moodDesc = avgMood > 0.7f ? "Content" : (avgMood > 0.4f ? "Stable" : "Troubled");
            std::string loyaltyDesc = avgLoyalty > 0.7f ? "Devoted" : (avgLoyalty > 0.4f ? "Loyal" : "Wavering");
            
            m_renderer.displayMessage("  Overall Mood: " + moodDesc);
            m_renderer.displayMessage("  Overall Loyalty: " + loyaltyDesc);
            
            if (unhappyCount > 0) {
                m_renderer.displayNotification(std::to_string(unhappyCount) + " settler(s) are unhappy!");
            }
        }
        
        // Check for critical resources
        auto resources = resourceReg.getAllResources();
        int scarceCount = 0;
        for (const auto& res : resources) {
            if (res->isScarse()) {
                scarceCount++;
            }
        }
        if (scarceCount > 0) {
            m_renderer.displayNotification(std::to_string(scarceCount) + " resource(s) at critical levels!");
        }
        
        // Events counter
        if (m_state.eventsSinceStartup > 0) {
            m_renderer.displayMessage("  Events occurred: " + std::to_string(m_state.eventsSinceStartup));
        }
    }

    void showResources() {
        auto& registry = ResourceRegistry::getInstance();
        m_renderer.displayResources(registry.getAllResources());
    }

    void showFactions() {
        auto& registry = FactionRegistry::getInstance();
        m_renderer.displayFactions(registry.getAllFactions());
    }

    void showPeople() {
        auto& registry = NPCRegistry::getInstance();
        auto npcs = registry.getAllNPCs();
        
        m_renderer.displayTitle("SETTLEMENT POPULATION");
        
        for (const auto& npc : npcs) {
            std::cout << "  " << npc->getName() << " - " << npc->getRole();
            
            // Show mood indicator
            float mood = npc->getShortTermMood();
            if (mood < 0.3f) {
                std::cout << " [Unhappy]";
            } else if (mood > 0.7f) {
                std::cout << " [Content]";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    void showAdvisors() {
        auto& registry = AdvisorRegistry::getInstance();
        auto advisors = registry.getAllAdvisors();
        
        if (advisors.empty()) {
            m_renderer.displayMessage("You have no advisors yet.");
            return;
        }
        
        m_renderer.displayTitle("YOUR ADVISORS");
        
        for (const auto& advisor : advisors) {
            std::string specialty;
            switch (advisor->getSpecialty()) {
                case Specialty::POLITICS: specialty = "Politics"; break;
                case Specialty::MILITARY: specialty = "Military"; break;
                case Specialty::CULTURE: specialty = "Culture"; break;
                case Specialty::RELIGION: specialty = "Religion"; break;
            }
            
            m_renderer.displayAdvisorAdvice(
                advisor->getName(),
                specialty,
                "Ready to provide counsel.",
                advisor->getInfluenceScore()
            );
        }
    }

    void advanceTime(int hours) {
        int daysPassed = 0;
        m_state.currentHour += hours;
        
        while (m_state.currentHour >= 24) {
            m_state.currentHour -= 24;
            m_state.currentDay++;
            daysPassed++;
            
            // Season changes every 90 days
            if (m_state.currentDay > 90) {
                m_state.currentDay = 1;
                
                if (m_state.currentSeason == "Spring") {
                    m_state.currentSeason = "Summer";
                } else if (m_state.currentSeason == "Summer") {
                    m_state.currentSeason = "Autumn";
                } else if (m_state.currentSeason == "Autumn") {
                    m_state.currentSeason = "Winter";
                } else {
                    m_state.currentSeason = "Spring";
                    m_state.currentYear++;
                }
                
                m_renderer.displayNotification("Season changed to " + m_state.currentSeason + "!");
            }
        }
        
        m_renderer.displayMessage("\nTime passes...");
        m_renderer.displayGameTime(m_state.currentDay, m_state.currentHour,
                                   m_state.currentSeason, m_state.currentYear);
        
        // Process simulation updates for each day that passed
        for (int d = 0; d < daysPassed; d++) {
            processResourceConsumption();
            processNPCMoodDecay();
            processImmigration();
            updateNPCProblems();  // Task #2: Update NPC problem detection
            checkForRandomEvents();
        }
        
        // Process hourly updates
        for (int h = 0; h < hours; h++) {
            processHourlyUpdates();
        }
    }
    
    // ========================================================================
    // Simulation Systems - Resource Management
    // ========================================================================
    
    void processResourceConsumption() {
        auto& registry = ResourceRegistry::getInstance();
        auto resources = registry.getAllResources();
        
        std::vector<std::string> warnings;
        std::vector<std::string> depletions;
        
        for (auto& res : resources) {
            int production = res->getProductionRate();
            int consumption = res->getConsumptionRate();
            int netChange = production - consumption;
            
            int oldQty = res->getQuantity();
            int newQty = std::max(0, oldQty + netChange);
            res->setQuantity(newQty);
            
            // Check for scarcity warnings
            if (!res->isScarse() && newQty < res->getScarcityThreshold()) {
                warnings.push_back(res->getName() + " is running low!");
            } else if (newQty == 0 && oldQty > 0) {
                depletions.push_back(res->getName() + " has been depleted!");
            }
        }
        
        // Display warnings
        for (const auto& warn : warnings) {
            m_renderer.displayNotification("⚠ " + warn);
        }
        for (const auto& depl : depletions) {
            m_renderer.displayError("⛔ " + depl);
        }
    }
    
    // ========================================================================
    // Simulation Systems - NPC Mood & Loyalty Updates
    // ========================================================================
    
    void processNPCMoodDecay() {
        auto& registry = NPCRegistry::getInstance();
        auto npcs = registry.getAllNPCs();
        auto& resourceReg = ResourceRegistry::getInstance();
        
        // Check for food scarcity affecting mood
        auto food = resourceReg.getResourceByName("Food");
        bool foodScarce = food && food->isScarse();
        float foodMoodPenalty = foodScarce ? -0.05f : 0.0f;
        
        for (auto& npc : npcs) {
            // Mood naturally decays toward 0.5 (neutral) over time
            float currentMood = npc->getShortTermMood();
            float targetMood = 0.5f;
            float decayRate = 0.02f;  // 2% per day
            
            float newMood = currentMood + (targetMood - currentMood) * decayRate;
            
            // Apply food scarcity penalty
            newMood += foodMoodPenalty;
            
            // Clamp mood to valid range
            newMood = std::max(0.0f, std::min(1.0f, newMood));
            npc->setShortTermMood(newMood);
            
            // Long-term attitude also drifts based on recent mood
            float currentAttitude = npc->getLongTermAttitude();
            float attitudeShift = (newMood - 0.5f) * 0.01f;  // Very slow shift
            float newAttitude = std::max(0.0f, std::min(1.0f, currentAttitude + attitudeShift));
            npc->setLongTermAttitude(newAttitude);
            
            // Update problem severity based on mood
            if (newMood < 0.3f) {
                npc->setProblemSeverity(std::min(1.0f, npc->getProblemSeverity() + 0.1f));
                if (npc->getProblemDescription().empty()) {
                    if (foodScarce) {
                        npc->setProblemDescription("Concerned about food shortage");
                    } else {
                        npc->setProblemDescription("Feeling unhappy");
                    }
                }
            } else if (newMood > 0.7f) {
                npc->setProblemSeverity(std::max(0.0f, npc->getProblemSeverity() - 0.1f));
            }
        }
    }
    
    void processHourlyUpdates() {
        m_state.currentTick++;
        
        // Update settlement reputation based on average NPC loyalty
        auto& npcReg = NPCRegistry::getInstance();
        auto npcs = npcReg.getAllNPCs();
        
        if (!npcs.empty()) {
            float totalLoyalty = 0.0f;
            for (const auto& npc : npcs) {
                totalLoyalty += npc->getLoyalty();
            }
            float avgLoyalty = totalLoyalty / npcs.size();
            
            // Slowly adjust reputation toward average loyalty
            float repDelta = (avgLoyalty - m_state.settlementReputation) * 0.01f;
            m_state.settlementReputation = std::max(0.0f, std::min(1.0f, 
                m_state.settlementReputation + repDelta));
        }
    }
    
    // ========================================================================
    // Simulation Systems - Immigration
    // ========================================================================
    
    void processImmigration() {
        if (m_state.immigrationCooldown > 0) {
            m_state.immigrationCooldown--;
            return;
        }
        
        // Immigration chance based on reputation and resources
        auto& resourceReg = ResourceRegistry::getInstance();
        auto food = resourceReg.getResourceByName("Food");
        
        bool hasFood = food && !food->isScarse();
        float immigrationChance = m_state.settlementReputation * 0.1f;  // Max 10% per day
        
        if (!hasFood) {
            immigrationChance *= 0.2f;  // Much lower if food is scarce
        }
        
        // Random check (simple pseudo-random based on tick)
        float roll = static_cast<float>((m_state.currentTick * 7 + m_state.currentDay * 13) % 100) / 100.0f;
        
        if (roll < immigrationChance) {
            generateImmigrant();
            m_state.immigrationCooldown = 7;  // Wait at least 7 days before next immigrant
        }
    }
    
    void generateImmigrant() {
        auto& npcReg = NPCRegistry::getInstance();
        
        // Generate random immigrant
        static const std::vector<std::string> names = {
            "Thomas", "Sarah", "William", "Emma", "James", "Mary", 
            "Robert", "Elizabeth", "John", "Margaret", "Henry", "Catherine"
        };
        static const std::vector<std::string> roles = {
            "Farmer", "Craftsman", "Laborer", "Merchant", "Hunter"
        };
        
        int nameIdx = (m_state.currentTick + m_state.currentDay) % names.size();
        int roleIdx = (m_state.currentTick * 3 + m_state.currentDay) % roles.size();
        
        auto immigrant = std::make_shared<NPC>(npcReg.getNextId());
        immigrant->setName(names[nameIdx] + " the Newcomer");
        immigrant->setRole(roles[roleIdx]);
        immigrant->setAge(18 + (m_state.currentTick % 30));
        immigrant->setGender(nameIdx % 2 == 0 ? "Male" : "Female");
        immigrant->setLoyalty(0.4f + (m_state.settlementReputation * 0.2f));
        immigrant->setShortTermMood(0.6f);
        immigrant->setLongTermAttitude(0.5f);
        immigrant->setPosition(Vector3(0, 0, 0));
        
        npcReg.registerNPC(immigrant);
        
        m_renderer.displayNotification("🏠 A new settler has arrived: " + immigrant->getName() + " (" + immigrant->getRole() + ")");
        m_state.eventsSinceStartup++;
    }
    
    // ========================================================================
    // Problem System (Task #2)
    // ========================================================================
    
    std::string getMoodString(float mood) {
        if (mood < 0.2f) return "Miserable";
        if (mood < 0.4f) return "Unhappy";
        if (mood < 0.6f) return "Neutral";
        if (mood < 0.8f) return "Content";
        return "Elated";
    }
    
    std::string generateNPCProblemDescription(const std::shared_ptr<NPC>& npc) {
        // Map role + mood/loyalty to issue type
        
        if (npc->getShortTermMood() < 0.3f) {
            if (npc->getRole() == "Farmer") {
                return "Food shortage! We're going hungry.";
            } else if (npc->getRole() == "Warrior" || npc->getRole() == "Hunter") {
                return "Our defenses are inadequate!";
            } else {
                return "I'm deeply concerned about our situation.";
            }
        } else if (npc->getLoyalty() < 0.3f) {
            if (npc->getRole() == "Merchant") {
                return "This settlement isn't meeting my needs.";
            } else {
                return "I'm losing faith in your leadership.";
            }
        } else if (npc->getProblemSeverity() > 0.6f) {
            return "I need to talk to you about something urgent.";
        } else {
            return "I have a concern I'd like to discuss.";
        }
    }
    
    void updateNPCProblems() {
        const auto& npcs = NPCRegistry::getInstance().getAllNPCs();
        const auto& resources = ResourceRegistry::getInstance().getAllResources();
        int currentTick = m_state.currentTick;
        
        // Find food resource
        float foodQty = 0.0f;
        float scarcityThreshold = 150.0f;
        for (const auto& res : resources) {
            if (res->getName() == "Food") {
                foodQty = static_cast<float>(res->getQuantity());
                scarcityThreshold = static_cast<float>(res->getScarcityThreshold());
                break;
            }
        }
        
        for (auto& npc : npcs) {
            // Update problem calculation every tick
            npc->calculateProblemSeverity();
            
            // Check for resolution
            if (npc->getProblemState() == ProblemState::IN_DIALOGUE ||
                npc->getProblemState() == ProblemState::ACKNOWLEDGED) {
                
                if (npc->isProblemResolved(foodQty, scarcityThreshold)) {
                    npc->setProblemState(ProblemState::RESOLVED);
                    npc->recordDialogueTick(currentTick);
                    m_renderer.displayNotification("✓ " + npc->getName() + "'s problem has been resolved!");
                    continue;
                }
            }
            
            // Check for escalation (every 5 game days)
            if (npc->getTicksAtProblem() > 5 * 14400) {
                npc->escalateProblem();
                m_renderer.displayNotification("⚠ " + npc->getName() + "'s concern is escalating!");
            }
            
            // Check if new problem detected
            if (npc->getProblemSeverity() >= 0.3f) {
                if (npc->getProblemState() == ProblemState::UNRESOLVED) {
                    // NPC recognizes problem and begins pathfinding
                    npc->setProblemState(ProblemState::IN_DIALOGUE);
                    
                    // Generate problem description based on NPC state
                    std::string problemDesc = generateNPCProblemDescription(npc);
                    npc->setProblemDescription(problemDesc);
                    
                    // Enqueue NPC for dialogue with priority calculation
                    float severity = npc->getProblemSeverity();
                    float influence = npc->getInfluenceScore();
                    float distance = 10.0f;  // Simplified distance (in real game would be Vector3::distance)
                    DialogueQueue::instance()->enqueue(npc, severity, influence, distance, currentTick);
                    
                    // If no active dialogue, dequeue and start the first one
                    if (!m_activeDialogueNPC && DialogueQueue::instance()->hasQueuedNPCs()) {
                        m_activeDialogueNPC = DialogueQueue::instance()->dequeue();
                        
                        // Display dialogue
                        m_renderer.displayMessage("\n" + std::string(60, '='));
                        m_renderer.displayMessage("[DIALOGUE] " + m_activeDialogueNPC->getName() + " (" + m_activeDialogueNPC->getRole() + ")");
                        m_renderer.displayMessage("  \"" + m_activeDialogueNPC->getProblemDescription() + "\"");
                        m_renderer.displayMessage("  Loyalty: " + std::to_string(static_cast<int>(m_activeDialogueNPC->getLoyalty() * 100)) + "%");
                        m_renderer.displayMessage("  Mood: " + getMoodString(m_activeDialogueNPC->getShortTermMood()));
                        m_renderer.displayMessage("  Problem Severity: " + std::to_string(static_cast<int>(m_activeDialogueNPC->getProblemSeverity() * 100)) + "%");
                        
                        // Display queue status if there are more NPCs waiting
                        if (DialogueQueue::instance()->hasQueuedNPCs()) {
                            m_renderer.displayMessage("\n" + DialogueQueue::instance()->getQueueStatus());
                        }
                        
                        m_renderer.displayMessage("\nWhat will you do? (type your response or 'leave')");
                        m_renderer.displayMessage(std::string(60, '=') + "\n");
                    }
                }
            } else {
                // Problem severity low, reset if in UNRESOLVED state
                if (npc->getProblemState() == ProblemState::UNRESOLVED) {
                    npc->setProblemSeverity(0.0f);
                    npc->resetTicksAtProblem();
                }
            }
            
            // Update tracking counters
            if (npc->getProblemState() == ProblemState::UNRESOLVED ||
                npc->getProblemState() == ProblemState::IN_DIALOGUE) {
                npc->incrementTicksAtProblem();
            }
            
            npc->updatePreviousStats();
        }
    }
    
    void handleDialogueResponse(const std::string& input) {
        if (!m_activeDialogueNPC) {
            return;
        }
        
        if (input == "leave") {
            m_renderer.displayMessage(m_activeDialogueNPC->getName() + ": OK, I'll think about this.\n");
            m_activeDialogueNPC->setProblemState(ProblemState::PERSISTENT);
            
            // Clear active and auto-advance to next queued NPC
            m_activeDialogueNPC = nullptr;
            advanceToNextQueuedNPC();
            return;
        }
        
        // Process decision using interpretWithLLM
        interpretWithLLM(input);
        
        // Update NPC loyalty based on action (will be extracted from interpreted action)
        // For now, modest positive shift for any engagement
        m_activeDialogueNPC->setLoyalty(m_activeDialogueNPC->getLoyalty() + 0.05f);
        m_renderer.displayMessage(m_activeDialogueNPC->getName() + ": I appreciate you listening.\n");
        
        // Mark as ACKNOWLEDGED
        m_activeDialogueNPC->setProblemState(ProblemState::ACKNOWLEDGED);
        m_activeDialogueNPC->recordDialogueTick(m_state.currentTick);
        
        m_renderer.displayNotification("✓ " + m_activeDialogueNPC->getName() + " acknowledged.");
        
        // Clear active and auto-advance to next queued NPC
        m_activeDialogueNPC = nullptr;
        advanceToNextQueuedNPC();
    }
    
    // ========================================================================
    // Dialogue Queue Management - Task #6 (New Method)
    // ========================================================================
    
    void advanceToNextQueuedNPC() {
        if (DialogueQueue::instance()->hasQueuedNPCs()) {
            m_activeDialogueNPC = DialogueQueue::instance()->dequeue();
            
            if (m_activeDialogueNPC) {
                // Display dialogue for next NPC
                m_renderer.displayMessage("\n" + std::string(60, '='));
                m_renderer.displayMessage("[DIALOGUE] " + m_activeDialogueNPC->getName() + " (" + m_activeDialogueNPC->getRole() + ")");
                m_renderer.displayMessage("  \"" + m_activeDialogueNPC->getProblemDescription() + "\"");
                m_renderer.displayMessage("  Loyalty: " + std::to_string(static_cast<int>(m_activeDialogueNPC->getLoyalty() * 100)) + "%");
                m_renderer.displayMessage("  Mood: " + getMoodString(m_activeDialogueNPC->getShortTermMood()));
                m_renderer.displayMessage("  Problem Severity: " + std::to_string(static_cast<int>(m_activeDialogueNPC->getProblemSeverity() * 100)) + "%");
                
                // Display queue status if there are more NPCs waiting
                if (DialogueQueue::instance()->hasQueuedNPCs()) {
                    m_renderer.displayMessage("\n" + DialogueQueue::instance()->getQueueStatus());
                }
                
                m_renderer.displayMessage("\nWhat will you do? (type your response or 'leave')");
                m_renderer.displayMessage(std::string(60, '=') + "\n");
            }
        }
    }

    
    void checkForRandomEvents() {
        // Only check for events once every few days
        if (m_state.currentDay - m_state.lastEventDay < 3) {
            return;
        }
        
        // Random event chance (10% per day after cooldown)
        float roll = static_cast<float>((m_state.currentTick * 11 + m_state.currentDay * 17) % 100) / 100.0f;
        
        if (roll < 0.10f) {
            triggerRandomEvent();
            m_state.lastEventDay = m_state.currentDay;
        }
    }
    
    void triggerRandomEvent() {
        // Select event type based on season and conditions
        int eventType = (m_state.currentTick + m_state.currentDay) % 6;
        
        switch (eventType) {
            case 0:
                triggerWeatherEvent();
                break;
            case 1:
                triggerResourceEvent();
                break;
            case 2:
                triggerSocialEvent();
                break;
            case 3:
                triggerEconomicEvent();
                break;
            case 4:
                triggerMoraleEvent();
                break;
            default:
                triggerMinorEvent();
                break;
        }
        
        m_state.eventsSinceStartup++;
    }
    
    void triggerWeatherEvent() {
        auto& resourceReg = ResourceRegistry::getInstance();
        
        if (m_state.currentSeason == "Winter") {
            m_renderer.displayNotification("❄ A harsh blizzard has struck! Resource production slowed.");
            // Reduce all production rates temporarily
            for (auto& res : resourceReg.getAllResources()) {
                res->setProductionRate(std::max(0, res->getProductionRate() - 2));
            }
        } else if (m_state.currentSeason == "Spring") {
            m_renderer.displayNotification("🌧 Spring rains have blessed the fields! Crop yields increased.");
            auto food = resourceReg.getResourceByName("Food");
            if (food) {
                food->addQuantity(50);
            }
        } else if (m_state.currentSeason == "Summer") {
            m_renderer.displayNotification("☀ A drought threatens the crops!");
            auto water = resourceReg.getResourceByName("Water");
            if (water) {
                water->addQuantity(-30);
            }
        } else {
            m_renderer.displayNotification("🍂 Autumn harvest brings abundance!");
            auto food = resourceReg.getResourceByName("Food");
            if (food) {
                food->addQuantity(80);
            }
        }
    }
    
    void triggerResourceEvent() {
        auto& resourceReg = ResourceRegistry::getInstance();
        auto resources = resourceReg.getAllResources();
        
        if (resources.empty()) return;
        
        int idx = m_state.currentTick % resources.size();
        auto& res = resources[idx];
        
        int roll = m_state.currentDay % 3;
        if (roll == 0) {
            m_renderer.displayNotification("📦 Traders arrive with extra " + res->getName() + "!");
            res->addQuantity(40);
        } else if (roll == 1) {
            m_renderer.displayNotification("🔥 A storage fire destroyed some " + res->getName() + "!");
            res->addQuantity(-25);
            
            // Check for cascade on critical resource loss
            if (res->getName() == "Food" && res->getQuantity() < res->getScarcityThreshold()) {
                m_renderer.displayNotification("\n⚠ CRITICAL: Food has fallen below survival threshold!");
                processCascades(7, "Famine Crisis", m_state.currentTick);
            }
        } else {
            m_renderer.displayNotification("⛏ Workers discovered a new source of " + res->getName() + "!");
            res->setProductionRate(res->getProductionRate() + 1);
        }
    }
    
    void triggerSocialEvent() {
        auto& npcReg = NPCRegistry::getInstance();
        auto npcs = npcReg.getAllNPCs();
        
        if (npcs.empty()) return;
        
        int idx = m_state.currentTick % npcs.size();
        auto& npc = npcs[idx];
        
        int roll = m_state.currentDay % 3;
        if (roll == 0) {
            m_renderer.displayNotification("🎉 " + npc->getName() + " celebrates a personal achievement! Morale boosted.");
            npc->setShortTermMood(std::min(1.0f, npc->getShortTermMood() + 0.2f));
            npc->setLoyalty(std::min(1.0f, npc->getLoyalty() + 0.05f));
        } else if (roll == 1) {
            m_renderer.displayNotification("😔 " + npc->getName() + " is going through a difficult time.");
            npc->setShortTermMood(std::max(0.0f, npc->getShortTermMood() - 0.15f));
            npc->setProblemSeverity(0.4f);
            npc->setProblemDescription("Needs emotional support");
            
            // Check for cascade on severe personal crisis
            if (npc->getShortTermMood() < 0.2f) {
                m_renderer.displayNotification("\n⚠ CRISIS CASCADE: " + npc->getName() + "'s crisis is spreading despair!");
                processCascades(4, "Personal Crisis", m_state.currentTick);
            }
        } else {
            m_renderer.displayNotification("🤝 A friendship forms between settlers. Community bonds strengthen.");
            for (auto& n : npcs) {
                n->setLoyalty(std::min(1.0f, n->getLoyalty() + 0.02f));
            }
        }
    }
    
    void triggerEconomicEvent() {
        auto& resourceReg = ResourceRegistry::getInstance();
        
        int roll = m_state.currentDay % 3;
        if (roll == 0) {
            m_renderer.displayNotification("💰 Traveling merchants offer favorable trade deals!");
            auto gold = resourceReg.getResourceByName("Gold");
            if (gold) {
                gold->addQuantity(30);
            }
        } else if (roll == 1) {
            m_renderer.displayNotification("📉 Economic downturn affects the region. Trade slows.");
            m_state.settlementReputation = std::max(0.0f, m_state.settlementReputation - 0.05f);
            
            // Check for cascade on severe reputation loss
            if (m_state.settlementReputation < 0.2f) {
                m_renderer.displayNotification("\n⚠ WARNING: Settlement reputation has collapsed!");
                processCascades(6, "Economic Crisis", m_state.currentTick);
            }
        } else {
            m_renderer.displayNotification("📈 Your settlement's fame spreads! Reputation increased.");
            m_state.settlementReputation = std::min(1.0f, m_state.settlementReputation + 0.08f);
        }
    }
    
    void triggerMoraleEvent() {
        auto& npcReg = NPCRegistry::getInstance();
        auto npcs = npcReg.getAllNPCs();
        
        int roll = m_state.currentDay % 3;
        if (roll == 0) {
            m_renderer.displayNotification("🎭 A traveling entertainer visits! Everyone's spirits are lifted.");
            for (auto& npc : npcs) {
                npc->setShortTermMood(std::min(1.0f, npc->getShortTermMood() + 0.1f));
            }
        } else if (roll == 1) {
            m_renderer.displayNotification("😰 Rumors of bandits nearby cause unrest.");
            for (auto& npc : npcs) {
                npc->setShortTermMood(std::max(0.0f, npc->getShortTermMood() - 0.08f));
            }
            
            // Check for cascade on severe morale drop
            float avgMood = 0.0f;
            for (const auto& npc : npcs) {
                avgMood += npc->getShortTermMood();
            }
            avgMood /= std::max(1, (int)npcs.size());
            
            if (avgMood < 0.3f) {
                m_renderer.displayNotification("\n⚠ CRITICAL: Settlement morale is catastrophically low!");
                processCascades(5, "Morale Crisis", m_state.currentTick);
            }
        } else {
            m_renderer.displayNotification("🏆 The settlement wins a regional competition! Pride swells.");
            for (auto& npc : npcs) {
                npc->setLoyalty(std::min(1.0f, npc->getLoyalty() + 0.05f));
            }
            m_state.settlementReputation = std::min(1.0f, m_state.settlementReputation + 0.1f);
        }
    }
    
    void triggerMinorEvent() {
        int roll = (m_state.currentTick + m_state.currentDay) % 4;
        
        switch (roll) {
            case 0:
                m_renderer.displayMessage("  A peaceful day passes without incident.");
                break;
            case 1:
                m_renderer.displayMessage("  Birds sing as the settlers go about their work.");
                break;
            case 2:
                m_renderer.displayMessage("  The settlement bustles with daily activity.");
                break;
            default:
                m_renderer.displayMessage("  Life continues its steady rhythm.");
                break;
        }
    }
    
    // ========================================================================
    // Event Cascade System (Task #3)
    // ========================================================================
    
    // Triggers cascade events based on primary event impact
    void processCascades(int primaryEventImpact, const std::string& primaryEventName, int randomSeed) {
        // Calculate cascade probability: P = sigmoid(impact × 0.15)
        float cascadeProb = 1.0f / (1.0f + std::exp(-primaryEventImpact * 0.15f));
        
        // Check if cascade occurs
        float randomRoll = (randomSeed % 1000) / 1000.0f;
        if (randomRoll >= cascadeProb) {
            return;  // No cascade
        }
        
        // Secondary cascade triggered
        triggerSecondaryCascadeEvent(primaryEventName, primaryEventImpact, randomSeed);
    }
    
    void triggerSecondaryCascadeEvent(const std::string& primaryEventName, int primaryImpact, int randomSeed) {
        // Map primary events to secondary cascades
        std::string secondaryEvent;
        std::string description;
        int secondaryImpact = primaryImpact - 2;  // Secondary events slightly less severe
        
        if (primaryEventName.find("Famine") != std::string::npos || 
            primaryEventName.find("food") != std::string::npos) {
            // Food crisis cascades to immigration crisis
            secondaryEvent = "Immigration Crisis";
            description = "⚠ CRISIS CASCADE: Desperate times prompt emigration!\n  Population morale declining rapidly.\n  [Impact: 6]";
            triggerImmigrationCrisisCascade(randomSeed);
        }
        else if (primaryEventName.find("Epidemic") != std::string::npos || 
                 primaryEventName.find("plague") != std::string::npos) {
            // Epidemic cascades to moral crisis
            secondaryEvent = "Moral Crisis";
            description = "⚠ CRISIS CASCADE: The settlement's faith is shaken!\n  Religious doubts spread among believers.\n  [Impact: 5]";
            triggerMoralCrisisCascade(randomSeed);
        }
        else if (primaryEventName.find("Rebellion") != std::string::npos || 
                 primaryEventName.find("rebellion") != std::string::npos) {
            // Rebellion cascades to military conflict
            secondaryEvent = "Military Conflict";
            description = "⚠ CRISIS CASCADE: Armed skirmish erupts!\n  Settlement defenses overwhelmed.\n  [Impact: 7]";
            triggerMilitaryConflictCascade(randomSeed);
        }
        else if (primaryEventName.find("Economic") != std::string::npos) {
            // Economic crisis cascades to social unrest
            secondaryEvent = "Social Unrest";
            description = "⚠ CRISIS CASCADE: Factions are turning against each other!\n  Settlement harmony erodes.\n  [Impact: 6]";
            triggerSocialUnrestCascade(randomSeed);
        }
        else {
            // Generic cascade: something unexpected happens
            secondaryEvent = "Unexpected Complication";
            description = "⚠ CRISIS CASCADE: Events spiral unexpectedly!\n  Settlement reeling from consequences.\n  [Impact: 5]";
        }
        
        m_renderer.displayMessage("\n" + description + "\n");
        
        // Check for tertiary cascade
        float tertiaryProb = 1.0f / (1.0f + std::exp(-secondaryImpact * 0.12f));  // Lower threshold
        float tertiaryRoll = ((randomSeed + 1) % 1000) / 1000.0f;
        
        if (tertiaryRoll < tertiaryProb) {
            triggerTertiaryCascadeEvent(secondaryEvent, secondaryImpact, randomSeed);
        }
    }
    
    void triggerImmigrationCrisisCascade(int randomSeed) {
        auto& npcReg = NPCRegistry::getInstance();
        int npcsToRemove = 2 + (randomSeed % 3);  // 2-4 NPCs leave
        
        auto npcs = npcReg.getAllNPCs();
        if (npcs.size() > 5) {  // Don't remove if too few NPCs
            for (int i = 0; i < npcsToRemove && i < (int)npcs.size(); i++) {
                m_renderer.displayMessage("  ⚠ " + npcs[i]->getName() + " and family have left the settlement!");
            }
        }
    }
    
    void triggerMoralCrisisCascade(int randomSeed) {
        auto& npcReg = NPCRegistry::getInstance();
        auto npcs = npcReg.getAllNPCs();
        
        // Reduce morale of affected NPCs
        for (auto& npc : npcs) {
            if ((randomSeed + npc->getId()) % 3 == 0) {
                float newMood = npc->getShortTermMood() * 0.6f;  // 40% mood reduction
                npc->setShortTermMood(newMood);
                m_renderer.displayMessage("  ⚠ " + npc->getName() + " loses faith...");
            }
        }
    }
    
    void triggerMilitaryConflictCascade(int randomSeed) {
        auto& resourceReg = ResourceRegistry::getInstance();
        auto weapons = resourceReg.getResourceByName("Weapons");
        
        if (weapons) {
            int loss = 5 + (randomSeed % 10);
            weapons->addQuantity(-loss);
            m_renderer.displayMessage("  ⚠ Military conflict: " + std::to_string(loss) + " weapons lost!");
        }
        
        // Reduce morale across all NPCs
        auto& npcReg = NPCRegistry::getInstance();
        auto npcs = npcReg.getAllNPCs();
        for (auto& npc : npcs) {
            npc->setLoyalty(npc->getLoyalty() * 0.8f);
        }
    }
    
    void triggerSocialUnrestCascade(int /* randomSeed */) {
        auto& factionReg = FactionRegistry::getInstance();
        auto factions = factionReg.getAllFactions();
        
        // Reduce loyalty across all factions
        for (auto& faction : factions) {
            for (int memberId : faction->getMemberIds()) {
                auto npc = NPCRegistry::getInstance().getNPCById(memberId);
                if (npc) {
                    npc->setLoyalty(npc->getLoyalty() * 0.75f);
                }
            }
            m_renderer.displayMessage("  ⚠ " + faction->getName() + " faction fractured by discord!");
        }
    }
    
    void triggerTertiaryCascadeEvent(const std::string& secondaryEventName, int /* secondaryImpact */, int randomSeed) {
        std::string tertiaryEvent;
        std::string description;
        
        if (secondaryEventName.find("Immigration") != std::string::npos) {
            // Immigration crisis cascades to faction collapse
            tertiaryEvent = "Faction Breakdown";
            description = "⚠ CRISIS CASCADE (Tertiary): A major faction has collapsed!\n  Trust in leadership shattered.\n  [Impact: 7]";
            triggerFactionCollapseCascade(randomSeed);
        }
        else if (secondaryEventName.find("Moral") != std::string::npos) {
            // Moral crisis cascades to religious schism
            tertiaryEvent = "Religious Schism";
            description = "⚠ CRISIS CASCADE (Tertiary): The settlement is torn by doctrinal conflict!\n  Religious unity broken.\n  [Impact: 6]";
        }
        else if (secondaryEventName.find("Military") != std::string::npos) {
            // Military conflict cascades to settlement takeover
            tertiaryEvent = "Settlement Threat";
            description = "⚠ CRISIS CASCADE (Tertiary): The settlement itself is under threat!\n  Evacuation may be necessary.\n  [Impact: 8]";
        }
        else if (secondaryEventName.find("Social") != std::string::npos) {
            // Social unrest cascades to civil war
            tertiaryEvent = "Civil War";
            description = "⚠ CRISIS CASCADE (Tertiary): Factions are at war!\n  Settlement infrastructure damaged.\n  [Impact: 9]";
            triggerCivilWarCascade(randomSeed);
        }
        else {
            tertiaryEvent = "Settlement Crisis";
            description = "⚠ CRISIS CASCADE (Tertiary): Multiple crises compound!\n  Everything is falling apart!\n  [Impact: 8]";
        }
        
        m_renderer.displayMessage("\n" + description + "\n");
    }
    
    void triggerFactionCollapseCascade(int randomSeed) {
        auto& factionReg = FactionRegistry::getInstance();
        auto factions = factionReg.getAllFactions();
        
        if (!factions.empty()) {
            int factionToRemove = randomSeed % factions.size();
            m_renderer.displayMessage("  ⚠ " + factions[factionToRemove]->getName() + " faction has dissolved!");
        }
    }
    
    void triggerCivilWarCascade(int randomSeed) {
        auto& resourceReg = ResourceRegistry::getInstance();
        auto& npcReg = NPCRegistry::getInstance();
        
        // Massive resource loss
        for (auto& resource : resourceReg.getAllResources()) {
            resource->addQuantity(-resource->getQuantity() / 3);  // Lose 1/3 of each resource
            m_renderer.displayMessage("  ⚠ " + resource->getName() + " stores decimated by conflict!");
        }
        
        // Heavy NPC casualties
        auto npcs = npcReg.getAllNPCs();
        int deaths = std::min((int)npcs.size() / 4, 3 + (randomSeed % 3));
        m_renderer.displayMessage("  ⚠ " + std::to_string(deaths) + " settlers have perished in the conflict!");
    }

    void speakWithNPC(const std::string& npcName) {
        auto& registry = NPCRegistry::getInstance();
        auto npcs = registry.getAllNPCs();
        
        // Find NPC by name (case-insensitive partial match)
        std::string searchName = toLower(npcName);
        std::shared_ptr<NPC> foundNpc = nullptr;
        
        for (const auto& npc : npcs) {
            std::string npcNameLower = toLower(npc->getName());
            if (npcNameLower.find(searchName) != std::string::npos) {
                foundNpc = npc;
                break;
            }
        }
        
        if (!foundNpc) {
            m_renderer.displayError("Could not find anyone named '" + npcName + "'");
            m_renderer.displayMessage("Use 'people' to see who lives in the settlement.");
            return;
        }
        
        m_renderer.displayNPCInfo(*foundNpc);
        
        // Generate dialogue using LLM if available
        if (m_state.llmAvailable) {
            generateNPCDialogue(*foundNpc);
        } else {
            // Basic fallback
            std::string mood = foundNpc->getShortTermMood() > 0.5f ? "content" : "concerned";
            m_renderer.displayDialogue(
                foundNpc->getName(),
                "Greetings, leader. I am " + mood + " today.",
                mood
            );
        }
    }

    void generateNPCDialogue(const NPC& npc) {
        // Build context for LLM
        std::string prompt = "You are " + npc.getName() + ", a " + npc.getRole() + " in a medieval settlement. ";
        prompt += "Generate a brief greeting and share any concerns. ";
        prompt += "Mood level: " + std::to_string(npc.getShortTermMood()) + "/1.0. ";
        prompt += "Loyalty: " + std::to_string(npc.getLoyalty()) + "/1.0. ";
        prompt += "Keep response under 50 words. Speak in first person.";
        
        m_renderer.displayMessage("(Generating response...)");
        
        LLMResponse response = m_llmProvider->callLLM(prompt);
        
        if (response.wasSuccessful && !response.text.empty()) {
            std::string mood = npc.getShortTermMood() > 0.5f ? "content" : "concerned";
            m_renderer.displayDialogue(npc.getName(), response.text, mood);
        } else {
            m_renderer.displayDialogue(
                npc.getName(),
                "Greetings, leader. How may I serve the settlement?",
                "neutral"
            );
        }
    }

    void interpretWithLLM(const std::string& input) {
        m_renderer.displayMessage("(Processing command...)");
        
        // Build context from current world state
        std::vector<std::string> contextNPCs;
        std::vector<std::string> contextFactions;
        std::vector<std::string> contextResources;
        
        // Gather NPC names
        auto& npcReg = NPCRegistry::getInstance();
        for (const auto& npc : npcReg.getAllNPCs()) {
            contextNPCs.push_back(npc->getName());
        }
        
        // Gather faction names
        auto& factionReg = FactionRegistry::getInstance();
        for (const auto& faction : factionReg.getAllFactions()) {
            contextFactions.push_back(faction->getName());
        }
        
        // Gather resource names
        auto& resourceReg = ResourceRegistry::getInstance();
        for (const auto& resource : resourceReg.getAllResources()) {
            contextResources.push_back(resource->getName());
        }
        
        // Use DecisionProcessor to process the input with full world context
        Decision decision = DecisionProcessor::processPlayerInputWithContext(
            input,
            contextNPCs,
            contextFactions,
            contextResources,
            m_llmProvider.get(),
            true  // executeImmediately
        );
        
        // Check if action was recognized and executed
        if (decision.actionType != ActionType::UNKNOWN && decision.confidence > 0.5f) {
            // Build and display the result
            displayDecisionResult(decision);
        } else {
            // Use LLM for interpretation hints when action is ambiguous
            std::string prompt = "You are interpreting player commands for a settlement simulation game. ";
            prompt += "The player said: \"" + input + "\". ";
            prompt += "Available actions: allocate, delegate, negotiate, inspire, suppress, command. ";
            prompt += "Available NPCs: ";
            for (size_t i = 0; i < std::min(contextNPCs.size(), size_t(5)); i++) {
                prompt += contextNPCs[i] + (i < std::min(contextNPCs.size(), size_t(5)) - 1 ? ", " : "");
            }
            prompt += ". Available resources: ";
            for (size_t i = 0; i < std::min(contextResources.size(), size_t(5)); i++) {
                prompt += contextResources[i] + (i < std::min(contextResources.size(), size_t(5)) - 1 ? ", " : "");
            }
            prompt += ". ";
            prompt += "Suggest how the player might rephrase their command to be clearer. Keep under 40 words.";
            
            LLMResponse response = m_llmProvider->callLLM(prompt);
            
            if (response.wasSuccessful && !response.text.empty()) {
                m_renderer.displayNotification("Command not recognized. Suggestion: " + response.text);
            } else {
                m_renderer.displayNotification("Command not recognized: '" + input + "'");
                m_renderer.displayMessage("Try: 'allocate food to farmers', 'inspire alice', 'negotiate with merchants'");
            }
        }
    }
    
    void displayDecisionResult(const Decision& decision) {
        // Get action name
        std::string actionName;
        switch (decision.actionType) {
            case ActionType::ALLOCATE: actionName = "Allocate"; break;
            case ActionType::DELEGATE: actionName = "Delegate"; break;
            case ActionType::NEGOTIATE: actionName = "Negotiate"; break;
            case ActionType::INSPIRE: actionName = "Inspire"; break;
            case ActionType::SUPPRESS: actionName = "Suppress"; break;
            case ActionType::COMMAND: actionName = "Command"; break;
            default: actionName = "Action"; break;
        }
        
        // Build result string
        std::stringstream result;
        result << "[RESULT] " << actionName << " executed";
        
        if (decision.confidence >= 0.9f) {
            result << " (high confidence: " << (int)(decision.confidence * 100) << "%)";
        } else if (decision.confidence >= 0.7f) {
            result << " (confidence: " << (int)(decision.confidence * 100) << "%)";
        }
        result << "\n";
        
        // Show target
        if (decision.targetNPCId >= 0) {
            auto npc = NPCRegistry::getInstance().getNPCById(decision.targetNPCId);
            if (npc) {
                result << "[TARGET] " << npc->getName() << " (" << npc->getRole() << ")\n";
            }
        }
        if (decision.targetFactionId >= 0) {
            auto faction = FactionRegistry::getInstance().getFactionById(decision.targetFactionId);
            if (faction) {
                result << "[TARGET] " << faction->getName() << " faction\n";
            }
        }
        if (decision.targetResourceId >= 0 && decision.targetQuantity > 0) {
            auto resource = ResourceRegistry::getInstance().getResourceById(decision.targetResourceId);
            if (resource) {
                result << "[RESOURCE] " << decision.targetQuantity << " " << resource->getName() << "\n";
            }
        }
        
        // Show impacts
        for (const auto& impact : decision.impacts) {
            if (impact.loyaltyDelta != 0.0f) {
                std::string sign = impact.loyaltyDelta > 0 ? "+" : "";
                result << "[IMPACT] Loyalty " << sign << std::fixed << std::setprecision(2) << impact.loyaltyDelta << "\n";
            }
            if (impact.moodDelta != 0.0f) {
                std::string sign = impact.moodDelta > 0 ? "+" : "";
                result << "[IMPACT] Mood " << sign << std::fixed << std::setprecision(2) << impact.moodDelta << "\n";
            }
        }
        
        // Show tone
        std::string toneStr;
        switch (decision.tone) {
            case ToneType::POSITIVE: toneStr = "Positive"; break;
            case ToneType::NEGATIVE: toneStr = "Negative"; break;
            default: toneStr = "Neutral"; break;
        }
        result << "[TONE] " << toneStr << " (multiplier: " << std::fixed << std::setprecision(2) << decision.toneMultiplier << "x)\n";
        
        // Show narrative if available
        if (!decision.narrativeFlavor.empty()) {
            result << "\n" << decision.narrativeFlavor;
        }
        
        m_renderer.displayActionResult("Your command: " + decision.playerInput, result.str());
    }

    void showLLMStatus() {
        m_renderer.displayTitle("LLM STATUS");
        
        m_renderer.displayMessage("Provider: Ollama");
        m_renderer.displayMessage("Server: " + m_llmConfig.ollamaServerUrl);
        m_renderer.displayMessage("Model: " + m_llmConfig.ollamaModel);
        
        if (m_state.llmAvailable) {
            m_renderer.displayMessage("Status: " + std::string("\033[32mConnected\033[0m"));
            
            // Try to list models
            auto models = m_llmProvider->getAvailableModels();
            if (!models.empty()) {
                m_renderer.displayMessage("\nAvailable models:");
                for (const auto& model : models) {
                    m_renderer.displayMessage("  - " + model);
                }
            }
        } else {
            m_renderer.displayMessage("Status: " + std::string("\033[31mDisconnected\033[0m"));
            m_renderer.displayMessage("\nTo connect:");
            m_renderer.displayMessage("  1. Ensure Ollama is installed");
            m_renderer.displayMessage("  2. Run: ollama serve");
            m_renderer.displayMessage("  3. Pull model: ollama pull " + m_llmConfig.ollamaModel);
        }
        std::cout << std::endl;
    }
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    GameConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--no-color") {
            config.colorOutput = false;
        } else if (arg == "--debug") {
            config.debugMode = true;
        } else if (arg == "--data" && i + 1 < argc) {
            config.dataPath = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            config.llmModel = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Typed Leadership Simulator\n";
            std::cout << "\nUsage: TypedLeadershipGame [options]\n";
            std::cout << "\nOptions:\n";
            std::cout << "  --no-color     Disable color output\n";
            std::cout << "  --debug        Enable debug mode\n";
            std::cout << "  --data <path>  Path to data directory (default: data/)\n";
            std::cout << "  --model <name> Ollama model to use (default: gemma3:12b)\n";
            std::cout << "  -h, --help     Show this help message\n";
            return 0;
        }
    }
    
    // Create and run game
    Game game(config);
    
    if (!game.initialize()) {
        std::cerr << "Failed to initialize game. Exiting.\n";
        return 1;
    }
    
    game.run();
    
    return 0;
}
