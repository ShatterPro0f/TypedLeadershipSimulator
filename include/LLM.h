#ifndef TLS_LLM_H
#define TLS_LLM_H

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <map>
#include <list>
#include <functional>
#include <chrono>
#include "Core.h"

namespace TLS {

// ============================================================================
// LLM Configuration
// ============================================================================

struct LLMConfig {
    enum ProviderType {
        OPENAI = 0,
        LOCAL_LLAMA = 1,
        OFFLINE_FALLBACK = 2
    };
    
    ProviderType provider = OFFLINE_FALLBACK;
    std::string apiKey;                 // For OpenAI
    std::string apiEndpoint;            // For LocalLLaMA
    int timeoutSeconds = 10;
    int maxRetries = 3;
    float temperatureDecisionInterpretation = 0.3f;  // Lower = more deterministic
    float temperatureNarrativeGeneration = 0.7f;    // Higher = more creative
    bool enableTokenTracking = true;
    bool enableCaching = true;
    
    // Load config from environment variables and file
    static LLMConfig loadFromEnvironment();
    static LLMConfig loadFromFile(const std::string& configPath);
};

// ============================================================================
// LLM Response Structures
// ============================================================================

struct LLMResponse {
    bool success = false;
    std::string content;
    int inputTokens = 0;
    int completionTokens = 0;
    float costUSD = 0.0f;
    std::string error;
    std::chrono::milliseconds duration{0};
};

struct LLMUsage {
    int inputTokens = 0;
    int completionTokens = 0;
    float costUSD = 0.0f;
    std::chrono::system_clock::time_point timestamp;
    std::string callType;  // "decision_interpretation", "narrative_generation", "npc_conversation"
};

// ============================================================================
// LLM Response Caching (LRU with TTL by call type)
// ============================================================================

struct LLMCacheEntry {
    std::string response;                              // Cached LLM response content
    std::chrono::system_clock::time_point timestamp;  // When cached
    int ttlSeconds = 0;                               // Time-to-live in seconds
    int inputTokens = 0;                              // For cost calculation
    int completionTokens = 0;
    float costUSD = 0.0f;
    
    bool isExpired() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp).count();
        return elapsed > ttlSeconds;
    }
};

struct LLMCacheConfig {
    // Maximum cache entries (LRU eviction when exceeded)
    int maxCacheEntries = 1000;
    
    // TTL (time-to-live) in seconds by call type
    int ttlDecisionInterpretation = 120;   // 2 minutes (decisions change quickly)
    int ttlNarrativeGeneration = 600;      // 10 minutes (world state slower to change)
    int ttlNPCConversation = 300;          // 5 minutes (NPC interactions moderate)
    
    // Enable/disable caching
    bool enableCaching = true;
    
    // Cost tracking
    bool trackCachingStats = true;
};

// ============================================================================
// Request Queue System
// ============================================================================

enum class QueuePriority {
    HIGH = 0,      // Player input (immediate, 3s timeout)
    MEDIUM = 1,    // World state narrative (10s timeout)
    LOW = 2        // NPC conversations (5s timeout)
};

struct LLMRequest {
    int requestId = 0;
    int timestamp = 0;
    std::vector<int> npcIds;           // Affected NPCs
    std::string prompt;
    std::string callType;              // "decision_interpretation", "narrative_generation", "ambient_dialogue"
    QueuePriority priority = QueuePriority::LOW;
    std::function<void(const LLMResponse&)> callback;
    
    // For replay/determinism
    bool isFromReplay = false;
    std::string cachedResponse;
};

// ============================================================================
// LLM Provider Abstract Interface
// ============================================================================

class LLMProvider {
public:
    virtual ~LLMProvider() = default;
    
    // Core interface
    virtual LLMResponse callLLM(const std::string& prompt, float temperature = 0.5f) = 0;
    virtual bool isAvailable() const = 0;
    virtual std::string getProviderName() const = 0;
    
    // Optional: async interface (can be implemented in subclasses)
    virtual void callLLMAsync(const LLMRequest& request) {
        // Default: synchronous fallback
        auto response = callLLM(request.prompt);
        if (request.callback) {
            request.callback(response);
        }
    }
    
    // Token counting (for cost estimation)
    virtual int estimateTokens(const std::string& text) const;
    virtual float calculateCostUSD(int inputTokens, int completionTokens) const {
        return 0.0f;  // Override in subclasses
    }
};

// ============================================================================
// OpenAI Provider Implementation
// ============================================================================

class OpenAIProvider : public LLMProvider {
public:
    explicit OpenAIProvider(const std::string& apiKey, float decisionTemp = 0.3f, float narrativeTemp = 0.7f);
    
    LLMResponse callLLM(const std::string& prompt, float temperature = 0.5f) override;
    bool isAvailable() const override;
    std::string getProviderName() const override { return "OpenAI"; }
    
    float calculateCostUSD(int inputTokens, int completionTokens) const override;
    
private:
    std::string apiKey_;
    float decisionTemperature_;
    float narrativeTemperature_;
    bool isAvailable_ = false;
    
    // HTTP request implementation
    LLMResponse makeHTTPRequest(const std::string& prompt, float temperature);
};

// ============================================================================
// Local LLaMA Provider Implementation
// ============================================================================

class LocalLLaMAProvider : public LLMProvider {
public:
    explicit LocalLLaMAProvider(const std::string& apiEndpoint = "http://localhost:8000",
                               float decisionTemp = 0.3f, float narrativeTemp = 0.7f);
    
    LLMResponse callLLM(const std::string& prompt, float temperature = 0.5f) override;
    bool isAvailable() const override;
    std::string getProviderName() const override { return "LocalLLaMA"; }
    
private:
    std::string apiEndpoint_;
    float decisionTemperature_;
    float narrativeTemperature_;
    bool isAvailable_ = false;
    
    LLMResponse makeHTTPRequest(const std::string& prompt, float temperature);
};

// ============================================================================
// Offline Fallback Provider (Rule-Based, No Network)
// ============================================================================

class OfflineFallbackProvider : public LLMProvider {
public:
    explicit OfflineFallbackProvider();
    
    LLMResponse callLLM(const std::string& prompt, float temperature = 0.5f) override;
    bool isAvailable() const override { return true; }
    std::string getProviderName() const override { return "OfflineFallback"; }
    
private:
    // Template-based narrative generation
    std::string generateOfflineNarrative(const std::string& prompt);
    std::string generateOfflineDecisionResponse(const std::string& prompt);
};

// ============================================================================
// LLM Request Queue Manager
// ============================================================================

class LLMRequestQueue {
public:
    static LLMRequestQueue* instance();
    
    // Queue management
    void enqueueRequest(const LLMRequest& request);
    LLMRequest dequeueRequest(QueuePriority priority = QueuePriority::HIGH);
    bool hasRequestsInQueue(QueuePriority priority) const;
    size_t getQueueSize(QueuePriority priority) const;
    void clearQueue(QueuePriority priority);
    
    // Process requests asynchronously
    void processNextRequest(std::shared_ptr<LLMProvider> provider);
    void flushQueue(std::shared_ptr<LLMProvider> provider);
    
private:
    LLMRequestQueue() = default;
    
    struct PriorityQueue {
        std::queue<LLMRequest> high;    // Player input
        std::queue<LLMRequest> medium;  // World state
        std::queue<LLMRequest> low;     // NPC conversations
    } queues_;
    
    int nextRequestId_ = 0;
};

// ============================================================================
// LLM Error Recovery (Exponential Backoff & Retry)
// ============================================================================

struct LLMErrorConfig {
    int maxRetries = 3;
    int baseRetryDelayMs = 1000;  // 1 second
    int maxRetryDelayMs = 8000;   // 8 seconds (4th retry)
    int decisionTimeoutMs = 3000;    // 3 seconds for player decisions
    int narrativeTimeoutMs = 10000;  // 10 seconds for world state
    int ambientTimeoutMs = 5000;     // 5 seconds for NPC conversations
    bool enableExponentialBackoff = true;
    bool fallbackToOfflineAfterRetries = true;
};

// ============================================================================
// LLM Manager (Central Coordinator)
// ============================================================================

class LLMManager {
public:
    static LLMManager* instance();
    
    // Initialization
    bool initialize(const LLMConfig& config, const LLMErrorConfig& errorConfig = LLMErrorConfig(),
                   const LLMCacheConfig& cacheConfig = LLMCacheConfig());
    void shutdown();
    
    // Provider access
    std::shared_ptr<LLMProvider> getProvider() const { return provider_; }
    LLMConfig getConfig() const { return config_; }
    LLMErrorConfig getErrorConfig() const { return errorConfig_; }
    
    // Synchronous calls (blocking) with integrated retry/timeout handling
    LLMResponse interpretPlayerDecision(const std::string& playerInput, const std::string& context);
    LLMResponse generateNarrative(const std::string& worldStateContext);
    LLMResponse generateNPCConversation(int npcId1, int npcId2, const std::string& context);
    
    // Asynchronous calls (non-blocking)
    void interpretPlayerDecisionAsync(const std::string& playerInput, const std::string& context,
                                     std::function<void(const LLMResponse&)> callback);
    void generateNarrativeAsync(const std::string& worldStateContext,
                               std::function<void(const LLMResponse&)> callback);
    void generateNPCConversationAsync(int npcId1, int npcId2, const std::string& context,
                                     std::function<void(const LLMResponse&)> callback);
    
    // Error recovery methods (NEW)
    /**
     * Call LLM with automatic retry on timeout/error
     * Uses exponential backoff: 1s, 2s, 4s, 8s
     * Falls back to offline provider after max retries
     * @param prompt Input prompt
     * @param callType Type of call for timeout selection
     * @param attemptNumber Current attempt (0 = first attempt)
     * @return LLMResponse (success or offline fallback)
     */
    LLMResponse callWithRetry(const std::string& prompt, const std::string& callType, int attemptNumber = 0);
    
    /**
     * Check if LLM call should be retried based on error
     * @param response Previous response
     * @return true if should retry
     */
    bool shouldRetry(const LLMResponse& response, int attemptNumber) const;
    
    /**
     * Get retry delay in milliseconds
     * @param attemptNumber Current attempt (0-indexed)
     * @return Delay in ms (exponential: 1000, 2000, 4000, 8000)
     */
    int getRetryDelayMs(int attemptNumber) const;
    
    /**
     * Get timeout for specific call type
     * @param callType Type of LLM call
     * @return Timeout in milliseconds
     */
    int getTimeoutMs(const std::string& callType) const;
    
    /**
     * Switch to offline fallback mode (temporarily disable API calls)
     * @param durationSeconds Duration to stay in fallback mode
     */
    void enableOfflineFallbackMode(int durationSeconds = 300);
    
    /**
     * Check if currently in offline fallback mode
     */
    bool isInOfflineFallbackMode() const;
    
    /**
     * Get error recovery statistics
     * @return String describing retry/error statistics
     */
    std::string getErrorRecoveryStats() const;
    
    // Usage tracking
    void recordUsage(const LLMUsage& usage);
    std::vector<LLMUsage> getUsageLog() const { return usageLog_; }
    float getTotalCostUSD() const;
    
    // Caching (LRU with TTL by call type)
    /**
     * Get cached response if available and not expired
     * @param promptHash Hash of prompt for lookup
     * @param callType Type of call (determines TTL)
     * @return Cached response, or empty string if not found/expired
     */
    std::string getCachedResponse(const std::string& promptHash, const std::string& callType);
    
    /**
     * Cache LLM response with TTL based on call type
     * @param promptHash Hash of prompt
     * @param callType Type of call (determines TTL)
     * @param response Response content to cache
     * @param inputTokens Tokens used (for cost tracking)
     * @param completionTokens Tokens generated
     * @param costUSD Cost in USD
     */
    void cacheResponse(const std::string& promptHash, const std::string& callType,
                      const std::string& response, int inputTokens = 0,
                      int completionTokens = 0, float costUSD = 0.0f);
    
    /**
     * Clear entire cache
     */
    void clearCache();
    
    /**
     * Get cache statistics
     * @return String with cache hit/miss rates and memory usage
     */
    std::string getCacheStats() const;
    
    /**
     * Get TTL for specific call type
     * @param callType Type of LLM call
     * @return TTL in seconds
     */
    int getCacheTTL(const std::string& callType) const;
    
    // Replay system
    void recordLLMCall(int tick, const LLMRequest& request, const LLMResponse& response);
    LLMResponse replayLLMCall(int tick, const std::string& callType);
    
    // Queue processing
    void processQueue();  // Process all pending async requests
    
private:
    LLMManager() = default;
    
    std::shared_ptr<LLMProvider> provider_;
    std::shared_ptr<LLMProvider> offlineFallback_;  // Always available fallback
    LLMConfig config_;
    LLMErrorConfig errorConfig_;  // Error recovery configuration
    LLMCacheConfig cacheConfig_;  // Cache configuration
    std::vector<LLMUsage> usageLog_;
    
    // LRU Cache with TTL: (promptHash, callType) -> CacheEntry
    // Use ordered list to track LRU order
    struct CacheEntry {
        std::string key;        // Prompt hash (for identification)
        std::string callType;   // Call type (for TTL lookup)
        LLMCacheEntry entry;    // Actual cache data
    };
    
    std::map<std::string, LLMCacheEntry> responseCache_;  // Fast lookup by hash
    std::list<std::string> lruOrder_;                      // LRU eviction order
    
    // Cache statistics
    int64_t cacheHits_ = 0;
    int64_t cacheMisses_ = 0;
    int64_t cacheEvictions_ = 0;
    float cachedCostSavings_ = 0.0f;  // Cost saved by cache hits
    
    // Replay log: (tick, callType) -> response
    std::map<std::pair<int, std::string>, LLMResponse> replayLog_;
    
    // Error recovery state
    int totalRetries_ = 0;
    int failedRetries_ = 0;
    int successfulRetries_ = 0;
    bool offlineFallbackEnabled_ = false;
    int64_t offlineFallbackUntilMs_ = 0;
    
    // Helper functions
    std::string hashPrompt(const std::string& prompt);
    void evictLRUEntry();  // Remove least-recently-used entry
    void updateLRUOrder(const std::string& key);  // Move to most-recently-used
};

// ============================================================================
// World State Snapshot for LLM Context
// ============================================================================

struct WorldStateSnapshot {
    int tickNumber = 0;
    std::vector<int> significantNPCIds;     // NPCs with mood/loyalty deltas > threshold
    std::vector<int> affectedFactionIds;    // Factions with loyalty changes
    std::vector<int> changedResourceIds;    // Resources that crossed scarcity
    std::vector<int> triggeredEventIds;     // Events that occurred this tick
    std::vector<int> immigrantNPCIds;       // New NPCs (immigration)
    
    // Convert to string for LLM context
    std::string toPromptContext() const;
};

// ============================================================================
// Helper Functions for Prompt Construction
// ============================================================================

// Build world state context for LLM
std::string buildWorldStateContext(const WorldStateSnapshot& snapshot);

// Build decision interpretation prompt
std::string buildDecisionInterpretationPrompt(const std::string& playerInput, const std::string& worldContext);

// Build narrative generation prompt
std::string buildNarrativeGenerationPrompt(const WorldStateSnapshot& snapshot);

// Build NPC conversation prompt
std::string buildNPCConversationPrompt(int npcId1, int npcId2, const std::string& topic, const std::string& location);

}  // namespace TLS

#endif  // TLS_LLM_H
