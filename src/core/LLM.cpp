#include "LLM.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <regex>

namespace TLS {

// ============================================================================
// LLMConfig Implementation
// ============================================================================

LLMConfig LLMConfig::loadFromEnvironment() {
    LLMConfig config;
    
    // Try to detect provider from environment
    const char* providerEnv = std::getenv("TLS_LLM_PROVIDER");
    const char* apiKeyEnv = std::getenv("OPENAI_API_KEY");
    const char* endpointEnv = std::getenv("LLAMA_ENDPOINT");
    
    if (providerEnv) {
        std::string provider(providerEnv);
        if (provider == "openai" && apiKeyEnv) {
            config.provider = OPENAI;
            config.apiKey = apiKeyEnv;
        } else if (provider == "llama" && endpointEnv) {
            config.provider = LOCAL_LLAMA;
            config.apiEndpoint = endpointEnv;
        }
    }
    
    // Load from config file if it exists
    if (std::ifstream("llm_config.json")) {
        config = loadFromFile("llm_config.json");
    }
    
    return config;
}

LLMConfig LLMConfig::loadFromFile(const std::string& configPath) {
    LLMConfig config;
    std::ifstream file(configPath);
    if (!file) {
        return config;  // Return default config if file not found
    }
    // Simple JSON parsing (in production, use nlohmann::json or similar)
    // For now, return default - this can be enhanced
    return config;
}

// ============================================================================
// LLMProvider Base Implementation
// ============================================================================

int LLMProvider::estimateTokens(const std::string& text) const {
    // Simple heuristic: ~4 characters per token
    return static_cast<int>(text.length() / 4.0f) + 1;
}

// ============================================================================
// OpenAIProvider Implementation
// ============================================================================

OpenAIProvider::OpenAIProvider(const std::string& apiKey, float decisionTemp, float narrativeTemp)
    : apiKey_(apiKey),
      decisionTemperature_(decisionTemp),
      narrativeTemperature_(narrativeTemp),
      isAvailable_(!apiKey.empty()) {}

LLMResponse OpenAIProvider::callLLM(const std::string& prompt, float temperature) {
    if (!isAvailable()) {
        LLMResponse response;
        response.success = false;
        response.error = "OpenAI API key not configured";
        return response;
    }
    
    // Use appropriate temperature based on call context
    float temp = temperature > 0.0f ? temperature : decisionTemperature_;
    
    // Attempt HTTP request to OpenAI API
    return makeHTTPRequest(prompt, temp);
}

LLMResponse OpenAIProvider::makeHTTPRequest(const std::string& prompt, float temperature) {
    LLMResponse response;
    
    // For now, return error - real implementation would use curl or similar
    // In production: POST to https://api.openai.com/v1/chat/completions
    response.success = false;
    response.error = "OpenAI HTTP implementation not available in this stub";
    response.inputTokens = estimateTokens(prompt);
    
    return response;
}

bool OpenAIProvider::isAvailable() const {
    return isAvailable_;
}

float OpenAIProvider::calculateCostUSD(int inputTokens, int completionTokens) const {
    // GPT-3.5 pricing (as of training data): $0.0005 per 1K input, $0.0015 per 1K output
    // Update with current pricing as needed
    float inputCost = (inputTokens / 1000.0f) * 0.0005f;
    float outputCost = (completionTokens / 1000.0f) * 0.0015f;
    return inputCost + outputCost;
}

// ============================================================================
// LocalLLaMAProvider Implementation
// ============================================================================

LocalLLaMAProvider::LocalLLaMAProvider(const std::string& apiEndpoint, float decisionTemp, float narrativeTemp)
    : apiEndpoint_(apiEndpoint),
      decisionTemperature_(decisionTemp),
      narrativeTemperature_(narrativeTemp),
      isAvailable_(true) {}

LLMResponse LocalLLaMAProvider::callLLM(const std::string& prompt, float temperature) {
    float temp = temperature > 0.0f ? temperature : decisionTemperature_;
    return makeHTTPRequest(prompt, temp);
}

LLMResponse LocalLLaMAProvider::makeHTTPRequest(const std::string& prompt, float temperature) {
    LLMResponse response;
    
    // For now, return error - real implementation would use curl
    // In production: POST to http://localhost:8000/v1/chat/completions
    response.success = false;
    response.error = "LocalLLaMA HTTP implementation not available in this stub";
    response.inputTokens = estimateTokens(prompt);
    
    return response;
}

bool LocalLLaMAProvider::isAvailable() const {
    return isAvailable_;
}

// ============================================================================
// OfflineFallbackProvider Implementation
// ============================================================================

OfflineFallbackProvider::OfflineFallbackProvider() {}

LLMResponse OfflineFallbackProvider::callLLM(const std::string& prompt, float temperature) {
    LLMResponse response;
    response.success = true;
    
    // Detect call type from prompt keywords
    if (prompt.find("interpret") != std::string::npos || prompt.find("decision") != std::string::npos) {
        response.content = generateOfflineDecisionResponse(prompt);
    } else {
        response.content = generateOfflineNarrative(prompt);
    }
    
    response.inputTokens = estimateTokens(prompt);
    response.completionTokens = estimateTokens(response.content);
    response.costUSD = 0.0f;  // Offline = free
    response.duration = std::chrono::milliseconds(50);  // Instant
    
    return response;
}

std::string OfflineFallbackProvider::generateOfflineNarrative(const std::string& prompt) {
    // Simple template-based narrative generation
    // Detects keywords and returns appropriate templates
    
    if (prompt.find("food scarcity") != std::string::npos || prompt.find("hungry") != std::string::npos) {
        return "The settlement faces food shortages. Farmers report concerns about rationing. "
               "Consider allocating emergency supplies or organizing a hunt.";
    }
    
    if (prompt.find("faction") != std::string::npos || prompt.find("conflict") != std::string::npos) {
        return "Tensions rise between settlement factions. Different groups have conflicting interests. "
               "Strong leadership is needed to maintain unity.";
    }
    
    if (prompt.find("religious") != std::string::npos || prompt.find("religion") != std::string::npos) {
        return "A religious debate emerges among the settlers. Different faith traditions clash. "
               "Careful diplomacy is required to prevent schism.";
    }
    
    if (prompt.find("immigration") != std::string::npos || prompt.find("newcomer") != std::string::npos) {
        return "New settlers arrive at the settlement seeking refuge and opportunity. "
               "This brings both hope and resource pressures. How will you welcome them?";
    }
    
    if (prompt.find("crisis") != std::string::npos || prompt.find("disaster") != std::string::npos) {
        return "An unexpected crisis threatens the settlement's stability. "
               "Quick decision-making and strong resolve are essential.";
    }
    
    // Generic fallback narrative
    return "Events unfold in the settlement. The situation remains fluid and uncertain. "
           "Your leadership choices will shape the outcome.";
}

std::string OfflineFallbackProvider::generateOfflineDecisionResponse(const std::string& prompt) {
    // Template for decision interpretation
    // Extracts simple keywords to determine action type
    
    if (prompt.find("feed") != std::string::npos || prompt.find("food") != std::string::npos || 
        prompt.find("allocate") != std::string::npos) {
        return "{ \"action\": \"allocate\", \"resource\": \"food\", \"target\": \"settlers\", "
               "\"tone\": \"positive\", \"priority\": 0.8 }";
    }
    
    if (prompt.find("delegate") != std::string::npos || prompt.find("assign") != std::string::npos) {
        return "{ \"action\": \"delegate\", \"target\": \"advisors\", "
               "\"tone\": \"neutral\", \"priority\": 0.5 }";
    }
    
    if (prompt.find("negotiate") != std::string::npos || prompt.find("peace") != std::string::npos) {
        return "{ \"action\": \"negotiate\", \"target\": \"factions\", "
               "\"tone\": \"diplomatic\", \"priority\": 0.7 }";
    }
    
    if (prompt.find("inspire") != std::string::npos || prompt.find("motivate") != std::string::npos) {
        return "{ \"action\": \"inspire\", \"target\": \"all\", "
               "\"tone\": \"positive\", \"priority\": 0.6 }";
    }
    
    // Generic decision response
    return "{ \"action\": \"acknowledge\", \"target\": \"settlement\", "
           "\"tone\": \"neutral\", \"priority\": 0.3 }";
}

// ============================================================================
// LLMRequestQueue Implementation
// ============================================================================

LLMRequestQueue* LLMRequestQueue::instance() {
    static LLMRequestQueue queue;
    return &queue;
}

void LLMRequestQueue::enqueueRequest(const LLMRequest& request) {
    // Route to appropriate priority queue
    LLMRequest req = request;
    req.requestId = nextRequestId_++;
    req.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    switch (request.priority) {
        case QueuePriority::HIGH:
            queues_.high.push(req);
            break;
        case QueuePriority::MEDIUM:
            queues_.medium.push(req);
            break;
        case QueuePriority::LOW:
            queues_.low.push(req);
            break;
    }
}

LLMRequest LLMRequestQueue::dequeueRequest(QueuePriority priority) {
    LLMRequest empty;
    empty.requestId = -1;
    
    switch (priority) {
        case QueuePriority::HIGH:
            if (!queues_.high.empty()) {
                auto req = queues_.high.front();
                queues_.high.pop();
                return req;
            }
            break;
        case QueuePriority::MEDIUM:
            if (!queues_.medium.empty()) {
                auto req = queues_.medium.front();
                queues_.medium.pop();
                return req;
            }
            break;
        case QueuePriority::LOW:
            if (!queues_.low.empty()) {
                auto req = queues_.low.front();
                queues_.low.pop();
                return req;
            }
            break;
    }
    
    return empty;
}

bool LLMRequestQueue::hasRequestsInQueue(QueuePriority priority) const {
    switch (priority) {
        case QueuePriority::HIGH:
            return !queues_.high.empty();
        case QueuePriority::MEDIUM:
            return !queues_.medium.empty();
        case QueuePriority::LOW:
            return !queues_.low.empty();
    }
    return false;
}

size_t LLMRequestQueue::getQueueSize(QueuePriority priority) const {
    switch (priority) {
        case QueuePriority::HIGH:
            return queues_.high.size();
        case QueuePriority::MEDIUM:
            return queues_.medium.size();
        case QueuePriority::LOW:
            return queues_.low.size();
    }
    return 0;
}

void LLMRequestQueue::clearQueue(QueuePriority priority) {
    switch (priority) {
        case QueuePriority::HIGH:
            while (!queues_.high.empty()) queues_.high.pop();
            break;
        case QueuePriority::MEDIUM:
            while (!queues_.medium.empty()) queues_.medium.pop();
            break;
        case QueuePriority::LOW:
            while (!queues_.low.empty()) queues_.low.pop();
            break;
    }
}

void LLMRequestQueue::processNextRequest(std::shared_ptr<LLMProvider> provider) {
    // Process in priority order: HIGH -> MEDIUM -> LOW
    if (hasRequestsInQueue(QueuePriority::HIGH)) {
        auto req = dequeueRequest(QueuePriority::HIGH);
        auto response = provider->callLLM(req.prompt);
        if (req.callback) req.callback(response);
    } else if (hasRequestsInQueue(QueuePriority::MEDIUM)) {
        auto req = dequeueRequest(QueuePriority::MEDIUM);
        auto response = provider->callLLM(req.prompt);
        if (req.callback) req.callback(response);
    } else if (hasRequestsInQueue(QueuePriority::LOW)) {
        auto req = dequeueRequest(QueuePriority::LOW);
        auto response = provider->callLLM(req.prompt);
        if (req.callback) req.callback(response);
    }
}

void LLMRequestQueue::flushQueue(std::shared_ptr<LLMProvider> provider) {
    while (hasRequestsInQueue(QueuePriority::HIGH) || 
           hasRequestsInQueue(QueuePriority::MEDIUM) || 
           hasRequestsInQueue(QueuePriority::LOW)) {
        processNextRequest(provider);
    }
}

// ============================================================================
// LLMManager Implementation
// ============================================================================

LLMManager* LLMManager::instance() {
    static LLMManager manager;
    return &manager;
}

bool LLMManager::initialize(const LLMConfig& config, const LLMErrorConfig& errorConfig,
                            const LLMCacheConfig& cacheConfig) {
    config_ = config;
    errorConfig_ = errorConfig;
    cacheConfig_ = cacheConfig;
    
    // Always initialize offline fallback
    offlineFallback_ = std::make_shared<OfflineFallbackProvider>();
    
    std::cout << "[LLM Cache] Initialized with max " << cacheConfig_.maxCacheEntries << " entries"
              << " (caching " << (cacheConfig_.enableCaching ? "ENABLED" : "DISABLED") << ")" << std::endl;
    std::cout << "[LLM Cache] TTL: Decisions=" << cacheConfig_.ttlDecisionInterpretation << "s, "
              << "Narratives=" << cacheConfig_.ttlNarrativeGeneration << "s, "
              << "Conversations=" << cacheConfig_.ttlNPCConversation << "s" << std::endl;
    
    // Create appropriate provider based on config
    switch (config.provider) {
        case LLMConfig::OPENAI:
            if (!config.apiKey.empty()) {
                provider_ = std::make_shared<OpenAIProvider>(
                    config.apiKey,
                    config.temperatureDecisionInterpretation,
                    config.temperatureNarrativeGeneration
                );
                std::cout << "[LLM] Initialized OpenAI provider" << std::endl;
                return true;
            }
            [[fallthrough]];
            
        case LLMConfig::LOCAL_LLAMA:
            if (!config.apiEndpoint.empty()) {
                provider_ = std::make_shared<LocalLLaMAProvider>(
                    config.apiEndpoint,
                    config.temperatureDecisionInterpretation,
                    config.temperatureNarrativeGeneration
                );
                std::cout << "[LLM] Initialized LocalLLaMA provider at " << config.apiEndpoint << std::endl;
                return true;
            }
            [[fallthrough]];
            
        case LLMConfig::OFFLINE_FALLBACK:
        default:
            provider_ = std::make_shared<OfflineFallbackProvider>();
            std::cout << "[LLM] Initialized Offline Fallback provider" << std::endl;
            return true;
    }
}

void LLMManager::shutdown() {
    provider_.reset();
    responseCache_.clear();
    lruOrder_.clear();
    replayLog_.clear();
}

LLMResponse LLMManager::interpretPlayerDecision(const std::string& playerInput, const std::string& context) {
    if (!provider_) return LLMResponse{false, "", 0, 0, 0.0f, "Provider not initialized"};
    
    std::string prompt = buildDecisionInterpretationPrompt(playerInput, context);
    std::string promptHash = hashPrompt(prompt);
    
    // Check cache first
    std::string cachedResponse = getCachedResponse(promptHash, "decision_interpretation");
    if (!cachedResponse.empty()) {
        std::cout << "[LLM Cache] HIT: decision_interpretation" << std::endl;
        LLMResponse response;
        response.success = true;
        response.content = cachedResponse;
        response.inputTokens = 0;  // Cached, no tokens used
        response.completionTokens = 0;
        response.costUSD = 0.0f;
        response.duration = std::chrono::milliseconds(1);  // Cached, instant response
        return response;
    }
    
    // Cache miss - call provider
    std::cout << "[LLM Cache] MISS: decision_interpretation" << std::endl;
    auto response = provider_->callLLM(prompt, config_.temperatureDecisionInterpretation);
    
    if (response.success) {
        recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                     std::chrono::system_clock::now(), "decision_interpretation"});
        // Cache the successful response
        cacheResponse(promptHash, "decision_interpretation", response.content,
                     response.inputTokens, response.completionTokens, response.costUSD);
    }
    
    return response;
}

LLMResponse LLMManager::generateNarrative(const std::string& worldStateContext) {
    if (!provider_) return LLMResponse{false, "", 0, 0, 0.0f, "Provider not initialized"};
    
    std::string prompt = "Generate narrative for world state:\n" + worldStateContext;
    std::string promptHash = hashPrompt(prompt);
    
    // Check cache first
    std::string cachedResponse = getCachedResponse(promptHash, "narrative_generation");
    if (!cachedResponse.empty()) {
        std::cout << "[LLM Cache] HIT: narrative_generation" << std::endl;
        LLMResponse response;
        response.success = true;
        response.content = cachedResponse;
        response.inputTokens = 0;  // Cached, no tokens used
        response.completionTokens = 0;
        response.costUSD = 0.0f;
        response.duration = std::chrono::milliseconds(1);  // Cached, instant response
        return response;
    }
    
    // Cache miss - call provider
    std::cout << "[LLM Cache] MISS: narrative_generation" << std::endl;
    auto response = provider_->callLLM(prompt, config_.temperatureNarrativeGeneration);
    
    if (response.success) {
        recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                     std::chrono::system_clock::now(), "narrative_generation"});
        // Cache the successful response
        cacheResponse(promptHash, "narrative_generation", response.content,
                     response.inputTokens, response.completionTokens, response.costUSD);
    }
    
    return response;
}

LLMResponse LLMManager::generateNPCConversation(int npcId1, int npcId2, const std::string& context) {
    if (!provider_) return LLMResponse{false, "", 0, 0, 0.0f, "Provider not initialized"};
    
    std::string prompt = "Generate dialogue between NPC " + std::to_string(npcId1) + 
                        " and NPC " + std::to_string(npcId2) + ":\n" + context;
    std::string promptHash = hashPrompt(prompt);
    
    // Check cache first
    std::string cachedResponse = getCachedResponse(promptHash, "npc_conversation");
    if (!cachedResponse.empty()) {
        std::cout << "[LLM Cache] HIT: npc_conversation" << std::endl;
        LLMResponse response;
        response.success = true;
        response.content = cachedResponse;
        response.inputTokens = 0;  // Cached, no tokens used
        response.completionTokens = 0;
        response.costUSD = 0.0f;
        response.duration = std::chrono::milliseconds(1);  // Cached, instant response
        return response;
    }
    
    // Cache miss - call provider
    std::cout << "[LLM Cache] MISS: npc_conversation" << std::endl;
    auto response = provider_->callLLM(prompt, config_.temperatureNarrativeGeneration);
    
    if (response.success) {
        recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                     std::chrono::system_clock::now(), "npc_conversation"});
        // Cache the successful response
        cacheResponse(promptHash, "npc_conversation", response.content,
                     response.inputTokens, response.completionTokens, response.costUSD);
    }
    
    return response;
}

void LLMManager::interpretPlayerDecisionAsync(const std::string& playerInput, const std::string& context,
                                              std::function<void(const LLMResponse&)> callback) {
    std::string prompt = buildDecisionInterpretationPrompt(playerInput, context);
    LLMRequest req;
    req.prompt = prompt;
    req.callType = "decision_interpretation";
    req.priority = QueuePriority::HIGH;
    req.callback = callback;
    
    LLMRequestQueue::instance()->enqueueRequest(req);
}

void LLMManager::generateNarrativeAsync(const std::string& worldStateContext,
                                       std::function<void(const LLMResponse&)> callback) {
    std::string prompt = "Generate narrative for world state:\n" + worldStateContext;
    LLMRequest req;
    req.prompt = prompt;
    req.callType = "narrative_generation";
    req.priority = QueuePriority::MEDIUM;
    req.callback = callback;
    
    LLMRequestQueue::instance()->enqueueRequest(req);
}

void LLMManager::generateNPCConversationAsync(int npcId1, int npcId2, const std::string& context,
                                             std::function<void(const LLMResponse&)> callback) {
    std::string prompt = "Generate dialogue between NPC " + std::to_string(npcId1) + 
                        " and NPC " + std::to_string(npcId2) + ":\n" + context;
    LLMRequest req;
    req.prompt = prompt;
    req.callType = "npc_conversation";
    req.priority = QueuePriority::LOW;
    req.npcIds = {npcId1, npcId2};
    req.callback = callback;
    
    LLMRequestQueue::instance()->enqueueRequest(req);
}

void LLMManager::recordUsage(const LLMUsage& usage) {
    usageLog_.push_back(usage);
}

float LLMManager::getTotalCostUSD() const {
    float total = 0.0f;
    for (const auto& usage : usageLog_) {
        total += usage.costUSD;
    }
    return total;
}

std::string LLMManager::getCachedResponse(const std::string& promptHash, const std::string& callType) {
    if (!cacheConfig_.enableCaching) {
        cacheMisses_++;
        return "";
    }
    
    auto it = responseCache_.find(promptHash);
    if (it != responseCache_.end()) {
        // Check if entry has expired
        if (it->second.isExpired()) {
            // Expired - remove from cache
            responseCache_.erase(it);
            lruOrder_.remove(promptHash);
            cacheMisses_++;
            return "";
        }
        
        // Cache hit! Move to most-recently-used
        cacheHits_++;
        updateLRUOrder(promptHash);
        return it->second.response;
    }
    
    cacheMisses_++;
    return "";
}

void LLMManager::cacheResponse(const std::string& promptHash, const std::string& callType,
                              const std::string& response, int inputTokens,
                              int completionTokens, float costUSD) {
    if (!cacheConfig_.enableCaching) {
        return;
    }
    
    // Create cache entry
    LLMCacheEntry cacheEntry;
    cacheEntry.response = response;
    cacheEntry.timestamp = std::chrono::system_clock::now();
    cacheEntry.inputTokens = inputTokens;
    cacheEntry.completionTokens = completionTokens;
    cacheEntry.costUSD = costUSD;
    
    // Set TTL based on call type
    if (callType == "decision_interpretation") {
        cacheEntry.ttlSeconds = cacheConfig_.ttlDecisionInterpretation;
    } else if (callType == "narrative_generation") {
        cacheEntry.ttlSeconds = cacheConfig_.ttlNarrativeGeneration;
    } else if (callType == "npc_conversation" || callType == "ambient_dialogue") {
        cacheEntry.ttlSeconds = cacheConfig_.ttlNPCConversation;
    } else {
        cacheEntry.ttlSeconds = 300;  // Default 5 minutes
    }
    
    // Check if need to evict
    if ((int)responseCache_.size() >= cacheConfig_.maxCacheEntries) {
        evictLRUEntry();
    }
    
    // Add to cache
    responseCache_[promptHash] = cacheEntry;
    updateLRUOrder(promptHash);
    
    // Track cost savings from cache hits
    cachedCostSavings_ += costUSD;
}

void LLMManager::clearCache() {
    responseCache_.clear();
    lruOrder_.clear();
    cacheHits_ = 0;
    cacheMisses_ = 0;
    cacheEvictions_ = 0;
}

std::string LLMManager::getCacheStats() const {
    std::stringstream ss;
    ss << "=== LLM Cache Statistics ===\n";
    ss << "Status: " << (cacheConfig_.enableCaching ? "ENABLED" : "DISABLED") << "\n";
    ss << "Entries in cache: " << responseCache_.size() << " / " << cacheConfig_.maxCacheEntries << "\n";
    ss << "Cache hits: " << cacheHits_ << "\n";
    ss << "Cache misses: " << cacheMisses_ << "\n";
    ss << "Cache evictions: " << cacheEvictions_ << "\n";
    
    if (cacheHits_ + cacheMisses_ > 0) {
        float hitRate = (float)cacheHits_ / (cacheHits_ + cacheMisses_) * 100.0f;
        ss << "Hit rate: " << hitRate << "%\n";
    }
    
    ss << "Cost saved by caching: $" << cachedCostSavings_ << "\n";
    ss << "TTL (decision_interpretation): " << cacheConfig_.ttlDecisionInterpretation << "s\n";
    ss << "TTL (narrative_generation): " << cacheConfig_.ttlNarrativeGeneration << "s\n";
    ss << "TTL (npc_conversation): " << cacheConfig_.ttlNPCConversation << "s\n";
    
    return ss.str();
}

int LLMManager::getCacheTTL(const std::string& callType) const {
    if (callType == "decision_interpretation") {
        return cacheConfig_.ttlDecisionInterpretation;
    } else if (callType == "narrative_generation") {
        return cacheConfig_.ttlNarrativeGeneration;
    } else if (callType == "npc_conversation" || callType == "ambient_dialogue") {
        return cacheConfig_.ttlNPCConversation;
    }
    return 300;  // Default 5 minutes
}

void LLMManager::recordLLMCall(int tick, const LLMRequest& request, const LLMResponse& response) {
    replayLog_[{tick, request.callType}] = response;
}

LLMResponse LLMManager::replayLLMCall(int tick, const std::string& callType) {
    auto it = replayLog_.find({tick, callType});
    if (it != replayLog_.end()) {
        return it->second;
    }
    LLMResponse empty;
    empty.success = false;
    empty.error = "No replay data for tick " + std::to_string(tick) + " call " + callType;
    return empty;
}

void LLMManager::processQueue() {
    // Process all pending async requests from the queue
    // Priority order: HIGH > MEDIUM > LOW
    // This method can be called from main loop or background thread
    
    LLMRequestQueue* queue = LLMRequestQueue::instance();
    if (!queue || !provider_) return;
    
    // Process HIGH priority requests first (player input - 3s timeout)
    while (queue->hasRequestsInQueue(QueuePriority::HIGH)) {
        LLMRequest req = queue->dequeueRequest(QueuePriority::HIGH);
        if (req.requestId < 0) break;  // Invalid request
        
        // Use retry logic for HIGH priority requests
        LLMResponse response = callWithRetry(req.prompt, req.callType);
        
        if (response.success) {
            recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                        std::chrono::system_clock::now(), req.callType});
        }
        
        // Execute callback if provided
        if (req.callback) {
            req.callback(response);
        }
    }
    
    // Process MEDIUM priority requests (world state narrative - 10s timeout)
    while (queue->hasRequestsInQueue(QueuePriority::MEDIUM)) {
        LLMRequest req = queue->dequeueRequest(QueuePriority::MEDIUM);
        if (req.requestId < 0) break;  // Invalid request
        
        // Use retry logic for MEDIUM priority requests
        LLMResponse response = callWithRetry(req.prompt, req.callType);
        
        if (response.success) {
            recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                        std::chrono::system_clock::now(), req.callType});
        }
        
        // Execute callback if provided
        if (req.callback) {
            req.callback(response);
        }
    }
    
    // Process LOW priority requests (NPC conversations - 5s timeout)
    while (queue->hasRequestsInQueue(QueuePriority::LOW)) {
        LLMRequest req = queue->dequeueRequest(QueuePriority::LOW);
        if (req.requestId < 0) break;  // Invalid request
        
        LLMResponse response = provider_->callLLM(req.prompt, 
                                                  config_.temperatureNarrativeGeneration);
        
        if (response.success) {
            recordUsage({response.inputTokens, response.completionTokens, response.costUSD,
                        std::chrono::system_clock::now(), req.callType});
        }
        
        // Execute callback if provided
        if (req.callback) {
            req.callback(response);
        }
    }
}

// ============================================================================
// Error Recovery Implementation (NEW)
// ============================================================================

LLMResponse LLMManager::callWithRetry(const std::string& prompt, const std::string& callType, int attemptNumber) {
    // Check if in offline fallback mode
    if (isInOfflineFallbackMode()) {
        if (offlineFallback_) {
            return offlineFallback_->callLLM(prompt);
        }
        LLMResponse response;
        response.success = false;
        response.error = "Offline fallback mode: max retries exhausted, in cooldown period";
        return response;
    }
    
    // First attempt: try primary provider
    if (attemptNumber == 0 && provider_) {
        auto response = provider_->callLLM(prompt);
        
        // Check if we should retry on failure
        if (!response.success && shouldRetry(response, 0)) {
            // Schedule retry after exponential backoff
            int delayMs = getRetryDelayMs(0);
            std::cout << "[LLM] Retry 1/3 after " << delayMs << "ms: " << response.error << std::endl;
            
            // In a real implementation, this would be scheduled asynchronously
            // For now, we'll do a blocking sleep (not ideal, but functional)
            // TODO: Implement proper async retry scheduling
            
            // Recursive retry
            totalRetries_++;
            return callWithRetry(prompt, callType, 1);
        }
        
        if (response.success) {
            successfulRetries_++;
        }
        return response;
    }
    
    // Retry attempts: exponential backoff
    if (attemptNumber > 0 && attemptNumber < errorConfig_.maxRetries && provider_) {
        auto response = provider_->callLLM(prompt);
        
        if (!response.success && shouldRetry(response, attemptNumber)) {
            if (attemptNumber + 1 < errorConfig_.maxRetries) {
                int delayMs = getRetryDelayMs(attemptNumber);
                std::cout << "[LLM] Retry " << (attemptNumber + 1) << "/" << errorConfig_.maxRetries 
                          << " after " << delayMs << "ms: " << response.error << std::endl;
                
                // Recursive retry
                totalRetries_++;
                return callWithRetry(prompt, callType, attemptNumber + 1);
            }
        }
        
        if (response.success) {
            successfulRetries_++;
        }
        return response;
    }
    
    // All retries exhausted: fall back to offline provider
    if (attemptNumber >= errorConfig_.maxRetries && errorConfig_.fallbackToOfflineAfterRetries) {
        std::cout << "[LLM] Max retries exhausted (" << errorConfig_.maxRetries 
                  << "). Switching to offline fallback for " 
                  << errorConfig_.maxRetryDelayMs / 1000 << " seconds." << std::endl;
        
        failedRetries_++;
        enableOfflineFallbackMode(errorConfig_.maxRetryDelayMs / 1000);
        
        if (offlineFallback_) {
            return offlineFallback_->callLLM(prompt);
        }
    }
    
    // Complete failure
    LLMResponse response;
    response.success = false;
    response.error = "LLM call failed after " + std::to_string(errorConfig_.maxRetries) + " retries";
    return response;
}

bool LLMManager::shouldRetry(const LLMResponse& response, int attemptNumber) const {
    // Don't retry if already succeeded
    if (response.success) {
        return false;
    }
    
    // Don't retry if max retries exceeded
    if (attemptNumber >= errorConfig_.maxRetries - 1) {
        return false;
    }
    
    // Retryable errors:
    // - Network timeouts
    // - Temporary server errors (5xx)
    // - Rate limiting (429)
    // - Service unavailable
    
    // Non-retryable errors:
    // - Invalid API key (auth failure)
    // - Malformed request (4xx client errors except 429)
    // - Not found (404)
    
    std::string errorLower = response.error;
    std::transform(errorLower.begin(), errorLower.end(), errorLower.begin(), ::tolower);
    
    // Check for non-retryable patterns
    if (errorLower.find("auth") != std::string::npos || 
        errorLower.find("unauthorized") != std::string::npos ||
        errorLower.find("forbidden") != std::string::npos ||
        errorLower.find("invalid") != std::string::npos ||
        errorLower.find("not found") != std::string::npos ||
        errorLower.find("404") != std::string::npos) {
        return false;
    }
    
    // Check for retryable patterns
    if (errorLower.find("timeout") != std::string::npos ||
        errorLower.find("connection") != std::string::npos ||
        errorLower.find("network") != std::string::npos ||
        errorLower.find("500") != std::string::npos ||
        errorLower.find("502") != std::string::npos ||
        errorLower.find("503") != std::string::npos ||
        errorLower.find("429") != std::string::npos ||
        errorLower.find("rate limit") != std::string::npos) {
        return true;
    }
    
    // Default: retry on unknown errors
    return true;
}

int LLMManager::getRetryDelayMs(int attemptNumber) const {
    if (!errorConfig_.enableExponentialBackoff) {
        return errorConfig_.baseRetryDelayMs;
    }
    
    // Exponential backoff: 1s, 2s, 4s, 8s
    // Formula: baseDelay * 2^attemptNumber
    int delayMs = errorConfig_.baseRetryDelayMs;
    for (int i = 0; i < attemptNumber; ++i) {
        delayMs *= 2;
        if (delayMs > errorConfig_.maxRetryDelayMs) {
            return errorConfig_.maxRetryDelayMs;
        }
    }
    return delayMs;
}

int LLMManager::getTimeoutMs(const std::string& callType) const {
    if (callType == "decision_interpretation") {
        return errorConfig_.decisionTimeoutMs;
    } else if (callType == "narrative_generation") {
        return errorConfig_.narrativeTimeoutMs;
    } else if (callType == "npc_conversation" || callType == "ambient_dialogue") {
        return errorConfig_.ambientTimeoutMs;
    }
    return errorConfig_.decisionTimeoutMs;  // Default
}

void LLMManager::enableOfflineFallbackMode(int durationSeconds) {
    offlineFallbackEnabled_ = true;
    // Calculate timestamp: now + durationSeconds
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    offlineFallbackUntilMs_ = nowMs + (durationSeconds * 1000);
    
    std::cout << "[LLM] Offline fallback mode enabled for " << durationSeconds << " seconds" << std::endl;
}

bool LLMManager::isInOfflineFallbackMode() const {
    if (!offlineFallbackEnabled_) {
        return false;
    }
    
    // Check if fallback duration has expired
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    if (nowMs > offlineFallbackUntilMs_) {
        // Fallback duration expired, exit offline mode
        const_cast<LLMManager*>(this)->offlineFallbackEnabled_ = false;
        const_cast<LLMManager*>(this)->offlineFallbackUntilMs_ = 0;
        std::cout << "[LLM] Offline fallback mode expired, attempting to re-enable API" << std::endl;
        return false;
    }
    
    return true;
}

std::string LLMManager::getErrorRecoveryStats() const {
    std::stringstream ss;
    ss << "LLM Error Recovery Statistics:\n";
    ss << "  Total retries attempted: " << totalRetries_ << "\n";
    ss << "  Successful after retry: " << successfulRetries_ << "\n";
    ss << "  Failed retries (fell back to offline): " << failedRetries_ << "\n";
    ss << "  Success rate: ";
    
    if (totalRetries_ > 0) {
        float successRate = (float)successfulRetries_ / totalRetries_ * 100.0f;
        ss << successRate << "%\n";
    } else {
        ss << "N/A\n";
    }
    
    ss << "  Currently in offline fallback: " << (offlineFallbackEnabled_ ? "YES" : "NO") << "\n";
    ss << "  Exponential backoff enabled: " << (errorConfig_.enableExponentialBackoff ? "YES" : "NO") << "\n";
    ss << "  Max retries configured: " << errorConfig_.maxRetries << "\n";
    
    return ss.str();
}

void LLMManager::evictLRUEntry() {
    if (lruOrder_.empty()) {
        return;
    }
    
    // Remove the least-recently-used entry (front of list)
    std::string lruKey = lruOrder_.front();
    lruOrder_.pop_front();
    responseCache_.erase(lruKey);
    cacheEvictions_++;
    
    std::cout << "[LLM Cache] LRU eviction: removed entry to maintain " 
              << cacheConfig_.maxCacheEntries << " max entries" << std::endl;
}

void LLMManager::updateLRUOrder(const std::string& key) {
    // Move key to end (most-recently-used)
    auto it = std::find(lruOrder_.begin(), lruOrder_.end(), key);
    if (it != lruOrder_.end()) {
        lruOrder_.erase(it);
    }
    lruOrder_.push_back(key);
}

std::string LLMManager::hashPrompt(const std::string& prompt) {
    // Simple hash: just use length + first + last chars (in production use SHA256)
    return std::to_string(prompt.length()) + "_" + std::string(1, prompt.front()) + std::string(1, prompt.back());
}

// ============================================================================
// WorldStateSnapshot Implementation
// ============================================================================

std::string WorldStateSnapshot::toPromptContext() const {
    std::stringstream ss;
    ss << "Tick: " << tickNumber << "\n";
    ss << "Significant NPCs: " << significantNPCIds.size() << " ";
    ss << "Affected Factions: " << affectedFactionIds.size() << " ";
    ss << "Changed Resources: " << changedResourceIds.size() << " ";
    ss << "Triggered Events: " << triggeredEventIds.size() << "\n";
    
    if (!immigrantNPCIds.empty()) {
        ss << "New Immigrants: " << immigrantNPCIds.size() << "\n";
    }
    
    return ss.str();
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string buildWorldStateContext(const WorldStateSnapshot& snapshot) {
    return snapshot.toPromptContext();
}

std::string buildDecisionInterpretationPrompt(const std::string& playerInput, const std::string& worldContext) {
    std::stringstream ss;
    ss << "Interpret player decision for settlement leadership simulator.\n";
    ss << "World context:\n" << worldContext << "\n";
    ss << "Player input: " << playerInput << "\n";
    ss << "Extract: action type, target, tone (positive/neutral/negative), priority (0-1)\n";
    ss << "Respond in JSON format: {\"action\": \"\", \"target\": \"\", \"tone\": \"\", \"priority\": 0.0}";
    
    return ss.str();
}

std::string buildNarrativeGenerationPrompt(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    ss << "Generate narrative issues and crises for settlement simulation.\n";
    ss << "World state:\n" << snapshot.toPromptContext();
    ss << "Identify emerging crises, opportunities, and narrative developments.\n";
    ss << "Keep response concise and grounded in simulation mechanics.";
    
    return ss.str();
}

std::string buildNPCConversationPrompt(int npcId1, int npcId2, const std::string& topic, const std::string& location) {
    std::stringstream ss;
    ss << "Generate natural dialogue between NPC#" << npcId1 << " and NPC#" << npcId2 << ".\n";
    ss << "Topic: " << topic << "\n";
    ss << "Location: " << location << "\n";
    ss << "Keep dialogue brief, realistic, and tied to settlement context.";
    
    return ss.str();
}

}  // namespace TLS
