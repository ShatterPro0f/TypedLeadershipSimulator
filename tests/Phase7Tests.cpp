#include <gtest/gtest.h>
#include "LLMProvider.h"
#include "LLMConfigManager.h"
#include "LLMRequestQueue.h"
#include "LLMResponseCache.h"
#include "LLMErrorRecovery.h"
#include "LLMDeterministicReplay.h"
#include "LLMTokenTracker.h"
#include <chrono>
#include <thread>

using namespace TLS;

// ==================== Test Suite 1: Provider Factory & Creation ====================

class ProviderFactoryTests : public ::testing::Test {
protected:
    void SetUp() override {
        config_.preferredProvider = LLMProviderType::OPENAI;
        config_.apiKey = "test-key-12345";
        config_.apiEndpoint = "https://api.openai.com/v1";
        config_.timeoutSeconds = 10;
        config_.maxRetries = 3;
    }

    LLMConfig config_;
};

TEST_F(ProviderFactoryTests, CreateOpenAIProvider) {
    auto provider = LLMProviderFactory::createProvider(LLMProviderType::OPENAI, config_);
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->getProviderType(), LLMProviderType::OPENAI);
    EXPECT_EQ(provider->getProviderName(), "OpenAI");
}

TEST_F(ProviderFactoryTests, CreateLocalLlamaProvider) {
    config_.llamaServerUrl = "http://localhost:8000";
    auto provider = LLMProviderFactory::createProvider(LLMProviderType::LLAMA_LOCAL, config_);
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->getProviderType(), LLMProviderType::LLAMA_LOCAL);
    EXPECT_EQ(provider->getProviderName(), "LocalLLaMA");
}

TEST_F(ProviderFactoryTests, CreateOfflineFallbackProvider) {
    auto provider = LLMProviderFactory::createProvider(LLMProviderType::OFFLINE_FALLBACK, config_);
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->getProviderType(), LLMProviderType::OFFLINE_FALLBACK);
    EXPECT_EQ(provider->getProviderName(), "OfflineFallback");
    EXPECT_TRUE(provider->isAvailable());  // Always available
}

TEST_F(ProviderFactoryTests, ProviderTypeToString) {
    EXPECT_EQ(LLMProviderFactory::providerTypeToString(LLMProviderType::OPENAI), "OpenAI");
    EXPECT_EQ(LLMProviderFactory::providerTypeToString(LLMProviderType::LLAMA_LOCAL), "LocalLLaMA");
    EXPECT_EQ(LLMProviderFactory::providerTypeToString(LLMProviderType::OFFLINE_FALLBACK), "OfflineFallback");
}

TEST_F(ProviderFactoryTests, StringToProviderType) {
    EXPECT_EQ(LLMProviderFactory::stringToProviderType("openai"), LLMProviderType::OPENAI);
    EXPECT_EQ(LLMProviderFactory::stringToProviderType("llama"), LLMProviderType::LLAMA_LOCAL);
    EXPECT_EQ(LLMProviderFactory::stringToProviderType("offline"), LLMProviderType::OFFLINE_FALLBACK);
}

// ==================== Test Suite 2: Configuration Loading ====================

class ConfigurationLoadingTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Set environment variables for testing
        _putenv_s("LLM_PROVIDER", "openai");
        _putenv_s("OPENAI_API_KEY", "test-api-key");
    }

    void TearDown() override {
        _putenv_s("LLM_PROVIDER", "");
        _putenv_s("OPENAI_API_KEY", "");
        _putenv_s("LLAMA_SERVER_URL", "");
    }
};

TEST_F(ConfigurationLoadingTests, GetDefaultConfiguration) {
    LLMConfig config = LLMConfigManager::getDefaultConfiguration();
    EXPECT_EQ(config.preferredProvider, LLMProviderType::OPENAI);
    EXPECT_EQ(config.timeoutSeconds, 10);
    EXPECT_EQ(config.maxRetries, 3);
    EXPECT_TRUE(config.fallbackEnabled);
    EXPECT_EQ(config.rateLimitPerMinute, 60.0f);
}

TEST_F(ConfigurationLoadingTests, ValidateConfiguration_Valid) {
    LLMConfig config = LLMConfigManager::getDefaultConfiguration();
    EXPECT_TRUE(LLMConfigManager::validateConfiguration(config));
}

TEST_F(ConfigurationLoadingTests, ValidateConfiguration_InvalidTimeout) {
    LLMConfig config = LLMConfigManager::getDefaultConfiguration();
    config.timeoutSeconds = -1;
    EXPECT_FALSE(LLMConfigManager::validateConfiguration(config));
}

TEST_F(ConfigurationLoadingTests, ValidateConfiguration_InvalidMaxRetries) {
    LLMConfig config = LLMConfigManager::getDefaultConfiguration();
    config.maxRetries = 100;  // Too many
    EXPECT_FALSE(LLMConfigManager::validateConfiguration(config));
}

TEST_F(ConfigurationLoadingTests, MergeConfigurations) {
    LLMConfig base = LLMConfigManager::getDefaultConfiguration();
    LLMConfig override;
    override.preferredProvider = LLMProviderType::LLAMA_LOCAL;
    override.timeoutSeconds = 20;

    LLMConfig merged = LLMConfigManager::mergeConfigurations(base, override);
    EXPECT_EQ(merged.preferredProvider, LLMProviderType::LLAMA_LOCAL);
    EXPECT_EQ(merged.timeoutSeconds, 20);
}

// ==================== Test Suite 3: Offline Fallback Generation ====================

class OfflineFallbackTests : public ::testing::Test {
};

TEST_F(OfflineFallbackTests, GenerateNpcNarrative) {
    std::string narrative = OfflineLLMFallback::generateNpcNarrative("RESOURCE_SCARCITY", 0.8f);
    EXPECT_FALSE(narrative.empty());
    EXPECT_NE(narrative.find("scarce"), std::string::npos);
}

TEST_F(OfflineFallbackTests, InterpretDecision_Allocate) {
    std::string result = OfflineLLMFallback::interpretDecision("allocate food to farmers", "");
    EXPECT_NE(result.find("allocate"), std::string::npos);
}

TEST_F(OfflineFallbackTests, InterpretDecision_Negotiate) {
    std::string result = OfflineLLMFallback::interpretDecision("negotiate peace", "");
    EXPECT_NE(result.find("negotiate"), std::string::npos);
}

TEST_F(OfflineFallbackTests, GenerateNpcDialogue_Farmer) {
    std::string dialogue = OfflineLLMFallback::generateNpcDialogue("Alice", "Farmer", "food");
    EXPECT_NE(dialogue.find("Alice"), std::string::npos);
    EXPECT_NE(dialogue.find("Farmer"), std::string::npos);
    EXPECT_FALSE(dialogue.empty());
}

TEST_F(OfflineFallbackTests, GenerateNpcDialogue_Warrior) {
    std::string dialogue = OfflineLLMFallback::generateNpcDialogue("Bob", "Warrior", "conflict");
    EXPECT_NE(dialogue.find("Bob"), std::string::npos);
    EXPECT_NE(dialogue.find("Warrior"), std::string::npos);
}

TEST_F(OfflineFallbackTests, GenerateCrisisNarrative_Famine) {
    std::string narrative = OfflineLLMFallback::generateCrisisNarrative("famine", 0.9f);
    EXPECT_FALSE(narrative.empty());
}

TEST_F(OfflineFallbackTests, AnalyzeWorldState) {
    std::string analysis = OfflineLLMFallback::analyzeWorldState("{\"food\": 50, \"conflict\": true}");
    EXPECT_FALSE(analysis.empty());
}

TEST_F(OfflineFallbackTests, SeverityScaling_Low) {
    std::string result = OfflineLLMFallback::generateCrisisNarrative("test", 0.2f);
    EXPECT_NE(result.find("Minor"), std::string::npos);
}

TEST_F(OfflineFallbackTests, SeverityScaling_Critical) {
    std::string result = OfflineLLMFallback::generateCrisisNarrative("test", 0.95f);
    EXPECT_NE(result.find("CRITICAL"), std::string::npos);
}

// ==================== Test Suite 4: Response Caching ====================

class ResponseCacheTests : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<LLMResponseCache>(300);  // 5 minute TTL
    }

    std::unique_ptr<LLMResponseCache> cache_;
};

TEST_F(ResponseCacheTests, CachePutAndGet) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test response";
    response.inputTokens = 10;
    response.completionTokens = 20;

    EXPECT_TRUE(cache_->put("test prompt", response));
    const LLMResponse* cached = cache_->get("test prompt");
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->text, "Test response");
}

TEST_F(ResponseCacheTests, CacheContains) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";
    cache_->put("prompt1", response);

    EXPECT_TRUE(cache_->contains("prompt1"));
    EXPECT_FALSE(cache_->contains("nonexistent"));
}

TEST_F(ResponseCacheTests, CacheInvalidate) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";
    cache_->put("prompt1", response);

    cache_->invalidate("prompt1");
    EXPECT_FALSE(cache_->contains("prompt1"));
}

TEST_F(ResponseCacheTests, CacheHitRate) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";
    cache_->put("prompt", response);

    cache_->get("prompt");  // Hit
    cache_->get("prompt");  // Hit
    cache_->get("other");   // Miss

    float hitRate = cache_->getHitRate();
    EXPECT_GT(hitRate, 0.5f);
}

TEST_F(ResponseCacheTests, CacheClear) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";
    cache_->put("prompt1", response);
    cache_->put("prompt2", response);

    cache_->clear();
    EXPECT_FALSE(cache_->contains("prompt1"));
    EXPECT_FALSE(cache_->contains("prompt2"));
}

TEST_F(ResponseCacheTests, CacheStatistics) {
    std::string stats = cache_->getStatistics();
    EXPECT_NE(stats.find("Cache Statistics"), std::string::npos);
    EXPECT_NE(stats.find("Hit Rate"), std::string::npos);
}

TEST_F(ResponseCacheTests, CacheEvictExpired) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";
    cache_->put("prompt", response);

    cache_->evictExpired();
    // Should still contain if not expired
    // Real expiration would need time-based testing
}

// ==================== Test Suite 5: Token Tracking ====================

class TokenTrackerTests : public ::testing::Test {
protected:
    void SetUp() override {
        TokenTracker::getInstance().reset();
        TokenTracker::getInstance().enableTracking();
    }

    void TearDown() override {
        TokenTracker::getInstance().reset();
    }
};

TEST_F(TokenTrackerTests, RecordUsage) {
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::DECISION_INTERPRETATION, 100, 50, true);
    EXPECT_EQ(TokenTracker::getInstance().getTotalInputTokens(), 100);
    EXPECT_EQ(TokenTracker::getInstance().getTotalCompletionTokens(), 50);
}

TEST_F(TokenTrackerTests, GetTotalTokens) {
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::WORLD_STATE_NARRATIVE, 100, 50, true);
    EXPECT_EQ(TokenTracker::getInstance().getTotalTokens(), 150);
}

TEST_F(TokenTrackerTests, GetUsageByModel) {
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::DECISION_INTERPRETATION, 100, 50, true);
    TokenTracker::getInstance().recordUsage("gpt-3.5-turbo", LLMCallType::NPC_CONVERSATION, 50, 25, true);

    TokenCount gpt4 = TokenTracker::getInstance().getUsageByModel("gpt-4");
    EXPECT_EQ(gpt4.inputTokens, 100);
    EXPECT_EQ(gpt4.completionTokens, 50);
}

TEST_F(TokenTrackerTests, GetUsageByCallType) {
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::DECISION_INTERPRETATION, 100, 50, true);
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::NPC_CONVERSATION, 75, 25, true);

    TokenCount decision = TokenTracker::getInstance().getUsageByCallType(LLMCallType::DECISION_INTERPRETATION);
    EXPECT_EQ(decision.inputTokens, 100);
}

TEST_F(TokenTrackerTests, BudgetManagement) {
    TokenTracker::getInstance().setBudgetLimit(10.0f);
    EXPECT_EQ(TokenTracker::getInstance().getBudgetLimit(), 10.0f);
    EXPECT_FALSE(TokenTracker::getInstance().isBudgetExceeded());
}

TEST_F(TokenTrackerTests, GetUsageSummary) {
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::DECISION_INTERPRETATION, 100, 50, true);
    std::string summary = TokenTracker::getInstance().getUsageSummary();
    EXPECT_NE(summary.find("Total Tokens"), std::string::npos);
}

// ==================== Test Suite 6: Exponential Backoff & Retry ====================

class ExponentialBackoffTests : public ::testing::Test {
};

TEST_F(ExponentialBackoffTests, CalculateDelay_Attempt0) {
    ExponentialBackoffCalculator calc(1000, 30000, false, 0.0f);
    int delay = calc.calculateDelay(0);
    EXPECT_EQ(delay, 1000);  // base * 2^0 = 1000
}

TEST_F(ExponentialBackoffTests, CalculateDelay_Attempt1) {
    ExponentialBackoffCalculator calc(1000, 30000, false, 0.0f);
    int delay = calc.calculateDelay(1);
    EXPECT_EQ(delay, 2000);  // base * 2^1 = 2000
}

TEST_F(ExponentialBackoffTests, CalculateDelay_Attempt2) {
    ExponentialBackoffCalculator calc(1000, 30000, false, 0.0f);
    int delay = calc.calculateDelay(2);
    EXPECT_EQ(delay, 4000);  // base * 2^2 = 4000
}

TEST_F(ExponentialBackoffTests, CalculateDelay_Capped) {
    ExponentialBackoffCalculator calc(1000, 5000, false, 0.0f);
    int delay = calc.calculateDelay(10);
    EXPECT_LE(delay, 5000);  // Capped at maxDelay
}

TEST_F(ExponentialBackoffTests, CalculateDelay_WithJitter) {
    ExponentialBackoffCalculator calc(1000, 30000, true, 0.1f);
    int delay = calc.calculateDelay(1);
    // Should be close to 2000, but with jitter ±10%
    EXPECT_GE(delay, 1800);
    EXPECT_LE(delay, 2200);
}

TEST_F(ExponentialBackoffTests, GetNextRetryTime) {
    ExponentialBackoffCalculator calc(1000, 30000, false, 0.0f);
    int64_t currentTime = 1000000;
    int64_t nextRetry = calc.getNextRetryTime(currentTime, 0);
    EXPECT_EQ(nextRetry, currentTime + 1000);
}

// ==================== Test Suite 7: Rate Limiting ====================

class RateLimiterTests : public ::testing::Test {
protected:
    void SetUp() override {
        limiter_ = std::make_unique<TokenBucketRateLimiter>(60.0f);  // 60 tokens/min
    }

    std::unique_ptr<TokenBucketRateLimiter> limiter_;
};

TEST_F(RateLimiterTests, CanMakeRequest_InitiallyTrue) {
    EXPECT_TRUE(limiter_->canMakeRequest());
}

TEST_F(RateLimiterTests, GetAvailableTokens_Initial) {
    float available = limiter_->getAvailableTokens();
    EXPECT_FLOAT_EQ(available, 60.0f);
}

TEST_F(RateLimiterTests, CanMakeRequest_MultipleRequests) {
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(limiter_->canMakeRequest());
    }
    EXPECT_FALSE(limiter_->canMakeRequest());  // 61st should fail
}

TEST_F(RateLimiterTests, GetWaitTimeSeconds_NoWait) {
    EXPECT_EQ(limiter_->getWaitTimeSeconds(), 0);
}

TEST_F(RateLimiterTests, Reset) {
    limiter_->canMakeRequest();
    limiter_->canMakeRequest();
    limiter_->reset();
    EXPECT_FLOAT_EQ(limiter_->getAvailableTokens(), 60.0f);
}

TEST_F(RateLimiterTests, SetRateLimit) {
    limiter_->setRateLimit(120.0f);
    float available = limiter_->getAvailableTokens();
    // After setRateLimit(120), max is 120, so available should be between 50-120
    EXPECT_LE(available, 120.0f);
    EXPECT_GE(available, 0.0f);
}

// ==================== Test Suite 8: Error Recovery ====================

class ErrorRecoveryTests : public ::testing::Test {
protected:
    void SetUp() override {
        config_.maxRetries = 3;
        config_.baseDelayMs = 1000;
        config_.maxDelayMs = 30000;
        manager_ = std::make_unique<ErrorRecoveryManager>(config_);
    }

    ErrorRecoveryConfig config_;
    std::unique_ptr<ErrorRecoveryManager> manager_;
};

TEST_F(ErrorRecoveryTests, IsRetryable_NetworkError) {
    LLMError error;
    error.type = LLMError::ErrorType::NETWORK_ERROR;
    EXPECT_TRUE(manager_->isRetryable(error));
}

TEST_F(ErrorRecoveryTests, IsRetryable_APIError_5xx) {
    LLMError error;
    error.type = LLMError::ErrorType::API_ERROR;
    error.httpStatusCode = 502;
    EXPECT_TRUE(manager_->isRetryable(error));
}

TEST_F(ErrorRecoveryTests, IsRetryable_APIError_4xx) {
    LLMError error;
    error.type = LLMError::ErrorType::API_ERROR;
    error.httpStatusCode = 401;
    EXPECT_FALSE(manager_->isRetryable(error));
}

TEST_F(ErrorRecoveryTests, GetRetryDelayMs) {
    int delay = manager_->getRetryDelayMs(0);
    // Base delay is 1000ms, with jitter it can be between 900-1100ms
    EXPECT_GE(delay, 800);
    EXPECT_LE(delay, 1200);
}

TEST_F(ErrorRecoveryTests, HandleError_Retryable) {
    LLMError error;
    error.type = LLMError::ErrorType::TIMEOUT;
    auto action = manager_->handleError(error, 0);
    EXPECT_EQ(action, ErrorRecoveryManager::RecoveryAction::RETRY_LATER);
}

TEST_F(ErrorRecoveryTests, GetErrorStatistics) {
    LLMError error;
    error.type = LLMError::ErrorType::TIMEOUT;
    manager_->recordError(error);
    std::string stats = manager_->getErrorStatistics();
    EXPECT_NE(stats.find("Error Recovery Statistics"), std::string::npos);
}

// ==================== Test Suite 9: Deterministic Replay ====================

class DeterministicReplayTests : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = &DeterministicReplayLogger::getInstance();
        logger_->clear();
        logger_->enableLogging();
    }

    void TearDown() override {
        logger_->clear();
    }

    DeterministicReplayLogger* logger_;
};

TEST_F(DeterministicReplayTests, RecordLLMCall) {
    logger_->recordLLMCall(
        100, LLMCallType::DECISION_INTERPRETATION, "test prompt", "test response",
        50, 100, 250, LLMProviderType::OPENAI
    );
    EXPECT_EQ(logger_->getLLMCallCount(), 1);
}

TEST_F(DeterministicReplayTests, GetLLMCallAtTick) {
    logger_->recordLLMCall(
        100, LLMCallType::DECISION_INTERPRETATION, "prompt", "response",
        50, 100, 250, LLMProviderType::OPENAI
    );
    const LLMCallRecord* record = logger_->getLLMCallAtTick(100, LLMCallType::DECISION_INTERPRETATION);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->prompt, "prompt");
}

TEST_F(DeterministicReplayTests, RecordRandomDecision) {
    logger_->recordRandomDecision(100, "emotionSystem", "moodVariance", 0.75f, 42);
    EXPECT_EQ(logger_->getRandomDecisionCount(), 1);
}

TEST_F(DeterministicReplayTests, GetRandomDecisionAtTick) {
    logger_->recordRandomDecision(100, "emotionSystem", "moodVariance", 0.75f, 42);
    const RandomDecisionRecord* record = logger_->getRandomDecisionAtTick(100, "emotionSystem");
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->randomValue, 0.75f);
}

TEST_F(DeterministicReplayTests, ValidateReplayAtTick) {
    logger_->recordLLMCall(100, LLMCallType::DECISION_INTERPRETATION, "p", "r", 10, 20, 100, LLMProviderType::OPENAI);
    EXPECT_TRUE(logger_->validateReplayAtTick(100, 1));
    EXPECT_FALSE(logger_->validateReplayAtTick(100, 2));
}

TEST_F(DeterministicReplayTests, GetStatistics) {
    logger_->recordLLMCall(100, LLMCallType::WORLD_STATE_NARRATIVE, "p", "r", 10, 20, 100, LLMProviderType::OPENAI);
    std::string stats = logger_->getStatistics();
    EXPECT_NE(stats.find("Replay Logger Statistics"), std::string::npos);
}

// ==================== Test Suite 10: Edge Cases ====================

class EdgeCaseTests : public ::testing::Test {
};

TEST_F(EdgeCaseTests, ProviderFactory_NullConfig) {
    auto provider = LLMProviderFactory::createProvider(LLMProviderType::UNKNOWN, LLMConfig());
    EXPECT_EQ(provider, nullptr);
}

TEST_F(EdgeCaseTests, TokenBucket_ZeroMaxTokens) {
    TokenBucketRateLimiter limiter(0.0f);
    EXPECT_FALSE(limiter.canMakeRequest());
}

TEST_F(EdgeCaseTests, ExponentialBackoff_ZeroBase) {
    ExponentialBackoffCalculator calc(0, 1000, false, 0.0f);
    int delay = calc.calculateDelay(5);
    EXPECT_EQ(delay, 1);  // Minimum 1ms
}

TEST_F(EdgeCaseTests, ResponseCache_DuplicateInsert) {
    LLMResponseCache cache(300);
    LLMResponse response;
    response.wasSuccessful = true;
    response.text = "Test";

    EXPECT_TRUE(cache.put("prompt", response));
    EXPECT_TRUE(cache.put("prompt", response));  // Should replace
}

TEST_F(EdgeCaseTests, TokenTracker_DisableTracking) {
    TokenTracker::getInstance().reset();
    TokenTracker::getInstance().disableTracking();
    TokenTracker::getInstance().recordUsage("gpt-4", LLMCallType::DECISION_INTERPRETATION, 100, 50, true);
    EXPECT_EQ(TokenTracker::getInstance().getTotalTokens(), 0);
}

TEST_F(EdgeCaseTests, ErrorRecovery_MaxRetriesExceeded) {
    ErrorRecoveryConfig config;
    config.maxRetries = 2;
    ErrorRecoveryManager manager(config);

    LLMError error;
    error.type = LLMError::ErrorType::TIMEOUT;
    auto action = manager.handleError(error, 3);  // Attempt > maxRetries
    EXPECT_EQ(action, ErrorRecoveryManager::RecoveryAction::USE_FALLBACK);
}

TEST_F(EdgeCaseTests, Configuration_EmptyEnvironment) {
    _putenv_s("LLM_PROVIDER", "");
    _putenv_s("OPENAI_API_KEY", "");
    LLMConfig config = LLMConfigManager::getDefaultConfiguration();
    EXPECT_NE(config.preferredProvider, LLMProviderType::UNKNOWN);
}

TEST_F(EdgeCaseTests, OfflineFallback_EmptyInput) {
    std::string result = OfflineLLMFallback::interpretDecision("", "");
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTests, ReplayLogger_GetCallsAtTick) {
    DeterministicReplayLogger& logger = DeterministicReplayLogger::getInstance();
    logger.clear();
    logger.recordLLMCall(100, LLMCallType::DECISION_INTERPRETATION, "p1", "r1", 10, 20, 100, LLMProviderType::OPENAI);
    logger.recordLLMCall(100, LLMCallType::NPC_CONVERSATION, "p2", "r2", 15, 25, 150, LLMProviderType::OFFLINE_FALLBACK);

    auto calls = logger.getLLMCallsAtTick(100);
    EXPECT_EQ(calls.size(), 2);
}

// Test count: 63 tests total across 10 suites
