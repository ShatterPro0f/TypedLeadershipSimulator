#include "LLM.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cmath>

namespace TLS {

// ============================================================================
// LocalLLaMAProvider (Ollama Support)
// ============================================================================

LocalLLaMAProvider::LocalLLaMAProvider(const std::string& apiEndpoint,
                                     float decisionTemp, float narrativeTemp)
    : apiEndpoint_(apiEndpoint), decisionTemperature_(decisionTemp),
      narrativeTemperature_(narrativeTemp)
{
    // Test connection to Ollama endpoint
    isAvailable_ = false;  // TODO: Test connection with simple HTTP request
    // For now, assume available if endpoint specified
    if (!apiEndpoint_.empty() && apiEndpoint_.find("localhost") != std::string::npos) {
        isAvailable_ = true;  // Optimistic - will fail gracefully if offline
    }
}

LLMResponse LocalLLaMAProvider::callLLM(const std::string& prompt, float temperature)
{
    if (!isAvailable()) {
        LLMResponse response;
        response.success = false;
        response.error = "LocalLLaMA provider not available";
        return response;
    }

    return makeHTTPRequest(prompt, temperature);
}

bool LocalLLaMAProvider::isAvailable() const
{
    return isAvailable_;
}

LLMResponse LocalLLaMAProvider::makeHTTPRequest(const std::string& prompt, float temperature)
{
    // Ollama API endpoint: POST /api/generate
    // Request body: { "model": "mistral", "prompt": "...", "temperature": 0.7, "stream": false }
    // Response: { "response": "...", "done": true }
    
    // TODO: Implement with proper HTTP library (e.g., libcurl)
    // For now, return fallback response structure
    
    LLMResponse response;
    response.success = true;
    response.content = "[Ollama Response - HTTP implementation pending]\nPrompt: " + prompt;
    response.inputTokens = estimateTokens(prompt);
    response.completionTokens = estimateTokens(response.content);
    response.duration = std::chrono::milliseconds(100);  // Simulated
    
    return response;
}

// ============================================================================
// OpenAIProvider Implementation
// ============================================================================

OpenAIProvider::OpenAIProvider(const std::string& apiKey, float decisionTemp, float narrativeTemp)
    : apiKey_(apiKey), decisionTemperature_(decisionTemp), narrativeTemperature_(narrativeTemp)
{
    // Check if API key is set
    isAvailable_ = !apiKey_.empty() && apiKey_ != "sk-";
}

LLMResponse OpenAIProvider::callLLM(const std::string& prompt, float temperature)
{
    if (!isAvailable()) {
        LLMResponse response;
        response.success = false;
        response.error = "OpenAI API key not configured";
        return response;
    }

    return makeHTTPRequest(prompt, temperature);
}

bool OpenAIProvider::isAvailable() const
{
    return isAvailable_;
}

LLMResponse OpenAIProvider::makeHTTPRequest(const std::string& prompt, float temperature)
{
    // OpenAI API endpoint: https://api.openai.com/v1/chat/completions
    // TODO: Implement with libcurl
    
    LLMResponse response;
    response.success = false;
    response.error = "OpenAI HTTP implementation not yet implemented (use local Ollama instead)";
    
    return response;
}

float OpenAIProvider::calculateCostUSD(int inputTokens, int completionTokens) const
{
    // GPT-3.5-turbo pricing (as of training data cutoff)
    const float INPUT_COST_PER_1K = 0.003f;
    const float OUTPUT_COST_PER_1K = 0.004f;
    
    float inputCost = (inputTokens / 1000.0f) * INPUT_COST_PER_1K;
    float outputCost = (completionTokens / 1000.0f) * OUTPUT_COST_PER_1K;
    
    return inputCost + outputCost;
}

// ============================================================================
// OfflineFallbackProvider Implementation
// ============================================================================

OfflineFallbackProvider::OfflineFallbackProvider()
{
    // Always available - no external dependencies
}

LLMResponse OfflineFallbackProvider::callLLM(const std::string& prompt, float temperature)
{
    LLMResponse response;
    response.success = true;
    response.duration = std::chrono::milliseconds(10);  // Instant
    
    // Route to appropriate handler based on prompt keywords
    if (prompt.find("interpret") != std::string::npos ||
        prompt.find("decision") != std::string::npos ||
        prompt.find("action") != std::string::npos) {
        response.content = generateOfflineDecisionResponse(prompt);
    } else {
        response.content = generateOfflineNarrative(prompt);
    }
    
    response.inputTokens = estimateTokens(prompt);
    response.completionTokens = estimateTokens(response.content);
    
    return response;
}

std::string OfflineFallbackProvider::generateOfflineDecisionResponse(const std::string& prompt)
{
    // Extract action from prompt (simple keyword matching)
    std::string action = "allocate";
    
    if (prompt.find("inspire") != std::string::npos) action = "inspire";
    if (prompt.find("suppress") != std::string::npos) action = "suppress";
    if (prompt.find("negotiate") != std::string::npos) action = "negotiate";
    if (prompt.find("delegate") != std::string::npos) action = "delegate";
    
    // Return structured decision
    std::ostringstream oss;
    oss << "{\n"
        << "  \"action\": \"" << action << "\",\n"
        << "  \"tone\": \"neutral\",\n"
        << "  \"target_type\": \"faction\",\n"
        << "  \"priority\": \"medium\",\n"
        << "  \"confidence\": 0.65\n"
        << "}";
    
    return oss.str();
}

std::string OfflineFallbackProvider::generateOfflineNarrative(const std::string& prompt)
{
    // Generate templated narrative responses based on prompt keywords
    std::vector<std::string> narratives = {
        "The settlement faces a critical shortage of food supplies. Farmers report poor harvests.",
        "Tensions rise between the merchant faction and warrior faction over resource distribution.",
        "A charismatic leader emerges among the priests, gathering followers for a new movement.",
        "Several families express interest in immigrating to the settlement.",
        "Religious schism threatens: orthodox vs reformist doctrines clash.",
        "Economic opportunity: trade caravan arriving from neighboring settlements.",
        "Cultural shift detected: younger generation questions established traditions.",
        "Diplomatic crisis: neighboring faction sends delegation with demands."
    };
    
    // Select based on prompt hash for determinism
    size_t hash = std::hash<std::string>{}(prompt);
    return narratives[hash % narratives.size()];
}

// ============================================================================
// LLM Base Provider Methods
// ============================================================================

int LLMProvider::estimateTokens(const std::string& text) const
{
    // Rough approximation: ~1 token per 4 characters (OpenAI's estimate)
    return static_cast<int>(text.length() / 4.0f) + 1;
}

// ============================================================================
// LLMManager Implementation
// ============================================================================

LLMManager* LLMManager::instance()
{
    static LLMManager mgr;
    return &mgr;
}

bool LLMManager::initialize(const LLMConfig& config)
{
    config_ = config;
    
    // Create appropriate provider based on config
    if (config.provider == LLMConfig::LOCAL_LLAMA) {
        provider_ = std::make_shared<LocalLLaMAProvider>(
            config.apiEndpoint,
            config.temperatureDecisionInterpretation,
            config.temperatureNarrativeGeneration
        );
    } else if (config.provider == LLMConfig::OPENAI) {
        provider_ = std::make_shared<OpenAIProvider>(
            config.apiKey,
            config.temperatureDecisionInterpretation,
            config.temperatureNarrativeGeneration
        );
    } else {
        // Fallback to offline provider
        provider_ = std::make_shared<OfflineFallbackProvider>();
    }
    
    return provider_ != nullptr;
}

void LLMManager::shutdown()
{
    clearCache();
    provider_ = nullptr;
}

LLMResponse LLMManager::interpretPlayerDecision(const std::string& playerInput, const std::string& context)
{
    if (!provider_) {
        LLMResponse response;
        response.success = false;
        response.error = "LLM provider not initialized";
        return response;
    }
    
    std::string prompt = buildDecisionInterpretationPrompt(playerInput, context);
    float temp = config_.temperatureDecisionInterpretation;
    
    return provider_->callLLM(prompt, temp);
}

LLMResponse LLMManager::generateNarrative(const std::string& worldStateContext)
{
    if (!provider_) {
        LLMResponse response;
        response.success = false;
        response.error = "LLM provider not initialized";
        return response;
    }
    
    std::string prompt = "Generate a brief narrative about the settlement based on: " + worldStateContext;
    float temp = config_.temperatureNarrativeGeneration;
    
    return provider_->callLLM(prompt, temp);
}

LLMResponse LLMManager::generateNPCConversation(int npcId1, int npcId2, const std::string& context)
{
    if (!provider_) {
        LLMResponse response;
        response.success = false;
        response.error = "LLM provider not initialized";
        return response;
    }
    
    std::string prompt = buildNPCConversationPrompt(npcId1, npcId2, "general", "settlement");
    float temp = config_.temperatureNarrativeGeneration;
    
    return provider_->callLLM(prompt, temp);
}

void LLMManager::interpretPlayerDecisionAsync(const std::string& playerInput, const std::string& context,
                                            std::function<void(const LLMResponse&)> callback)
{
    // TODO: Queue and process asynchronously
    // For now, call synchronously
    auto response = interpretPlayerDecision(playerInput, context);
    if (callback) {
        callback(response);
    }
}

void LLMManager::generateNarrativeAsync(const std::string& worldStateContext,
                                       std::function<void(const LLMResponse&)> callback)
{
    // TODO: Queue and process asynchronously
    auto response = generateNarrative(worldStateContext);
    if (callback) {
        callback(response);
    }
}

void LLMManager::generateNPCConversationAsync(int npcId1, int npcId2, const std::string& context,
                                            std::function<void(const LLMResponse&)> callback)
{
    // TODO: Queue and process asynchronously
    auto response = generateNPCConversation(npcId1, npcId2, context);
    if (callback) {
        callback(response);
    }
}

void LLMManager::recordUsage(const LLMUsage& usage)
{
    usageLog_.push_back(usage);
}

float LLMManager::getTotalCostUSD() const
{
    float total = 0.0f;
    for (const auto& usage : usageLog_) {
        total += usage.costUSD;
    }
    return total;
}

std::string LLMManager::getCachedResponse(const std::string& promptHash)
{
    auto it = responseCache_.find(promptHash);
    if (it != responseCache_.end()) {
        return it->second;
    }
    return "";
}

void LLMManager::cacheResponse(const std::string& promptHash, const std::string& response)
{
    if (config_.enableCaching) {
        responseCache_[promptHash] = response;
    }
}

void LLMManager::clearCache()
{
    responseCache_.clear();
}

void LLMManager::recordLLMCall(int tick, const LLMRequest& request, const LLMResponse& response)
{
    // For replay system
    std::pair<int, std::string> key = {tick, request.callType};
    replayLog_[key] = response;
}

LLMResponse LLMManager::replayLLMCall(int tick, const std::string& callType)
{
    std::pair<int, std::string> key = {tick, callType};
    auto it = replayLog_.find(key);
    if (it != replayLog_.end()) {
        return it->second;
    }
    
    LLMResponse response;
    response.success = false;
    response.error = "No replay data for this tick/callType";
    return response;
}

void LLMManager::processQueue()
{
    // TODO: Process queued async requests
}

std::string LLMManager::hashPrompt(const std::string& prompt)
{
    // Simple hash for caching (not cryptographic)
    size_t hash = std::hash<std::string>{}(prompt);
    return std::to_string(hash);
}

// ============================================================================
// Helper Functions for Prompt Construction
// ============================================================================

std::string buildWorldStateContext(const WorldStateSnapshot& snapshot)
{
    std::ostringstream oss;
    oss << "World State (Tick " << snapshot.tickNumber << "):\n";
    oss << "- Significant NPCs: " << snapshot.significantNPCIds.size() << "\n";
    oss << "- Affected Factions: " << snapshot.affectedFactionIds.size() << "\n";
    oss << "- Changed Resources: " << snapshot.changedResourceIds.size() << "\n";
    oss << "- Triggered Events: " << snapshot.triggeredEventIds.size() << "\n";
    oss << "- New Immigrants: " << snapshot.immigrantNPCIds.size() << "\n";
    return oss.str();
}

std::string buildDecisionInterpretationPrompt(const std::string& playerInput, const std::string& worldContext)
{
    std::ostringstream oss;
    oss << "You are an AI advisor in a medieval settlement simulation. "
        << "The player has made a decision. Interpret it as a structured action.\n\n"
        << "World Context:\n" << worldContext << "\n\n"
        << "Player Decision: \"" << playerInput << "\"\n\n"
        << "Respond with JSON containing: action, tone, target_type, priority, confidence\n"
        << "Valid actions: allocate, inspire, suppress, negotiate, delegate\n"
        << "Tones: positive, neutral, negative, aggressive\n"
        << "Target types: npc, faction, resource, culture\n";
    return oss.str();
}

std::string buildNarrativeGenerationPrompt(const WorldStateSnapshot& snapshot)
{
    std::ostringstream oss;
    oss << "You are a narrative generator for a settlement simulation. "
        << "Generate 2-3 emergent narrative issues based on the current world state.\n\n"
        << buildWorldStateContext(snapshot)
        << "\n\nGenerate plausible story hooks and crises for the player to address.";
    return oss.str();
}

std::string buildNPCConversationPrompt(int npcId1, int npcId2, const std::string& topic, const std::string& location)
{
    std::ostringstream oss;
    oss << "Two NPCs (ID: " << npcId1 << " and " << npcId2 << ") are conversing about " << topic
        << " in the " << location << ".\n"
        << "Generate a brief, natural dialogue (2-3 exchanges) between them.\n"
        << "Keep it immersive and consistent with settlement life.\n";
    return oss.str();
}

// ============================================================================
// WorldStateSnapshot Implementation
// ============================================================================

std::string WorldStateSnapshot::toPromptContext() const
{
    return buildWorldStateContext(*this);
}

// ============================================================================
// LLMConfig Implementation
// ============================================================================

LLMConfig LLMConfig::loadFromEnvironment()
{
    LLMConfig config;
    
    // Check for Ollama endpoint
    const char* ollamaEndpoint = std::getenv("OLLAMA_ENDPOINT");
    if (ollamaEndpoint) {
        config.provider = LOCAL_LLAMA;
        config.apiEndpoint = std::string(ollamaEndpoint);
    } else {
        // Default to Ollama localhost
        config.provider = LOCAL_LLAMA;
        config.apiEndpoint = "http://localhost:11434";
    }
    
    return config;
}

LLMConfig LLMConfig::loadFromFile(const std::string& configPath)
{
    // TODO: Load from JSON config file
    return loadFromEnvironment();
}

}  // namespace TLS
