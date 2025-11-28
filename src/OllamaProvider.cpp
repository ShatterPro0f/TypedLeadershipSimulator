#include "OllamaProvider.h"

// cpp-httplib requires Windows 10+ for WSAPoll
// Define _WIN32_WINNT before including httplib.h
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Windows 10
#endif
#endif

// cpp-httplib header-only library for HTTP requests
#define CPPHTTPLIB_RECV_FLAGS 0
#define CPPHTTPLIB_SEND_FLAGS 0
#include "../external/httplib.h"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>

namespace TLS {

// ============================================================================
// Constructor implementations
// ============================================================================

OllamaProvider::OllamaProvider(const LLMConfig& config)
    : m_serverUrl(config.ollamaServerUrl)
    , m_modelName(config.ollamaModel)
    , m_timeoutSeconds(config.ollamaTimeoutSeconds)
{
    parseServerUrl(m_serverUrl);
    m_usage = LLMUsage();
}

OllamaProvider::OllamaProvider(const std::string& serverUrl,
                               const std::string& modelName,
                               int timeoutSeconds)
    : m_serverUrl(serverUrl)
    , m_modelName(modelName)
    , m_timeoutSeconds(timeoutSeconds)
{
    parseServerUrl(m_serverUrl);
    m_usage = LLMUsage();
}

// ============================================================================
// URL Parsing
// ============================================================================

void OllamaProvider::parseServerUrl(const std::string& url) {
    // Default values
    m_host = "localhost";
    m_port = 11434;
    
    std::string urlCopy = url;
    
    // Remove protocol prefix if present
    size_t protoPos = urlCopy.find("://");
    if (protoPos != std::string::npos) {
        urlCopy = urlCopy.substr(protoPos + 3);
    }
    
    // Remove trailing slash if present
    if (!urlCopy.empty() && urlCopy.back() == '/') {
        urlCopy.pop_back();
    }
    
    // Find port separator
    size_t colonPos = urlCopy.find(':');
    if (colonPos != std::string::npos) {
        m_host = urlCopy.substr(0, colonPos);
        std::string portStr = urlCopy.substr(colonPos + 1);
        try {
            m_port = std::stoi(portStr);
        } catch (...) {
            m_port = 11434;
        }
    } else {
        m_host = urlCopy;
    }
}

// ============================================================================
// LLMProvider interface implementation
// ============================================================================

LLMResponse OllamaProvider::callLLM(const std::string& prompt) {
    LLMResponse response;
    response.provider = LLMProviderType::OLLAMA;
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Build JSON request body for Ollama /api/generate
    std::ostringstream jsonBody;
    jsonBody << "{"
             << "\"model\":\"" << m_modelName << "\","
             << "\"prompt\":\"";
    
    // Escape special characters in prompt
    for (char c : prompt) {
        switch (c) {
            case '"':  jsonBody << "\\\""; break;
            case '\\': jsonBody << "\\\\"; break;
            case '\n': jsonBody << "\\n"; break;
            case '\r': jsonBody << "\\r"; break;
            case '\t': jsonBody << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) >= 32) {
                    jsonBody << c;
                }
                break;
        }
    }
    
    jsonBody << "\","
             << "\"stream\":false"
             << "}";
    
    try {
        std::string jsonResponse = makeHttpPost("/api/generate", jsonBody.str());
        
        if (jsonResponse.empty()) {
            response.wasSuccessful = false;
            response.errorMessage = "Empty response from Ollama server";
            return response;
        }
        
        // Check for error in response
        if (jsonResponse.find("\"error\"") != std::string::npos) {
            size_t errorStart = jsonResponse.find("\"error\"");
            size_t colonPos = jsonResponse.find(':', errorStart);
            size_t quoteStart = jsonResponse.find('"', colonPos + 1);
            size_t quoteEnd = jsonResponse.find('"', quoteStart + 1);
            
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                response.errorMessage = jsonResponse.substr(quoteStart + 1, 
                                                            quoteEnd - quoteStart - 1);
            } else {
                response.errorMessage = "Unknown Ollama error";
            }
            response.wasSuccessful = false;
            return response;
        }
        
        // Parse response
        int evalCount = 0;
        int promptEvalCount = 0;
        response.text = parseGenerateResponse(jsonResponse, evalCount, promptEvalCount);
        
        // Update token counts
        response.inputTokens = promptEvalCount;
        response.completionTokens = evalCount;
        response.totalTokens = promptEvalCount + evalCount;
        response.wasSuccessful = true;
        
        // Update usage tracking
        {
            std::lock_guard<std::mutex> lock(m_usageMutex);
            m_usage.totalInputTokens += promptEvalCount;
            m_usage.totalCompletionTokens += evalCount;
            m_usage.totalRequests++;
            // Ollama is local/free, so no cost
            m_usage.estimatedCostUSD = 0.0f;
        }
        
    } catch (const std::exception& e) {
        response.wasSuccessful = false;
        response.errorMessage = std::string("HTTP error: ") + e.what();
    }
    
    // Calculate latency
    auto endTime = std::chrono::steady_clock::now();
    response.latencyMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
    );
    
    return response;
}

bool OllamaProvider::isAvailable() const {
    return testConnection();
}

LLMProviderType OllamaProvider::getProviderType() const {
    return LLMProviderType::OLLAMA;
}

std::string OllamaProvider::getProviderName() const {
    return "Ollama (" + m_modelName + ")";
}

LLMUsage OllamaProvider::getTokenUsage() const {
    std::lock_guard<std::mutex> lock(m_usageMutex);
    return m_usage;
}

void OllamaProvider::resetTokenUsage() {
    std::lock_guard<std::mutex> lock(m_usageMutex);
    m_usage = LLMUsage();
}

// ============================================================================
// Ollama-specific methods
// ============================================================================

bool OllamaProvider::isModelAvailable(const std::string& modelName) const {
    std::vector<std::string> models = getAvailableModels();
    
    for (const auto& model : models) {
        if (model == modelName) {
            return true;
        }
        // Also check without version suffix (e.g., "gemma3" matches "gemma3:12b")
        size_t colonPos = model.find(':');
        if (colonPos != std::string::npos) {
            std::string baseName = model.substr(0, colonPos);
            if (baseName == modelName || model == modelName) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> OllamaProvider::getAvailableModels() const {
    std::vector<std::string> models;
    
    try {
        std::string jsonResponse = makeHttpGet("/api/tags");
        if (!jsonResponse.empty()) {
            models = parseTagsResponse(jsonResponse);
        }
    } catch (...) {
        // Return empty vector on error
    }
    
    return models;
}

bool OllamaProvider::testConnection() const {
    try {
        httplib::Client client(m_host, m_port);
        client.set_connection_timeout(3);  // Short timeout for connection test
        client.set_read_timeout(3);
        
        auto result = client.Get("/api/tags");
        return result && result->status == 200;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// HTTP helper methods
// ============================================================================

std::string OllamaProvider::makeHttpPost(const std::string& endpoint,
                                          const std::string& jsonBody) const {
    httplib::Client client(m_host, m_port);
    client.set_connection_timeout(m_timeoutSeconds);
    client.set_read_timeout(m_timeoutSeconds);
    client.set_write_timeout(m_timeoutSeconds);
    
    auto result = client.Post(endpoint, jsonBody, "application/json");
    
    if (!result) {
        throw std::runtime_error("Failed to connect to Ollama server at " + 
                                 m_host + ":" + std::to_string(m_port));
    }
    
    if (result->status != 200) {
        throw std::runtime_error("Ollama server returned status " + 
                                 std::to_string(result->status));
    }
    
    return result->body;
}

std::string OllamaProvider::makeHttpGet(const std::string& endpoint) const {
    httplib::Client client(m_host, m_port);
    client.set_connection_timeout(m_timeoutSeconds);
    client.set_read_timeout(m_timeoutSeconds);
    
    auto result = client.Get(endpoint);
    
    if (!result) {
        throw std::runtime_error("Failed to connect to Ollama server");
    }
    
    if (result->status != 200) {
        throw std::runtime_error("Ollama server returned status " + 
                                 std::to_string(result->status));
    }
    
    return result->body;
}

// ============================================================================
// JSON parsing helpers (simple manual parsing to avoid extra dependencies)
// ============================================================================

std::string OllamaProvider::parseGenerateResponse(const std::string& jsonResponse,
                                                   int& evalCount,
                                                   int& promptEvalCount) const {
    std::string responseText;
    evalCount = 0;
    promptEvalCount = 0;
    
    // Find "response" field
    size_t responsePos = jsonResponse.find("\"response\"");
    if (responsePos != std::string::npos) {
        size_t colonPos = jsonResponse.find(':', responsePos);
        if (colonPos != std::string::npos) {
            size_t quoteStart = jsonResponse.find('"', colonPos + 1);
            if (quoteStart != std::string::npos) {
                // Find end of string (handle escaped quotes)
                size_t pos = quoteStart + 1;
                while (pos < jsonResponse.size()) {
                    if (jsonResponse[pos] == '"' && jsonResponse[pos - 1] != '\\') {
                        break;
                    }
                    pos++;
                }
                
                responseText = jsonResponse.substr(quoteStart + 1, pos - quoteStart - 1);
                
                // Unescape the string
                std::string unescaped;
                for (size_t i = 0; i < responseText.size(); i++) {
                    if (responseText[i] == '\\' && i + 1 < responseText.size()) {
                        switch (responseText[i + 1]) {
                            case 'n':  unescaped += '\n'; i++; break;
                            case 'r':  unescaped += '\r'; i++; break;
                            case 't':  unescaped += '\t'; i++; break;
                            case '"':  unescaped += '"';  i++; break;
                            case '\\': unescaped += '\\'; i++; break;
                            default:   unescaped += responseText[i]; break;
                        }
                    } else {
                        unescaped += responseText[i];
                    }
                }
                responseText = unescaped;
            }
        }
    }
    
    // Find "eval_count" (tokens generated)
    size_t evalPos = jsonResponse.find("\"eval_count\"");
    if (evalPos != std::string::npos) {
        size_t colonPos = jsonResponse.find(':', evalPos);
        if (colonPos != std::string::npos) {
            size_t numStart = colonPos + 1;
            while (numStart < jsonResponse.size() && !std::isdigit(jsonResponse[numStart])) {
                numStart++;
            }
            size_t numEnd = numStart;
            while (numEnd < jsonResponse.size() && std::isdigit(jsonResponse[numEnd])) {
                numEnd++;
            }
            if (numStart < numEnd) {
                evalCount = std::stoi(jsonResponse.substr(numStart, numEnd - numStart));
            }
        }
    }
    
    // Find "prompt_eval_count" (input tokens)
    size_t promptEvalPos = jsonResponse.find("\"prompt_eval_count\"");
    if (promptEvalPos != std::string::npos) {
        size_t colonPos = jsonResponse.find(':', promptEvalPos);
        if (colonPos != std::string::npos) {
            size_t numStart = colonPos + 1;
            while (numStart < jsonResponse.size() && !std::isdigit(jsonResponse[numStart])) {
                numStart++;
            }
            size_t numEnd = numStart;
            while (numEnd < jsonResponse.size() && std::isdigit(jsonResponse[numEnd])) {
                numEnd++;
            }
            if (numStart < numEnd) {
                promptEvalCount = std::stoi(jsonResponse.substr(numStart, numEnd - numStart));
            }
        }
    }
    
    return responseText;
}

std::vector<std::string> OllamaProvider::parseTagsResponse(
    const std::string& jsonResponse) const {
    
    std::vector<std::string> models;
    
    // Find "models" array
    size_t modelsPos = jsonResponse.find("\"models\"");
    if (modelsPos == std::string::npos) {
        return models;
    }
    
    // Look for each "name" field within models array
    size_t searchPos = modelsPos;
    while (true) {
        size_t namePos = jsonResponse.find("\"name\"", searchPos);
        if (namePos == std::string::npos) {
            break;
        }
        
        size_t colonPos = jsonResponse.find(':', namePos);
        if (colonPos == std::string::npos) {
            break;
        }
        
        size_t quoteStart = jsonResponse.find('"', colonPos + 1);
        if (quoteStart == std::string::npos) {
            break;
        }
        
        size_t quoteEnd = jsonResponse.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos) {
            break;
        }
        
        std::string modelName = jsonResponse.substr(quoteStart + 1, 
                                                     quoteEnd - quoteStart - 1);
        models.push_back(modelName);
        
        searchPos = quoteEnd + 1;
    }
    
    return models;
}

}  // namespace TLS
