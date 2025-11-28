#include "LLMErrorRecovery.h"
#include <cmath>
#include <sstream>
#include <random>
#include <algorithm>
#include <chrono>

namespace TLS {

// ==================== LLMError ====================

std::string LLMError::toString() const {
    std::ostringstream oss;
    oss << "LLMError{type=" << static_cast<int>(type)
        << ", message=\"" << message << "\""
        << ", httpStatus=" << httpStatusCode
        << ", attempt=" << attemptNumber
        << ", retryable=" << (isRetryable ? "true" : "false")
        << "}";
    return oss.str();
}

// ==================== ExponentialBackoffCalculator ====================

ExponentialBackoffCalculator::ExponentialBackoffCalculator(
    int baseDelayMs,
    int maxDelayMs,
    bool useJitter,
    float jitterFactor
)
    : baseDelayMs_(baseDelayMs)
    , maxDelayMs_(maxDelayMs)
    , useJitter_(useJitter)
    , jitterFactor_(jitterFactor) {
}

int ExponentialBackoffCalculator::calculateDelay(int attemptNumber) const {
    return calculateDelayWithStrategy(attemptNumber, RetryStrategy::EXPONENTIAL_BACKOFF);
}

int ExponentialBackoffCalculator::calculateDelayWithStrategy(
    int attemptNumber,
    RetryStrategy strategy
) const {
    int delay = 0;

    switch (strategy) {
        case RetryStrategy::EXPONENTIAL_BACKOFF:
            // delay = base * 2^attempt
            delay = baseDelayMs_ * static_cast<int>(std::pow(2.0, attemptNumber));
            break;

        case RetryStrategy::LINEAR_BACKOFF:
            // delay = base * (1 + attempt)
            delay = baseDelayMs_ * (1 + attemptNumber);
            break;

        case RetryStrategy::FIXED_DELAY:
            // delay = base (constant)
            delay = baseDelayMs_;
            break;

        default:
            delay = baseDelayMs_;
    }

    // Apply jitter
    if (useJitter_) {
        delay = applyJitter(delay);
    }

    // Cap at maximum
    delay = std::min(delay, maxDelayMs_);

    return std::max(delay, 1);  // Minimum 1ms
}

int64_t ExponentialBackoffCalculator::getNextRetryTime(
    int64_t currentTimeMs,
    int attemptNumber
) const {
    int delayMs = calculateDelay(attemptNumber);
    return currentTimeMs + delayMs;
}

int ExponentialBackoffCalculator::applyJitter(int delay) const {
    // Random jitter: ±jitterFactor%
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    float jitterAmount = delay * jitterFactor_;
    std::uniform_real_distribution<> dis(-jitterAmount, jitterAmount);
    
    int jitteredDelay = delay + static_cast<int>(dis(gen));
    return std::max(jitteredDelay, 1);  // Minimum 1ms
}

// ==================== ErrorRecoveryManager ====================

ErrorRecoveryManager::ErrorRecoveryManager(const ErrorRecoveryConfig& config)
    : config_(config)
    , backoffCalc_(
        config.baseDelayMs,
        config.maxDelayMs,
        config.addJitterToBackoff,
        config.jitterFactor
    ) {
}

ErrorRecoveryManager::RecoveryAction ErrorRecoveryManager::handleError(
    const LLMError& error,
    int attemptNumber
) {
    recordError(error);

    if (!isRetryable(error)) {
        return RecoveryAction::FAIL;
    }

    if (attemptNumber >= config_.maxRetries) {
        if (config_.fallbackToOffline) {
            return RecoveryAction::USE_FALLBACK;
        }
        return RecoveryAction::FAIL;
    }

    // Check if should switch to offline mode
    if (shouldSwitchToOfflineMode(consecutiveErrors_, attemptNumber + 1)) {
        degradedMode_ = true;
        return RecoveryAction::USE_FALLBACK;
    }

    // Decide between immediate retry and delayed retry
    if (error.type == LLMError::ErrorType::TIMEOUT) {
        return RecoveryAction::RETRY_LATER;
    } else if (error.type == LLMError::ErrorType::RATE_LIMITED) {
        return RecoveryAction::RETRY_LATER;
    } else {
        return RecoveryAction::RETRY_IMMEDIATELY;
    }
}

bool ErrorRecoveryManager::isRetryable(const LLMError& error) const {
    switch (error.type) {
        case LLMError::ErrorType::NETWORK_ERROR:
        case LLMError::ErrorType::TIMEOUT:
        case LLMError::ErrorType::RATE_LIMITED:
            return true;
        case LLMError::ErrorType::API_ERROR:
            return error.httpStatusCode >= 500;  // 5xx errors are retryable
        case LLMError::ErrorType::INVALID_RESPONSE:
        case LLMError::ErrorType::PARSE_ERROR:
        case LLMError::ErrorType::PROVIDER_UNAVAILABLE:
            return true;
        case LLMError::ErrorType::UNKNOWN:
            return error.isRetryable;
        default:
            return false;
    }
}

int ErrorRecoveryManager::getRetryDelayMs(int attemptNumber) const {
    return backoffCalc_.calculateDelay(attemptNumber);
}

bool ErrorRecoveryManager::shouldSwitchToOfflineMode(
    int consecutiveErrors,
    int totalAttempts
) const {
    // Switch to offline after 3 consecutive errors
    if (consecutiveErrors >= 3) {
        return true;
    }

    // Or if error rate is high (>50%)
    float errorRate = getErrorRate();
    if (errorRate > 0.5f && totalAttempts > 10) {
        return true;
    }

    return false;
}

std::string ErrorRecoveryManager::getErrorStatistics() const {
    std::ostringstream oss;
    oss << "Error Recovery Statistics:\n";
    oss << "  Total Errors: " << totalErrors_ << "\n";
    oss << "  Retryable: " << retryableErrors_ << "\n";
    oss << "  Consecutive Errors: " << consecutiveErrors_ << "\n";
    oss << "  Successful Recoveries: " << successfulRecoveries_ << "\n";
    oss << "  Error Rate: " << (getErrorRate() * 100.0f) << "%\n";
    oss << "  Degraded Mode: " << (degradedMode_ ? "Yes" : "No") << "\n";
    oss << "  Fallback Duration: " << fallbackDurationSeconds_ << "s\n";
    return oss.str();
}

void ErrorRecoveryManager::recordError(const LLMError& error) {
    totalErrors_++;
    if (isRetryable(error)) {
        retryableErrors_++;
        consecutiveErrors_++;
    } else {
        consecutiveErrors_ = 0;
    }

    // Keep last N errors
    recentErrors_.push_back(error);
    if (recentErrors_.size() > 100) {
        recentErrors_.erase(recentErrors_.begin());
    }
}

void ErrorRecoveryManager::resetErrorTracking() {
    totalErrors_ = 0;
    retryableErrors_ = 0;
    consecutiveErrors_ = 0;
    successfulRecoveries_ = 0;
    degradedMode_ = false;
    recentErrors_.clear();
}

float ErrorRecoveryManager::getErrorRate() const {
    if (totalErrors_ == 0) return 0.0f;
    return static_cast<float>(retryableErrors_) / static_cast<float>(totalErrors_);
}

// ==================== FailoverProvider ====================

FailoverProvider::FailoverProvider(const ErrorRecoveryConfig& config)
    : recoveryManager_(config) {
}

void FailoverProvider::addProvider(std::shared_ptr<LLMProvider> provider, int priority) {
    ProviderEntry entry;
    entry.provider = provider;
    entry.priority = priority;
    entry.failureCount = 0;
    entry.isHealthy = true;

    providers_.push_back(entry);

    // Sort by priority (lower = higher priority)
    std::sort(providers_.begin(), providers_.end(),
        [](const ProviderEntry& a, const ProviderEntry& b) {
            return a.priority < b.priority;
        });
}

LLMResponse FailoverProvider::callLLM(const std::string& prompt) {
    if (providers_.empty()) {
        LLMResponse response;
        response.wasSuccessful = false;
        response.errorMessage = "No providers configured";
        return response;
    }

    return callWithFallback(prompt, 0);
}

LLMResponse FailoverProvider::callWithFallback(const std::string& prompt, size_t startIndex) {
    for (size_t i = startIndex; i < providers_.size(); ++i) {
        ProviderEntry& entry = providers_[i];

        if (!entry.isHealthy) {
            continue;  // Skip unhealthy providers
        }

        // Try this provider
        LLMResponse response = entry.provider->callLLM(prompt);

        if (response.wasSuccessful) {
            entry.failureCount = 0;  // Reset failure count on success
            currentProviderIndex_ = i;
            return response;
        }

        // This provider failed
        entry.failureCount++;
        LLMError error;
        error.type = LLMError::ErrorType::PROVIDER_UNAVAILABLE;
        error.message = response.errorMessage;

        // Check if should mark provider as unhealthy
        if (shouldRotateProvider(response)) {
            entry.isHealthy = false;
        }
    }

    // All providers failed - use offline fallback
    LLMResponse fallbackResponse;
    fallbackResponse.wasSuccessful = true;
    fallbackResponse.provider = LLMProviderType::OFFLINE_FALLBACK;
    fallbackResponse.text = "Fallback response (all providers failed): " + prompt.substr(0, 50) + "...";
    fallbackResponse.inputTokens = static_cast<int>(prompt.length() / 4);
    fallbackResponse.completionTokens = 50;
    fallbackResponse.totalTokens = fallbackResponse.inputTokens + fallbackResponse.completionTokens;
    fallbackResponse.latencyMs = 10;

    return fallbackResponse;
}

bool FailoverProvider::shouldRotateProvider(const LLMResponse& response) {
    // Rotate if too many consecutive failures
    if (!response.wasSuccessful) {
        int maxFailures = 3;
        if (providers_[currentProviderIndex_].failureCount >= maxFailures) {
            return true;
        }
    }
    return false;
}

bool FailoverProvider::isAvailable() const {
    // Available if at least one provider is healthy or offline fallback is enabled
    for (const auto& entry : providers_) {
        if (entry.isHealthy && entry.provider->isAvailable()) {
            return true;
        }
    }
    return true;  // Always available (offline fallback)
}

LLMUsage FailoverProvider::getTokenUsage() const {
    LLMUsage totalUsage{0, 0, 0, 0.0f, 0};

    for (const auto& entry : providers_) {
        LLMUsage usage = entry.provider->getTokenUsage();
        totalUsage.totalInputTokens += usage.totalInputTokens;
        totalUsage.totalCompletionTokens += usage.totalCompletionTokens;
        totalUsage.totalRequests += usage.totalRequests;
        totalUsage.estimatedCostUSD += usage.estimatedCostUSD;
    }

    return totalUsage;
}

void FailoverProvider::resetTokenUsage() {
    for (auto& entry : providers_) {
        entry.provider->resetTokenUsage();
    }
}

}  // namespace TLS
