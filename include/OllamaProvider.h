#pragma once

#include "LLMProvider.h"
#include <string>
#include <chrono>
#include <mutex>

namespace TLS {

/**
 * @class OllamaProvider
 * @brief LLM Provider implementation for Ollama local server
 * 
 * Connects to Ollama running at localhost:11434 (configurable).
 * Supports any Ollama-compatible model (gemma3:12b, llama3, mistral, etc.).
 * 
 * Ollama API endpoints used:
 * - POST /api/generate - For text generation
 * - GET /api/tags - To check available models
 * 
 * Required: Ollama must be running locally before use.
 * Install Ollama from: https://ollama.ai
 */
class OllamaProvider : public LLMProvider {
public:
    /**
     * Construct OllamaProvider with configuration
     * @param config LLM configuration with Ollama-specific settings
     */
    explicit OllamaProvider(const LLMConfig& config);
    
    /**
     * Construct OllamaProvider with custom server URL and model
     * @param serverUrl Ollama server URL (e.g., "http://localhost:11434")
     * @param modelName Model to use (e.g., "gemma3:12b", "llama3")
     * @param timeoutSeconds Request timeout in seconds
     */
    OllamaProvider(const std::string& serverUrl, 
                   const std::string& modelName,
                   int timeoutSeconds = 60);
    
    virtual ~OllamaProvider() = default;
    
    // LLMProvider interface implementation
    LLMResponse callLLM(const std::string& prompt) override;
    bool isAvailable() const override;
    LLMProviderType getProviderType() const override;
    std::string getProviderName() const override;
    LLMUsage getTokenUsage() const override;
    void resetTokenUsage() override;
    
    // Ollama-specific methods
    
    /**
     * Check if a specific model is available in Ollama
     * @param modelName Model to check (e.g., "gemma3:12b")
     * @return true if model is available
     */
    bool isModelAvailable(const std::string& modelName) const;
    
    /**
     * Get list of available models from Ollama
     * @return Vector of model names
     */
    std::vector<std::string> getAvailableModels() const;
    
    /**
     * Get currently configured model name
     * @return Model name string
     */
    std::string getModelName() const { return m_modelName; }
    
    /**
     * Set model to use for generation
     * @param modelName New model name
     */
    void setModelName(const std::string& modelName) { m_modelName = modelName; }
    
    /**
     * Get server URL
     * @return Ollama server URL
     */
    std::string getServerUrl() const { return m_serverUrl; }
    
    /**
     * Test connection to Ollama server
     * @return true if server is reachable
     */
    bool testConnection() const;

private:
    std::string m_serverUrl;
    std::string m_modelName;
    int m_timeoutSeconds;
    
    // Token usage tracking
    mutable std::mutex m_usageMutex;
    LLMUsage m_usage;
    
    // HTTP helper methods
    std::string makeHttpPost(const std::string& endpoint, 
                             const std::string& jsonBody) const;
    std::string makeHttpGet(const std::string& endpoint) const;
    
    // JSON parsing helpers
    std::string parseGenerateResponse(const std::string& jsonResponse, 
                                       int& evalCount,
                                       int& promptEvalCount) const;
    std::vector<std::string> parseTagsResponse(const std::string& jsonResponse) const;
    
    // URL parsing helper
    void parseServerUrl(const std::string& url);
    std::string m_host;
    int m_port;
};

}  // namespace TLS
