#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace TLS {

// ============================================================================
// LLM Provider Types and Configuration
// ============================================================================

enum class LLMProviderType {
    OPENAI = 0,
    LLAMA_LOCAL = 1,
    OFFLINE_FALLBACK = 2,
    OLLAMA = 3,
    UNKNOWN = 4
};

/**
 * @struct LLMConfig
 * @brief Configuration for LLM provider selection and behavior
 */
struct LLMConfig {
    LLMProviderType preferredProvider = LLMProviderType::OPENAI;
    std::string apiKey;
    std::string apiEndpoint;
    int timeoutSeconds = 10;
    int maxRetries = 3;
    bool fallbackEnabled = true;
    float rateLimitPerMinute = 60.0f;
    
    // OpenAI specific
    std::string openaiModel = "gpt-4";
    
    // Local LLaMA specific
    std::string llamaServerUrl = "http://localhost:8000";
    int llamaTimeoutSeconds = 15;
    
    // Ollama specific
    std::string ollamaServerUrl = "http://localhost:11434";
    std::string ollamaModel = "gemma3:12b";
    int ollamaTimeoutSeconds = 60;
};

/**
 * @struct LLMResponse
 * @brief Response from LLM provider
 */
struct LLMResponse {
    bool wasSuccessful = false;
    std::string text;
    std::string errorMessage;
    int inputTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
    int latencyMs = 0;
    LLMProviderType provider = LLMProviderType::UNKNOWN;

    /**
     * Convert response to string for logging
     * @return String representation of response
     */
    std::string toString() const;
};

/**
 * @struct LLMUsage
 * @brief Token usage tracking for cost estimation
 */
struct LLMUsage {
    int totalInputTokens = 0;
    int totalCompletionTokens = 0;
    int totalRequests = 0;
    float estimatedCostUSD = 0.0f;
    int64_t timestamp = 0;

    /**
     * Convert usage to string for logging
     * @return String representation of usage
     */
    std::string toString() const;
};

/**
 * @class LLMProvider
 * @brief Abstract base class for LLM provider implementations
 *
 * Defines the interface for all LLM providers (OpenAI, Local LLaMA, Offline)
 * All implementations must support:
 * - Availability checking
 * - Synchronous API calls (blocking)
 * - Error handling and timeouts
 * - Token usage tracking
 */
class LLMProvider {
public:
    virtual ~LLMProvider() = default;

    /**
     * Call the LLM with a prompt
     * @param prompt The input text/prompt
     * @return LLMResponse containing result or error
     */
    virtual LLMResponse callLLM(const std::string& prompt) = 0;

    /**
     * Check if provider is available (has valid config, network, etc.)
     * @return true if provider can be used, false otherwise
     */
    virtual bool isAvailable() const = 0;

    /**
     * Get provider type
     * @return LLMProviderType identifying this provider
     */
    virtual LLMProviderType getProviderType() const = 0;

    /**
     * Get human-readable provider name
     * @return Name of provider (e.g., "OpenAI GPT-4")
     */
    virtual std::string getProviderName() const = 0;

    /**
     * Get current token usage stats
     * @return LLMUsage struct with cumulative usage
     */
    virtual LLMUsage getTokenUsage() const = 0;

    /**
     * Reset token usage counter
     */
    virtual void resetTokenUsage() = 0;
};

/**
 * @class LLMProviderFactory
 * @brief Factory for creating LLM provider instances
 */
class LLMProviderFactory {
public:
    /**
     * Create an LLM provider instance
     * @param providerType Type of provider to create
     * @param config Configuration for the provider
     * @return Pointer to new LLMProvider (caller owns)
     */
    static LLMProvider* createProvider(
        LLMProviderType providerType,
        const LLMConfig& config
    );

    /**
     * Create provider from configuration
     * @param config LLMConfig structure
     * @param providerType Override type (or use config.preferredProvider)
     * @return Pointer to new LLMProvider (caller owns)
     */
    static LLMProvider* createProviderFromConfig(
        const LLMConfig& config,
        LLMProviderType providerType = LLMProviderType::UNKNOWN
    );

    /**
     * Get provider type string name
     * @param type LLMProviderType enum value
     * @return String name of provider type
     */
    static std::string providerTypeToString(LLMProviderType type);

    /**
     * Convert string to provider type
     * @param str String name
     * @return LLMProviderType enum value
     */
    static LLMProviderType stringToProviderType(const std::string& str);
};

}  // namespace TLS
