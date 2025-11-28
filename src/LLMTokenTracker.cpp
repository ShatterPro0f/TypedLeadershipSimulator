#include "LLMTokenTracker.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <ctime>

namespace TLS {

// ==================== TokenCount ====================

std::string TokenCount::toString() const {
    std::ostringstream oss;
    oss << "Tokens{input=" << inputTokens << ", completion=" << completionTokens 
        << ", total=" << getTotalTokens() << "}";
    return oss.str();
}

// ==================== ModelPricing ====================

float ModelPricing::calculateCost(int inputTokens, int completionTokens) const {
    float inputCost = (inputTokens / 1000.0f) * inputCostPer1kTokens;
    float completionCost = (completionTokens / 1000.0f) * completionCostPer1kTokens;
    return inputCost + completionCost;
}

std::string ModelPricing::toString() const {
    std::ostringstream oss;
    oss << modelName << " - $" << inputCostPer1kTokens << "/1k input, "
        << "$" << completionCostPer1kTokens << "/1k completion";
    return oss.str();
}

// ==================== TokenUsageEntry ====================

std::string TokenUsageEntry::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"timestamp\": " << timestamp << ",\n";
    oss << "  \"modelName\": \"" << modelName << "\",\n";
    oss << "  \"callType\": " << static_cast<int>(callType) << ",\n";
    oss << "  \"inputTokens\": " << inputTokens << ",\n";
    oss << "  \"completionTokens\": " << completionTokens << ",\n";
    oss << "  \"costUSD\": " << std::fixed << std::setprecision(6) << costUSD << ",\n";
    oss << "  \"wasSuccessful\": " << (wasSuccessful ? "true" : "false") << "\n";
    oss << "}";
    return oss.str();
}

// ==================== TokenTracker ====================

TokenTracker& TokenTracker::getInstance() {
    static TokenTracker instance;
    return instance;
}

TokenTracker::TokenTracker() {
    initializeDefaultPricing();
}

void TokenTracker::enableTracking() {
    trackingEnabled_ = true;
}

void TokenTracker::disableTracking() {
    trackingEnabled_ = false;
}

void TokenTracker::recordUsage(
    const std::string& modelName,
    LLMCallType callType,
    int inputTokens,
    int completionTokens,
    bool wasSuccessful
) {
    if (!trackingEnabled_) return;

    TokenUsageEntry entry;
    entry.timestamp = getCurrentTimestamp();
    entry.modelName = modelName;
    entry.callType = callType;
    entry.inputTokens = inputTokens;
    entry.completionTokens = completionTokens;
    entry.wasSuccessful = wasSuccessful;

    // Calculate cost
    const ModelPricing* pricing = getModelPricing(modelName);
    if (pricing) {
        entry.costUSD = pricing->calculateCost(inputTokens, completionTokens);
    } else {
        entry.costUSD = 0.0f;  // Unknown model, free
    }

    usageEntries_.push_back(entry);

    // Update aggregated statistics
    totalInputTokens_ += inputTokens;
    totalCompletionTokens_ += completionTokens;
    totalCalls_++;
    if (wasSuccessful) {
        successfulCalls_++;
    } else {
        failedCalls_++;
    }
    totalEstimatedCostUSD_ += entry.costUSD;
}

int64_t TokenTracker::getTotalInputTokens() const {
    return totalInputTokens_;
}

int64_t TokenTracker::getTotalCompletionTokens() const {
    return totalCompletionTokens_;
}

int64_t TokenTracker::getTotalTokens() const {
    return totalInputTokens_ + totalCompletionTokens_;
}

TokenCount TokenTracker::getUsageByModel(const std::string& modelName) const {
    TokenCount count{0, 0};
    for (const auto& entry : usageEntries_) {
        if (entry.modelName == modelName) {
            count.inputTokens += entry.inputTokens;
            count.completionTokens += entry.completionTokens;
        }
    }
    return count;
}

TokenCount TokenTracker::getUsageByCallType(LLMCallType type) const {
    TokenCount count{0, 0};
    for (const auto& entry : usageEntries_) {
        if (entry.callType == type) {
            count.inputTokens += entry.inputTokens;
            count.completionTokens += entry.completionTokens;
        }
    }
    return count;
}

TokenCount TokenTracker::getAverageTokensPerCall() const {
    if (totalCalls_ == 0) return {0, 0};
    
    return {
        static_cast<int>(totalInputTokens_ / totalCalls_),
        static_cast<int>(totalCompletionTokens_ / totalCalls_)
    };
}

float TokenTracker::estimateCost(
    const std::string& modelName,
    int inputTokens,
    int completionTokens
) const {
    const ModelPricing* pricing = getModelPricing(modelName);
    if (pricing) {
        return pricing->calculateCost(inputTokens, completionTokens);
    }
    return 0.0f;
}

void TokenTracker::registerModel(const ModelPricing& pricing) {
    modelPricing_[pricing.modelName] = pricing;
}

const ModelPricing* TokenTracker::getModelPricing(const std::string& modelName) const {
    auto it = modelPricing_.find(modelName);
    if (it != modelPricing_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string TokenTracker::getUsageStatistics() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"totalInputTokens\": " << totalInputTokens_ << ",\n";
    oss << "  \"totalCompletionTokens\": " << totalCompletionTokens_ << ",\n";
    oss << "  \"totalTokens\": " << getTotalTokens() << ",\n";
    oss << "  \"totalCalls\": " << totalCalls_ << ",\n";
    oss << "  \"successfulCalls\": " << successfulCalls_ << ",\n";
    oss << "  \"failedCalls\": " << failedCalls_ << ",\n";
    oss << "  \"estimatedCostUSD\": " << std::fixed << std::setprecision(6) << totalEstimatedCostUSD_ << ",\n";
    oss << "  \"budgetLimit\": " << budgetLimitUSD_ << ",\n";
    oss << "  \"remainingBudget\": " << getRemainingBudget() << "\n";
    oss << "}";
    return oss.str();
}

std::string TokenTracker::getUsageSummary() const {
    std::ostringstream oss;
    oss << "Token Usage Summary:\n";
    oss << "  Total Tokens: " << getTotalTokens() << "\n";
    oss << "    - Input: " << totalInputTokens_ << "\n";
    oss << "    - Completion: " << totalCompletionTokens_ << "\n";
    oss << "  Total API Calls: " << totalCalls_ << "\n";
    oss << "    - Successful: " << successfulCalls_ << "\n";
    oss << "    - Failed: " << failedCalls_ << "\n";
    oss << "  Estimated Cost: $" << std::fixed << std::setprecision(4) << totalEstimatedCostUSD_ << "\n";
    
    if (budgetLimitUSD_ > 0) {
        oss << "  Budget Status:\n";
        oss << "    - Limit: $" << std::fixed << std::setprecision(2) << budgetLimitUSD_ << "\n";
        oss << "    - Remaining: $" << std::fixed << std::setprecision(2) << getRemainingBudget() << "\n";
        oss << "    - Usage: " << std::fixed << std::setprecision(1) 
            << ((totalEstimatedCostUSD_ / budgetLimitUSD_) * 100.0f) << "%\n";
    }
    
    if (totalCalls_ > 0) {
        oss << "  Average Cost per Call: $" << std::fixed << std::setprecision(6) 
            << (totalEstimatedCostUSD_ / totalCalls_) << "\n";
    }
    
    return oss.str();
}

TokenCount TokenTracker::getHourlyUsage(int hoursAgo) const {
    TokenCount count{0, 0};
    int64_t now = getCurrentTimestamp();
    int64_t targetTime = now - (hoursAgo * 3600000);  // Convert hours to ms
    
    for (const auto& entry : usageEntries_) {
        if (entry.timestamp >= targetTime && entry.timestamp <= now) {
            count.inputTokens += entry.inputTokens;
            count.completionTokens += entry.completionTokens;
        }
    }
    return count;
}

TokenCount TokenTracker::getDailyUsage(int daysAgo) const {
    TokenCount count{0, 0};
    int64_t now = getCurrentTimestamp();
    int64_t targetTime = now - (daysAgo * 86400000);  // Convert days to ms
    
    for (const auto& entry : usageEntries_) {
        if (entry.timestamp >= targetTime && entry.timestamp <= now) {
            count.inputTokens += entry.inputTokens;
            count.completionTokens += entry.completionTokens;
        }
    }
    return count;
}

bool TokenTracker::exportUsageReport(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << getUsageSummary() << "\n\n";
    file << "Detailed Usage Entries:\n";
    file << "[\n";

    for (size_t i = 0; i < usageEntries_.size(); ++i) {
        file << usageEntries_[i].toJson();
        if (i < usageEntries_.size() - 1) {
            file << ",\n";
        }
    }

    file << "\n]\n";
    file.close();
    return true;
}

void TokenTracker::reset() {
    usageEntries_.clear();
    totalInputTokens_ = 0;
    totalCompletionTokens_ = 0;
    totalCalls_ = 0;
    successfulCalls_ = 0;
    failedCalls_ = 0;
    totalEstimatedCostUSD_ = 0.0f;
}

void TokenTracker::initializeDefaultPricing() {
    // OpenAI pricing (as of 2024)
    registerModel({
        "gpt-4",
        0.03f,    // $0.03 per 1K input tokens
        0.06f     // $0.06 per 1K completion tokens
    });

    registerModel({
        "gpt-3.5-turbo",
        0.0005f,  // $0.0005 per 1K input tokens
        0.0015f   // $0.0015 per 1K completion tokens
    });

    // Local LLaMA - free
    registerModel({
        "llama",
        0.0f,
        0.0f
    });

    // Offline fallback - free
    registerModel({
        "offline-fallback",
        0.0f,
        0.0f
    });
}

int64_t TokenTracker::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// ==================== UsageReport ====================

UsageReport UsageReport::generate() {
    UsageReport report;
    
    TokenTracker& tracker = TokenTracker::getInstance();
    report.totalInputTokens = tracker.getTotalInputTokens();
    report.totalCompletionTokens = tracker.getTotalCompletionTokens();
    report.totalApiCalls = tracker.getTotalApiCalls();
    report.estimatedCostUSD = tracker.getTotalEstimatedCost();
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    report.reportGeneratedAt = ss.str();
    
    // Calculate per-call metrics
    if (report.totalApiCalls > 0) {
        report.averageTokensPerCall = static_cast<float>(
            report.totalInputTokens + report.totalCompletionTokens
        ) / static_cast<float>(report.totalApiCalls);
        report.costPerCall = report.estimatedCostUSD / report.totalApiCalls;
    }
    
    return report;
}

std::string UsageReport::format() const {
    std::ostringstream oss;
    oss << "===== Usage Report =====\n";
    oss << "Generated: " << reportGeneratedAt << "\n\n";
    oss << "Token Usage:\n";
    oss << "  Total Input Tokens: " << totalInputTokens << "\n";
    oss << "  Total Completion Tokens: " << totalCompletionTokens << "\n";
    oss << "  Total Tokens: " << (totalInputTokens + totalCompletionTokens) << "\n\n";
    oss << "API Calls:\n";
    oss << "  Total Calls: " << totalApiCalls << "\n";
    oss << "  Average Tokens per Call: " << std::fixed << std::setprecision(1) 
        << averageTokensPerCall << "\n\n";
    oss << "Cost:\n";
    oss << "  Estimated Total Cost: $" << std::fixed << std::setprecision(4) 
        << estimatedCostUSD << "\n";
    oss << "  Average Cost per Call: $" << std::fixed << std::setprecision(6) 
        << costPerCall << "\n";
    oss << "=======================\n";
    return oss.str();
}

std::string UsageReport::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"reportGeneratedAt\": \"" << reportGeneratedAt << "\",\n";
    oss << "  \"totalInputTokens\": " << totalInputTokens << ",\n";
    oss << "  \"totalCompletionTokens\": " << totalCompletionTokens << ",\n";
    oss << "  \"totalApiCalls\": " << totalApiCalls << ",\n";
    oss << "  \"estimatedCostUSD\": " << std::fixed << std::setprecision(4) << estimatedCostUSD << ",\n";
    oss << "  \"averageTokensPerCall\": " << std::fixed << std::setprecision(1) << averageTokensPerCall << ",\n";
    oss << "  \"costPerCall\": " << std::fixed << std::setprecision(6) << costPerCall << "\n";
    oss << "}";
    return oss.str();
}

bool UsageReport::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << format();
    file.close();
    return true;
}

}  // namespace TLS
