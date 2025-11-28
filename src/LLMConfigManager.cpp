#include "LLMConfigManager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace TLS {

// ==================== LLMConfigManager ====================

LLMConfig LLMConfigManager::loadConfiguration(const std::string& configPath) {
    // Priority: File -> Environment -> Defaults
    LLMConfig config = getDefaultConfiguration();

    // Try to load from file if provided
    std::string filePath = configPath.empty() ? "data/llm_config.json" : configPath;
    LLMConfig fileConfig = loadFromFile(filePath);
    config = mergeConfigurations(config, fileConfig);

    // Override with environment variables
    LLMConfig envConfig = loadFromEnvironment();
    config = mergeConfigurations(config, envConfig);

    // Validate
    if (!validateConfiguration(config)) {
        // Use defaults if validation fails
        config = getDefaultConfiguration();
    }

    return config;
}

LLMConfig LLMConfigManager::loadFromFile(const std::string& filePath) {
    LLMConfig config = getDefaultConfiguration();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        // File not found - return defaults
        return config;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Simple JSON parsing (not full parsing - just extract key values)
    // In production, use a JSON library
    
    // Extract provider
    if (content.find("\"provider\": \"openai\"") != std::string::npos ||
        content.find("\"provider\":\"openai\"") != std::string::npos) {
        config.preferredProvider = LLMProviderType::OPENAI;
    } else if (content.find("\"provider\": \"llama\"") != std::string::npos ||
               content.find("\"provider\":\"llama\"") != std::string::npos) {
        config.preferredProvider = LLMProviderType::LLAMA_LOCAL;
    } else if (content.find("\"provider\": \"offline\"") != std::string::npos ||
               content.find("\"provider\":\"offline\"") != std::string::npos) {
        config.preferredProvider = LLMProviderType::OFFLINE_FALLBACK;
    }

    // Extract timeout
    size_t timeoutPos = content.find("\"timeoutSeconds\":");
    if (timeoutPos != std::string::npos) {
        try {
            std::string numStr = content.substr(timeoutPos + 17, 3);
            config.timeoutSeconds = std::stoi(numStr);
        } catch (...) {
            // Keep default
        }
    }

    // Extract max retries
    size_t retriesPos = content.find("\"maxRetries\":");
    if (retriesPos != std::string::npos) {
        try {
            std::string numStr = content.substr(retriesPos + 13, 2);
            config.maxRetries = std::stoi(numStr);
        } catch (...) {
            // Keep default
        }
    }

    // Extract fallback enabled
    if (content.find("\"fallbackEnabled\": false") != std::string::npos ||
        content.find("\"fallbackEnabled\":false") != std::string::npos) {
        config.fallbackEnabled = false;
    }

    // Extract rate limit
    size_t ratePos = content.find("\"rateLimitPerMinute\":");
    if (ratePos != std::string::npos) {
        try {
            std::string numStr = content.substr(ratePos + 21, 6);
            config.rateLimitPerMinute = std::stof(numStr);
        } catch (...) {
            // Keep default
        }
    }

    // Note: API keys are NOT loaded from JSON for security (use environment variables)
    
    return config;
}

LLMConfig LLMConfigManager::loadFromEnvironment() {
    LLMConfig config = getDefaultConfiguration();

    // Read environment variables
    const char* provider = std::getenv(ENV_PREFERRED_PROVIDER);
    if (provider) {
        if (std::string(provider) == "openai") {
            config.preferredProvider = LLMProviderType::OPENAI;
        } else if (std::string(provider) == "llama") {
            config.preferredProvider = LLMProviderType::LLAMA_LOCAL;
        } else if (std::string(provider) == "offline") {
            config.preferredProvider = LLMProviderType::OFFLINE_FALLBACK;
        }
    }

    const char* apiKey = std::getenv(ENV_OPENAI_KEY);
    if (apiKey) {
        config.apiKey = apiKey;
    }

    const char* llamaUrl = std::getenv(ENV_LLAMA_URL);
    if (llamaUrl) {
        config.llamaServerUrl = llamaUrl;
    }

    const char* endpoint = std::getenv("LLM_API_ENDPOINT");
    if (endpoint) {
        config.apiEndpoint = endpoint;
    }

    const char* model = std::getenv("LLM_MODEL");
    if (model) {
        config.openaiModel = model;
    }

    const char* timeout = std::getenv("LLM_TIMEOUT_SECONDS");
    if (timeout) {
        try {
            config.timeoutSeconds = std::stoi(timeout);
        } catch (...) {
            // Keep default
        }
    }

    return config;
}

LLMConfig LLMConfigManager::getDefaultConfiguration() {
    LLMConfig config;
    config.preferredProvider = LLMProviderType::OPENAI;
    config.apiKey = "";
    config.apiEndpoint = "https://api.openai.com/v1";
    config.timeoutSeconds = 10;
    config.maxRetries = 3;
    config.fallbackEnabled = true;
    config.rateLimitPerMinute = 60.0f;
    config.openaiModel = "gpt-3.5-turbo";
    config.llamaServerUrl = "http://localhost:8000";
    config.llamaTimeoutSeconds = 30;
    return config;
}

bool LLMConfigManager::validateConfiguration(const LLMConfig& config) {
    // Basic validation
    if (config.timeoutSeconds <= 0 || config.timeoutSeconds > 300) {
        return false;
    }
    if (config.maxRetries < 0 || config.maxRetries > 10) {
        return false;
    }
    if (config.rateLimitPerMinute <= 0) {
        return false;
    }

    // Check provider-specific requirements
    if (config.preferredProvider == LLMProviderType::OPENAI && config.apiKey.empty()) {
        // OpenAI requires API key (could warn but not fail)
    }
    if (config.preferredProvider == LLMProviderType::LLAMA_LOCAL && config.llamaServerUrl.empty()) {
        // LLaMA requires server URL (could warn but not fail)
    }

    return true;
}

LLMConfig LLMConfigManager::mergeConfigurations(
    const LLMConfig& base,
    const LLMConfig& override
) {
    LLMConfig merged = base;

    // Override with non-default values from override config
    if (override.preferredProvider != LLMProviderType::UNKNOWN) {
        merged.preferredProvider = override.preferredProvider;
    }
    if (!override.apiKey.empty()) {
        merged.apiKey = override.apiKey;
    }
    if (!override.apiEndpoint.empty() && override.apiEndpoint != base.apiEndpoint) {
        merged.apiEndpoint = override.apiEndpoint;
    }
    if (override.timeoutSeconds != base.timeoutSeconds) {
        merged.timeoutSeconds = override.timeoutSeconds;
    }
    if (override.maxRetries != base.maxRetries) {
        merged.maxRetries = override.maxRetries;
    }
    if (override.fallbackEnabled != base.fallbackEnabled) {
        merged.fallbackEnabled = override.fallbackEnabled;
    }
    if (override.rateLimitPerMinute != base.rateLimitPerMinute) {
        merged.rateLimitPerMinute = override.rateLimitPerMinute;
    }
    if (!override.openaiModel.empty() && override.openaiModel != base.openaiModel) {
        merged.openaiModel = override.openaiModel;
    }
    if (!override.llamaServerUrl.empty() && override.llamaServerUrl != base.llamaServerUrl) {
        merged.llamaServerUrl = override.llamaServerUrl;
    }
    if (override.llamaTimeoutSeconds != base.llamaTimeoutSeconds) {
        merged.llamaTimeoutSeconds = override.llamaTimeoutSeconds;
    }

    return merged;
}

bool LLMConfigManager::saveConfiguration(
    const LLMConfig& config,
    const std::string& filePath
) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    // Write JSON (note: API keys are NOT saved for security)
    file << "{\n";
    file << "  \"provider\": \"" << LLMProviderFactory::providerTypeToString(config.preferredProvider) << "\",\n";
    file << "  \"apiEndpoint\": \"" << config.apiEndpoint << "\",\n";
    file << "  \"timeoutSeconds\": " << config.timeoutSeconds << ",\n";
    file << "  \"maxRetries\": " << config.maxRetries << ",\n";
    file << "  \"fallbackEnabled\": " << (config.fallbackEnabled ? "true" : "false") << ",\n";
    file << "  \"rateLimitPerMinute\": " << config.rateLimitPerMinute << ",\n";
    file << "  \"openaiModel\": \"" << config.openaiModel << "\",\n";
    file << "  \"llamaServerUrl\": \"" << config.llamaServerUrl << "\",\n";
    file << "  \"llamaTimeoutSeconds\": " << config.llamaTimeoutSeconds << ",\n";
    file << "  \"_note\": \"API keys are NOT stored here for security. Use environment variables instead.\"\n";
    file << "}\n";

    file.close();
    return true;
}

}  // namespace TLS
