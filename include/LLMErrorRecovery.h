#pragma once

#include "LLMProvider.h"
#include <chrono>
#include <vector>
#include <functional>

namespace TLS {

/**
 * @enum RetryStrategy
 * @brief Retry strategy selection
 */
enum class RetryStrategy {
    EXPONENTIAL_BACKOFF = 0,  // 1s, 2s, 4s, 8s...
    LINEAR_BACKOFF = 1,        // 1s, 2s, 3s, 4s...
    FIXED_DELAY = 2            // 1s, 1s, 1s, 1s...
};

/**
 * @enum ErrorRecoveryMode
 * @brief Error recovery strategy
 */
enum class ErrorRecoveryMode {
    RETRY_WITH_FALLBACK = 0,    // Retry N times, then use offline fallback
    FAILFAST = 1,                // Fail immediately on error
    QUEUE_AND_RETRY = 2,         // Queue request, retry later
    USE_CACHE_STALE = 3          // Use stale cache if available
};

/**
 * @struct ErrorRecoveryConfig
 * @brief Configuration for error recovery behavior
 */
struct ErrorRecoveryConfig {
    RetryStrategy retryStrategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    ErrorRecoveryMode recoveryMode = ErrorRecoveryMode::RETRY_WITH_FALLBACK;
    int maxRetries = 3;
    int baseDelayMs = 1000;
    int maxDelayMs = 30000;
    bool addJitterToBackoff = true;
    float jitterFactor = 0.1f;  // ±10% jitter
    bool fallbackToOffline = true;
};

/**
 * @struct LLMError
 * @brief Error information for LLM call
 */
struct LLMError {
    enum class ErrorType {
        NETWORK_ERROR,
        TIMEOUT,
        API_ERROR,
        INVALID_RESPONSE,
        RATE_LIMITED,
        PARSE_ERROR,
        PROVIDER_UNAVAILABLE,
        UNKNOWN
    };

    ErrorType type = ErrorType::UNKNOWN;
    std::string message;
    int httpStatusCode = 0;
    int attemptNumber = 0;
    int64_t errorTime = 0;
    bool isRetryable = true;

    std::string toString() const;
};

/**
 * @class ExponentialBackoffCalculator
 * @brief Calculates exponential backoff delays with optional jitter
 *
 * Algorithm:
 * - Base case: delay = base * 2^attempt
 * - With jitter: delay += random(-jitter, +jitter)
 * - Capped: delay = min(delay, maxDelay)
 */
class ExponentialBackoffCalculator {
public:
    /**
     * Initialize backoff calculator
     * @param baseDelayMs Base delay in milliseconds
     * @param maxDelayMs Maximum delay cap
     * @param useJitter Whether to add random jitter
     * @param jitterFactor Jitter magnitude (e.g., 0.1 = ±10%)
     */
    ExponentialBackoffCalculator(
        int baseDelayMs,
        int maxDelayMs,
        bool useJitter = true,
        float jitterFactor = 0.1f
    );

    /**
     * Calculate delay for given attempt number
     * @param attemptNumber Attempt number (0-indexed)
     * @return Delay in milliseconds
     */
    int calculateDelay(int attemptNumber) const;

    /**
     * Calculate delay with specific strategy
     */
    int calculateDelayWithStrategy(
        int attemptNumber,
        RetryStrategy strategy
    ) const;

    /**
     * Get next retry timestamp
     * @param currentTimeMs Current time in milliseconds
     * @param attemptNumber Current attempt number
     * @return Timestamp (ms) when next retry should occur
     */
    int64_t getNextRetryTime(int64_t currentTimeMs, int attemptNumber) const;

private:
    int baseDelayMs_;
    int maxDelayMs_;
    bool useJitter_;
    float jitterFactor_;

    int applyJitter(int delay) const;
};

/**
 * @class ErrorRecoveryManager
 * @brief Manages error recovery, retries, and fallback strategies
 *
 * Responsibilities:
 * - Determine if error is retryable
 * - Calculate retry delays
 * - Manage fallback chain (retry → cache → offline)
 * - Track error patterns and statistics
 */
class ErrorRecoveryManager {
public:
    explicit ErrorRecoveryManager(const ErrorRecoveryConfig& config = ErrorRecoveryConfig());

    /**
     * Handle error and determine recovery action
     * @param error LLM error
     * @param attemptNumber Current attempt number
     * @return Recommended recovery action
     */
    enum class RecoveryAction {
        RETRY_LATER,         // Queue for later retry
        RETRY_IMMEDIATELY,   // Retry right away
        USE_FALLBACK,        // Switch to offline fallback
        FAIL                 // Give up
    };

    RecoveryAction handleError(const LLMError& error, int attemptNumber);

    /**
     * Check if error is retryable
     * @param error Error to check
     * @return true if should retry
     */
    bool isRetryable(const LLMError& error) const;

    /**
     * Calculate retry delay
     * @param attemptNumber Attempt number
     * @return Delay in milliseconds
     */
    int getRetryDelayMs(int attemptNumber) const;

    /**
     * Check if should switch to offline mode
     * @param consecutiveErrors Number of consecutive errors
     * @param totalAttempts Total attempts made
     * @return true if should use offline fallback
     */
    bool shouldSwitchToOfflineMode(int consecutiveErrors, int totalAttempts) const;

    /**
     * Get error statistics
     * @return String with error counts, retry rates, etc.
     */
    std::string getErrorStatistics() const;

    /**
     * Record error for statistics tracking
     */
    void recordError(const LLMError& error);

    /**
     * Reset error tracking
     */
    void resetErrorTracking();

    /**
     * Get current error rate (0-1)
     */
    float getErrorRate() const;

    /**
     * Check if system is in "degraded mode" (too many errors)
     */
    bool isDegradedMode() const { return degradedMode_; }

    /**
     * Set degraded mode (suppress further API calls)
     */
    void setDegradedMode(bool enabled) { degradedMode_ = enabled; }

    /**
     * Get fallback duration (how long to use offline before retrying API)
     */
    int getFallbackDurationSeconds() const { return fallbackDurationSeconds_; }

private:
    ErrorRecoveryConfig config_;
    ExponentialBackoffCalculator backoffCalc_;
    
    // Statistics tracking
    int totalErrors_ = 0;
    int retryableErrors_ = 0;
    int consecutiveErrors_ = 0;
    int successfulRecoveries_ = 0;
    bool degradedMode_ = false;
    int fallbackDurationSeconds_ = 300;  // 5 minutes default

    std::vector<LLMError> recentErrors_;
};

/**
 * @class FailoverProvider
 * @brief Manages failover between multiple LLM providers
 *
 * Implements provider fallback chain:
 * 1. Primary provider (usually OpenAI)
 * 2. Secondary provider (Local LLaMA)
 * 3. Offline fallback (template-based)
 */
class FailoverProvider : public LLMProvider {
public:
    explicit FailoverProvider(const ErrorRecoveryConfig& config = ErrorRecoveryConfig());

    /**
     * Add provider to failover chain
     * @param provider Provider to add
     * @param priority Lower = higher priority
     */
    void addProvider(std::shared_ptr<LLMProvider> provider, int priority);

    /**
     * Call LLM with automatic failover
     * @param prompt Input prompt
     * @return Response (from successful provider or offline fallback)
     */
    LLMResponse callLLM(const std::string& prompt) override;

    bool isAvailable() const override;
    LLMProviderType getProviderType() const override { return LLMProviderType::UNKNOWN; }
    std::string getProviderName() const override { return "Failover"; }
    LLMUsage getTokenUsage() const override;
    void resetTokenUsage() override;

private:
    struct ProviderEntry {
        std::shared_ptr<LLMProvider> provider;
        int priority = 0;
        int failureCount = 0;
        bool isHealthy = true;
    };

    std::vector<ProviderEntry> providers_;
    ErrorRecoveryManager recoveryManager_;
    size_t currentProviderIndex_ = 0;

    LLMResponse callWithFallback(const std::string& prompt, size_t startIndex);
    bool shouldRotateProvider(const LLMResponse& response);
};

}  // namespace TLS
