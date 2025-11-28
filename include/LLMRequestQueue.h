#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>

namespace TLS {

// Forward declarations
struct LLMResponse;
class LLMProvider;

/**
 * @enum LLMRequestPriority
 * @brief Priority levels for LLM requests
 */
enum class LLMRequestPriority {
    HIGH = 0,      // Player input interpretation (timeout: 3s)
    MEDIUM = 1,    // World state narrative (timeout: 10s)
    LOW = 2        // NPC ambient dialogue (timeout: 5s)
};

/**
 * @struct LLMRequest
 * @brief Single LLM request in queue
 */
struct LLMRequest {
    int requestId = -1;
    LLMRequestPriority priority = LLMRequestPriority::MEDIUM;
    std::string prompt;
    std::function<void(const LLMResponse&)> callback;
    int64_t enqueuedTick = 0;
    int timeoutTicks = 100;
    
    // Retry tracking
    int attemptCount = 0;
    int maxAttempts = 3;
    int64_t nextRetryTick = 0;
};

/**
 * @class TokenBucketRateLimiter
 * @brief Rate limiting using token bucket algorithm
 *
 * Maintains a bucket of tokens that refill at a configurable rate.
 * Each request consumes tokens; requests blocked if insufficient tokens.
 */
class TokenBucketRateLimiter {
public:
    /**
     * @param maxTokensPerMinute Maximum requests allowed per minute
     */
    explicit TokenBucketRateLimiter(float maxTokensPerMinute = 60.0f);

    /**
     * Check if request is allowed
     * @return true if request can proceed, false if rate limited
     */
    bool canMakeRequest();

    /**
     * Get current available tokens
     * @return Number of tokens currently available
     */
    float getAvailableTokens() const;

    /**
     * Get wait time until next request allowed (in seconds)
     * @return Seconds to wait (0 if request allowed now)
     */
    float getWaitTimeSeconds() const;

    /**
     * Reset rate limiter to full capacity
     */
    void reset();

    /**
     * Set rate limit
     * @param maxTokensPerMinute New rate limit
     */
    void setRateLimit(float maxTokensPerMinute);

private:
    float maxTokens_;
    float tokensAvailable_;
    float refillRatePerSecond_;
    int64_t lastRefillTime_;

    void refillBucket();
};

/**
 * @class LLMRequestQueue
 * @brief Manages prioritized queue of LLM requests with timeout handling
 *
 * Features:
 * - Priority-based queuing (HIGH > MEDIUM > LOW)
 * - Duplicate request detection
 * - Per-priority queue size limits
 * - Timeout enforcement
 * - Request retry scheduling
 */
class LLMRequestQueue {
public:
    /**
     * Enqueue a new LLM request
     * @param request Request to enqueue
     * @return true if enqueued, false if rejected (queue full, duplicate, etc.)
     */
    bool enqueueRequest(const LLMRequest& request);

    /**
     * Dequeue next ready request (not timed out)
     * @param currentTick Current game tick
     * @return Pointer to request, or nullptr if queue empty or all timed out
     */
    LLMRequest* dequeueNextRequest(int64_t currentTick);

    /**
     * Check if queue has requests ready to process
     * @return true if any non-timed-out request available
     */
    bool hasReadyRequests(int64_t currentTick) const;

    /**
     * Get queue size for specific priority
     * @param priority Priority level
     * @return Number of requests at this priority
     */
    size_t getQueueSizeForPriority(LLMRequestPriority priority) const;

    /**
     * Get total queue size
     * @return Total requests in queue
     */
    size_t getTotalQueueSize() const;

    /**
     * Clear all requests from queue
     */
    void clear();

    /**
     * Process timed-out requests
     * @param currentTick Current game tick
     * @return Number of requests that timed out
     */
    int processTimeouts(int64_t currentTick);

    /**
     * Get timeout ticks for priority level
     * @param priority Priority level
     * @return Ticks before request times out
     */
    static int getTimeoutTicksForPriority(LLMRequestPriority priority);

private:
    static constexpr size_t MAX_HIGH_QUEUE = 5;
    static constexpr size_t MAX_MEDIUM_QUEUE = 3;
    static constexpr size_t MAX_LOW_QUEUE = 10;
    static constexpr size_t MAX_TOTAL_QUEUE = 15;

    std::deque<LLMRequest> highPriorityQueue_;
    std::deque<LLMRequest> mediumPriorityQueue_;
    std::deque<LLMRequest> lowPriorityQueue_;

    int nextRequestId_ = 0;

    std::deque<LLMRequest>& getQueueForPriority(LLMRequestPriority priority);
    const std::deque<LLMRequest>& getQueueForPriority(LLMRequestPriority priority) const;
    size_t getMaxSizeForPriority(LLMRequestPriority priority) const;
    bool isDuplicate(const std::string& prompt) const;
};

}  // namespace TLS
