#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include "LLMDeterministicReplay.h"  // For LLMCallType enum

namespace TLS {

/**
 * @struct TokenCount
 * @brief Token count for a single LLM call
 */
struct TokenCount {
    int inputTokens = 0;
    int completionTokens = 0;

    int getTotalTokens() const {
        return inputTokens + completionTokens;
    }

    std::string toString() const;
};

/**
 * @struct ModelPricing
 * @brief Pricing information for an LLM model
 */
struct ModelPricing {
    std::string modelName;
    float inputCostPer1kTokens = 0.0f;    // USD
    float completionCostPer1kTokens = 0.0f;

    float calculateCost(int inputTokens, int completionTokens) const;
    std::string toString() const;
};

/**
 * @struct TokenUsageEntry
 * @brief Single token usage entry
 */
struct TokenUsageEntry {
    int64_t timestamp = 0;
    std::string modelName;
    LLMCallType callType = LLMCallType::UNKNOWN;
    int inputTokens = 0;
    int completionTokens = 0;
    float costUSD = 0.0f;
    bool wasSuccessful = true;

    std::string toJson() const;
};

/**
 * @class TokenTracker
 * @brief Tracks LLM token usage and estimates costs
 *
 * Features:
 * - Per-model token counting
 * - Cost estimation (USD)
 * - Daily/hourly aggregation
 * - Budget alerts
 * - Usage reporting
 *
 * Pricing Models (configurable):
 * - OpenAI GPT-4: $0.03 per 1K input, $0.06 per 1K completion
 * - OpenAI GPT-3.5: $0.0005 per 1K input, $0.0015 per 1K completion
 * - Local LLaMA: $0 (free)
 */
class TokenTracker {
public:
    /**
     * Singleton instance
     */
    static TokenTracker& getInstance();

    /**
     * Enable token tracking
     */
    void enableTracking();

    /**
     * Disable tracking
     */
    void disableTracking();

    /**
     * Check if tracking is enabled
     */
    bool isTrackingEnabled() const { return trackingEnabled_; }

    /**
     * Record token usage
     * @param modelName Name of model used
     * @param callType Type of LLM call
     * @param inputTokens Number of input tokens
     * @param completionTokens Number of completion tokens
     * @param wasSuccessful Whether call was successful
     */
    void recordUsage(
        const std::string& modelName,
        LLMCallType callType,
        int inputTokens,
        int completionTokens,
        bool wasSuccessful = true
    );

    /**
     * Get total tokens used
     */
    int64_t getTotalInputTokens() const;
    int64_t getTotalCompletionTokens() const;
    int64_t getTotalTokens() const;

    /**
     * Get total API calls made
     */
    int getTotalApiCalls() const { return totalCalls_; }

    /**
     * Get total estimated cost (USD)
     */
    float getTotalEstimatedCost() const { return totalEstimatedCostUSD_; }

    /**
     * Get usage by model
     */
    TokenCount getUsageByModel(const std::string& modelName) const;

    /**
     * Get usage by call type
     */
    TokenCount getUsageByCallType(LLMCallType type) const;

    /**
     * Get average tokens per call
     */
    TokenCount getAverageTokensPerCall() const;

    /**
     * Get estimated cost for given tokens
     */
    float estimateCost(
        const std::string& modelName,
        int inputTokens,
        int completionTokens
    ) const;

    /**
     * Set budget limit (in USD)
     * @param budgetUSD Budget in US dollars
     */
    void setBudgetLimit(float budgetUSD) { budgetLimitUSD_ = budgetUSD; }

    /**
     * Get budget limit
     */
    float getBudgetLimit() const { return budgetLimitUSD_; }

    /**
     * Check if budget exceeded
     */
    bool isBudgetExceeded() const {
        return (budgetLimitUSD_ > 0) && (totalEstimatedCostUSD_ > budgetLimitUSD_);
    }

    /**
     * Get remaining budget
     */
    float getRemainingBudget() const {
        if (budgetLimitUSD_ <= 0) return -1.0f;
        return budgetLimitUSD_ - totalEstimatedCostUSD_;
    }

    /**
     * Get budget alert threshold (percentage, e.g., 0.8 = 80%)
     */
    float getBudgetAlertThreshold() const { return budgetAlertThreshold_; }

    /**
     * Set budget alert threshold
     */
    void setBudgetAlertThreshold(float threshold) {
        budgetAlertThreshold_ = threshold;
    }

    /**
     * Check if budget alert should trigger
     */
    bool shouldAlertBudget() const {
        if (budgetLimitUSD_ <= 0) return false;
        return (totalEstimatedCostUSD_ / budgetLimitUSD_) >= budgetAlertThreshold_;
    }

    /**
     * Register model pricing
     */
    void registerModel(const ModelPricing& pricing);

    /**
     * Get pricing for model
     */
    const ModelPricing* getModelPricing(const std::string& modelName) const;

    /**
     * Get usage statistics as JSON
     */
    std::string getUsageStatistics() const;

    /**
     * Get usage statistics as human-readable string
     */
    std::string getUsageSummary() const;

    /**
     * Get hourly usage
     */
    TokenCount getHourlyUsage(int hoursAgo = 0) const;

    /**
     * Get daily usage
     */
    TokenCount getDailyUsage(int daysAgo = 0) const;

    /**
     * Export usage report to file
     */
    bool exportUsageReport(const std::string& filepath) const;

    /**
     * Clear all tracked usage
     */
    void reset();

    /**
     * Get all usage entries
     */
    const std::vector<TokenUsageEntry>& getAllEntries() const {
        return usageEntries_;
    }

private:
    TokenTracker();

    std::vector<TokenUsageEntry> usageEntries_;
    std::map<std::string, ModelPricing> modelPricing_;
    
    // Aggregated statistics
    int64_t totalInputTokens_ = 0;
    int64_t totalCompletionTokens_ = 0;
    int totalCalls_ = 0;
    int successfulCalls_ = 0;
    int failedCalls_ = 0;
    float totalEstimatedCostUSD_ = 0.0f;
    
    // Budget management
    float budgetLimitUSD_ = -1.0f;  // -1 = no limit
    float budgetAlertThreshold_ = 0.8f;
    
    bool trackingEnabled_ = true;

    void initializeDefaultPricing();
    int64_t getCurrentTimestamp() const;
};

/**
 * @class UsageReport
 * @brief Formatted usage report for export/display
 */
class UsageReport {
public:
    /**
     * Generate usage report
     */
    static UsageReport generate();

    /**
     * Get report as formatted string
     */
    std::string format() const;

    /**
     * Get report as JSON
     */
    std::string toJson() const;

    /**
     * Save report to file
     */
    bool saveToFile(const std::string& filepath) const;

    // Report data
    int64_t totalInputTokens = 0;
    int64_t totalCompletionTokens = 0;
    int totalApiCalls = 0;
    float estimatedCostUSD = 0.0f;
    std::string reportGeneratedAt;
    std::map<std::string, TokenCount> usageByModel;
    std::map<std::string, TokenCount> usageByCallType;
    float averageTokensPerCall = 0.0f;
    float costPerCall = 0.0f;
};

}  // namespace TLS
