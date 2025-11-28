#pragma once

#include "LLMProvider.h"
#include <string>
#include <map>
#include <memory>

namespace TLS {

/**
 * @struct CachedLLMResponse
 * @brief Cached LLM response with metadata
 */
struct CachedLLMResponse {
    LLMResponse response;
    int64_t cacheTime = 0;
    int64_t expiryTime = 0;  // 0 = never expires
    int hitCount = 0;
};

/**
 * @class LLMResponseCache
 * @brief Caches LLM responses to avoid redundant API calls
 *
 * Features:
 * - Hash-based prompt caching
 * - Configurable TTL (time-to-live)
 * - Cache invalidation
 * - Usage statistics
 * - Memory bounds
 */
class LLMResponseCache {
public:
    /**
     * Default cache entry TTL in seconds (5 minutes)
     */
    static constexpr int DEFAULT_TTL_SECONDS = 300;

    /**
     * Maximum cache size (number of entries)
     */
    static constexpr size_t MAX_CACHE_SIZE = 1000;

    /**
     * Initialize cache with TTL
     * @param ttlSeconds Time-to-live for cache entries
     */
    explicit LLMResponseCache(int ttlSeconds = DEFAULT_TTL_SECONDS);

    /**
     * Get cached response if available and not expired
     * @param prompt Prompt string
     * @return Pointer to cached response, or nullptr if not found/expired
     */
    const LLMResponse* get(const std::string& prompt);

    /**
     * Store response in cache
     * @param prompt Input prompt
     * @param response Response to cache
     * @return true if cached, false if cache full
     */
    bool put(const std::string& prompt, const LLMResponse& response);

    /**
     * Check if prompt is in cache and not expired
     * @param prompt Prompt to check
     * @return true if cached and valid
     */
    bool contains(const std::string& prompt);

    /**
     * Invalidate specific cache entry
     * @param prompt Prompt to invalidate
     */
    void invalidate(const std::string& prompt);

    /**
     * Clear entire cache
     */
    void clear();

    /**
     * Get cache statistics
     * @return String with cache hit rate, size, etc.
     */
    std::string getStatistics() const;

    /**
     * Get cache hit count
     */
    int getTotalHits() const { return totalHits_; }

    /**
     * Get cache miss count
     */
    int getTotalMisses() const { return totalMisses_; }

    /**
     * Get hit rate (0-1)
     */
    float getHitRate() const;

    /**
     * Perform cache maintenance (evict expired entries)
     */
    void evictExpired();

private:
    std::map<std::string, CachedLLMResponse> cache_;
    int ttlSeconds_;
    int totalHits_ = 0;
    int totalMisses_ = 0;

    std::string computeHash(const std::string& prompt) const;
    int64_t getCurrentTime() const;
};

/**
 * @class OfflineLLMFallback
 * @brief Rule-based LLM fallback for when API is unavailable
 *
 * Generates plausible but formulaic responses using templates.
 * No hallucination risk; completely deterministic.
 *
 * Use cases:
 * - Network is down
 * - API key invalid
 * - Rate limited
 * - All retries exhausted
 */
class OfflineLLMFallback {
public:
    /**
     * Generate offline narrative for NPC issue
     * @param problemType Type of problem (e.g., "RESOURCE_SCARCITY")
     * @param severity Problem severity (0-1)
     * @return Generated narrative text
     */
    static std::string generateNpcNarrative(
        const std::string& problemType,
        float severity
    );

    /**
     * Generate decision interpretation
     * @param playerInput Raw player input
     * @param context World context
     * @return Parsed decision structure as JSON string
     */
    static std::string interpretDecision(
        const std::string& playerInput,
        const std::string& context
    );

    /**
     * Generate world state analysis
     * @param worldStateJson World state as JSON string
     * @return Analysis and suggested issues as text
     */
    static std::string analyzeWorldState(const std::string& worldStateJson);

    /**
     * Generate NPC dialogue
     * @param npcName NPC name
     * @param npcRole NPC role/occupation
     * @param topic Conversation topic
     * @return NPC dialogue text
     */
    static std::string generateNpcDialogue(
        const std::string& npcName,
        const std::string& npcRole,
        const std::string& topic
    );

    /**
     * Generate crisis narrative
     * @param crisisType Type of crisis
     * @param severity Crisis severity (0-1)
     * @return Crisis narrative text
     */
    static std::string generateCrisisNarrative(
        const std::string& crisisType,
        float severity
    );

private:
    /**
     * Generate templated text from problem type
     */
    static std::string getTemplateForProblem(const std::string& problemType);

    /**
     * Apply severity scaling to template
     */
    static std::string applySeverityScaling(
        const std::string& template_text,
        float severity
    );
};

}  // namespace TLS
