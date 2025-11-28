#include "LLMRequestQueue.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace TLS {

// ==================== TokenBucketRateLimiter ====================

TokenBucketRateLimiter::TokenBucketRateLimiter(float maxTokensPerMinute)
    : maxTokens_(maxTokensPerMinute)
    , tokensAvailable_(maxTokensPerMinute)
    , refillRatePerSecond_(maxTokensPerMinute / 60.0f)
    , lastRefillTime_(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) {
}

bool TokenBucketRateLimiter::canMakeRequest() {
    refillBucket();
    
    if (tokensAvailable_ >= 1.0f) {
        tokensAvailable_ -= 1.0f;
        return true;
    }
    return false;
}

float TokenBucketRateLimiter::getAvailableTokens() const {
    // Note: This is const, so we can't update bucket state here
    // In a real implementation, consider making this non-const or using mutable
    return tokensAvailable_;
}

float TokenBucketRateLimiter::getWaitTimeSeconds() const {
    if (tokensAvailable_ >= 1.0f) {
        return 0.0f;
    }
    
    // Calculate wait time until 1 token is available
    float tokensNeeded = 1.0f - tokensAvailable_;
    return tokensNeeded / refillRatePerSecond_;
}

void TokenBucketRateLimiter::reset() {
    tokensAvailable_ = maxTokens_;
    lastRefillTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TokenBucketRateLimiter::setRateLimit(float maxTokensPerMinute) {
    maxTokens_ = maxTokensPerMinute;
    refillRatePerSecond_ = maxTokensPerMinute / 60.0f;
    tokensAvailable_ = std::min(tokensAvailable_, maxTokens_);
}

void TokenBucketRateLimiter::refillBucket() {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    int64_t elapsed = now - lastRefillTime_;
    
    if (elapsed <= 0) return;
    
    // Calculate tokens to add: refill_rate * (elapsed_seconds)
    float secondsElapsed = elapsed / 1000.0f;
    float tokensToAdd = refillRatePerSecond_ * secondsElapsed;
    
    tokensAvailable_ = std::min(maxTokens_, tokensAvailable_ + tokensToAdd);
    lastRefillTime_ = now;
}

// ==================== LLMRequestQueue ====================

bool LLMRequestQueue::enqueueRequest(const LLMRequest& request) {
    // Check for duplicates
    if (isDuplicate(request.prompt)) {
        return false;
    }

    // Check queue size limits
    size_t totalSize = highPriorityQueue_.size() + mediumPriorityQueue_.size() + lowPriorityQueue_.size();
    if (totalSize >= MAX_TOTAL_QUEUE) {
        return false;  // Queue full
    }

    LLMRequest newRequest = request;
    newRequest.requestId = nextRequestId_++;

    // Add to appropriate queue based on priority
    auto& queue = getQueueForPriority(request.priority);
    size_t maxSize = getMaxSizeForPriority(request.priority);
    
    if (queue.size() < maxSize) {
        queue.push_back(newRequest);
        return true;
    }
    
    return false;
}

LLMRequest* LLMRequestQueue::dequeueNextRequest(int64_t currentTick) {
    // Process timeouts first
    processTimeouts(currentTick);

    // Dequeue by priority: HIGH > MEDIUM > LOW
    if (!highPriorityQueue_.empty()) {
        LLMRequest req = highPriorityQueue_.front();
        highPriorityQueue_.pop_front();
        // Store in temporary (requires modification to store current request)
        // For now, return null to indicate need to refactor
        return nullptr;
    }

    if (!mediumPriorityQueue_.empty()) {
        LLMRequest req = mediumPriorityQueue_.front();
        mediumPriorityQueue_.pop_front();
        return nullptr;
    }

    if (!lowPriorityQueue_.empty()) {
        LLMRequest req = lowPriorityQueue_.front();
        lowPriorityQueue_.pop_front();
        return nullptr;
    }

    return nullptr;  // No requests
}

bool LLMRequestQueue::hasReadyRequests(int64_t currentTick) const {
    // Check each queue for non-timed-out requests
    for (const auto& req : highPriorityQueue_) {
        if (currentTick - req.enqueuedTick <= req.timeoutTicks) {
            return true;
        }
    }
    for (const auto& req : mediumPriorityQueue_) {
        if (currentTick - req.enqueuedTick <= req.timeoutTicks) {
            return true;
        }
    }
    for (const auto& req : lowPriorityQueue_) {
        if (currentTick - req.enqueuedTick <= req.timeoutTicks) {
            return true;
        }
    }
    return false;
}

size_t LLMRequestQueue::getQueueSizeForPriority(LLMRequestPriority priority) const {
    switch (priority) {
        case LLMRequestPriority::HIGH:
            return highPriorityQueue_.size();
        case LLMRequestPriority::MEDIUM:
            return mediumPriorityQueue_.size();
        case LLMRequestPriority::LOW:
            return lowPriorityQueue_.size();
        default:
            return 0;
    }
}

size_t LLMRequestQueue::getTotalQueueSize() const {
    return highPriorityQueue_.size() + 
           mediumPriorityQueue_.size() + 
           lowPriorityQueue_.size();
}

void LLMRequestQueue::clear() {
    highPriorityQueue_.clear();
    mediumPriorityQueue_.clear();
    lowPriorityQueue_.clear();
}

int LLMRequestQueue::processTimeouts(int64_t currentTick) {
    int timedOut = 0;

    // Check high priority queue
    for (auto it = highPriorityQueue_.begin(); it != highPriorityQueue_.end(); ) {
        if (currentTick - it->enqueuedTick > it->timeoutTicks) {
            it = highPriorityQueue_.erase(it);
            timedOut++;
        } else {
            ++it;
        }
    }

    // Check medium priority queue
    for (auto it = mediumPriorityQueue_.begin(); it != mediumPriorityQueue_.end(); ) {
        if (currentTick - it->enqueuedTick > it->timeoutTicks) {
            it = mediumPriorityQueue_.erase(it);
            timedOut++;
        } else {
            ++it;
        }
    }

    // Check low priority queue
    for (auto it = lowPriorityQueue_.begin(); it != lowPriorityQueue_.end(); ) {
        if (currentTick - it->enqueuedTick > it->timeoutTicks) {
            it = lowPriorityQueue_.erase(it);
            timedOut++;
        } else {
            ++it;
        }
    }
    
    return timedOut;
}

int LLMRequestQueue::getTimeoutTicksForPriority(LLMRequestPriority priority) {
    switch (priority) {
        case LLMRequestPriority::HIGH:
            return 180;  // 3 seconds at 60 ticks/second
        case LLMRequestPriority::MEDIUM:
            return 600;  // 10 seconds
        case LLMRequestPriority::LOW:
            return 300;  // 5 seconds
        default:
            return 0;
    }
}

std::deque<LLMRequest>& LLMRequestQueue::getQueueForPriority(LLMRequestPriority priority) {
    switch (priority) {
        case LLMRequestPriority::HIGH:
            return highPriorityQueue_;
        case LLMRequestPriority::MEDIUM:
            return mediumPriorityQueue_;
        case LLMRequestPriority::LOW:
            return lowPriorityQueue_;
        default:
            return mediumPriorityQueue_;  // Default fallback
    }
}

const std::deque<LLMRequest>& LLMRequestQueue::getQueueForPriority(LLMRequestPriority priority) const {
    switch (priority) {
        case LLMRequestPriority::HIGH:
            return highPriorityQueue_;
        case LLMRequestPriority::MEDIUM:
            return mediumPriorityQueue_;
        case LLMRequestPriority::LOW:
            return lowPriorityQueue_;
        default:
            return mediumPriorityQueue_;  // Default fallback
    }
}

size_t LLMRequestQueue::getMaxSizeForPriority(LLMRequestPriority priority) const {
    switch (priority) {
        case LLMRequestPriority::HIGH:
            return MAX_HIGH_QUEUE;
        case LLMRequestPriority::MEDIUM:
            return MAX_MEDIUM_QUEUE;
        case LLMRequestPriority::LOW:
            return MAX_LOW_QUEUE;
        default:
            return MAX_MEDIUM_QUEUE;
    }
}

bool LLMRequestQueue::isDuplicate(const std::string& prompt) const {
    for (const auto& req : highPriorityQueue_) {
        if (req.prompt == prompt) return true;
    }
    for (const auto& req : mediumPriorityQueue_) {
        if (req.prompt == prompt) return true;
    }
    for (const auto& req : lowPriorityQueue_) {
        if (req.prompt == prompt) return true;
    }
    return false;
}

}  // namespace TLS
