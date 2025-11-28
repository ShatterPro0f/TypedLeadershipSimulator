#pragma once

#include "LLMProvider.h"
#include <string>
#include <memory>

namespace TLS {

/**
 * @class LLMConfigManager
 * @brief Loads and manages LLM configuration from files and environment
 *
 * Configuration sources (priority order):
 * 1. Environment variables (OPENAI_API_KEY, LLAMA_URL, etc.)
 * 2. Local config file (data/llm_config.json)
 * 3. Hardcoded defaults
 *
 * Configuration validation:
 * - API keys never logged
 * - Endpoints validated as URLs
 * - Timeouts must be positive
 * - Rate limits must be > 0
 */
class LLMConfigManager {
public:
    /**
     * Load configuration from file and environment
     * @param configPath Path to JSON config file (optional)
     * @return LLMConfig structure with loaded values
     */
    static LLMConfig loadConfiguration(const std::string& configPath = "");

    /**
     * Load configuration from JSON file
     * @param filePath Path to JSON config file
     * @return LLMConfig or default if file not found
     */
    static LLMConfig loadFromFile(const std::string& filePath);

    /**
     * Load configuration from environment variables
     * @return LLMConfig with values from environment
     */
    static LLMConfig loadFromEnvironment();

    /**
     * Get default configuration
     * @return LLMConfig with hardcoded defaults
     */
    static LLMConfig getDefaultConfiguration();

    /**
     * Validate configuration values
     * @param config Configuration to validate
     * @return true if valid, false if errors detected
     */
    static bool validateConfiguration(const LLMConfig& config);

    /**
     * Merge two configurations (second overrides first)
     * @param base Base configuration
     * @param override Configuration to override with
     * @return Merged configuration
     */
    static LLMConfig mergeConfigurations(
        const LLMConfig& base,
        const LLMConfig& override
    );

    /**
     * Save configuration to file (excluding sensitive data)
     * @param config Configuration to save
     * @param filePath Path to save to
     * @return true if saved successfully
     */
    static bool saveConfiguration(const LLMConfig& config, const std::string& filePath);

private:
    static constexpr const char* ENV_OPENAI_KEY = "OPENAI_API_KEY";
    static constexpr const char* ENV_LLAMA_URL = "LLAMA_SERVER_URL";
    static constexpr const char* ENV_PREFERRED_PROVIDER = "LLM_PROVIDER";
    static constexpr const char* CONFIG_FILE_DEFAULT = "data/llm_config.json";
};

}  // namespace TLS
