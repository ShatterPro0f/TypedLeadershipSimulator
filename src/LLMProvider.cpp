#include "LLMProvider.h"
#include "LLMResponseCache.h"
#include "OllamaProvider.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace TLS {

// ==================== LLMResponse ====================

std::string LLMResponse::toString() const {
    std::ostringstream oss;
    oss << "LLMResponse{success=" << (wasSuccessful ? "true" : "false")
        << ", text_length=" << text.length()
        << ", input_tokens=" << inputTokens
        << ", completion_tokens=" << completionTokens
        << ", latency_ms=" << latencyMs
        << ", provider=" << static_cast<int>(provider)
        << "}";
    return oss.str();
}

// ==================== LLMUsage ====================

std::string LLMUsage::toString() const {
    std::ostringstream oss;
    oss << "LLMUsage{total_input=" << totalInputTokens
        << ", total_completion=" << totalCompletionTokens
        << ", requests=" << totalRequests
        << ", cost_usd=$" << estimatedCostUSD
        << "}";
    return oss.str();
}

// ==================== OpenAIProvider ====================

class OpenAIProvider : public LLMProvider {
private:
    LLMConfig config_;
    LLMUsage usage_;

public:
    OpenAIProvider(const LLMConfig& config)
        : config_(config), usage_{0, 0, 0, 0.0f, 0} {}

    LLMResponse callLLM(const std::string& prompt) override;
    bool isAvailable() const override;
    LLMProviderType getProviderType() const override { return LLMProviderType::OPENAI; }
    std::string getProviderName() const override;
    LLMUsage getTokenUsage() const override;
    void resetTokenUsage() override;
};

LLMResponse OpenAIProvider::callLLM(const std::string& prompt) {
    LLMResponse response;
    response.wasSuccessful = false;
    response.provider = LLMProviderType::OPENAI;

    if (config_.apiKey.empty()) {
        response.errorMessage = "API key not configured";
        return response;
    }

    response.wasSuccessful = true;
    response.text = "OpenAI response: " + prompt.substr(0, 50);
    response.inputTokens = static_cast<int>(prompt.length() / 4);
    response.completionTokens = 50;
    response.totalTokens = response.inputTokens + response.completionTokens;
    response.latencyMs = 1000;

    usage_.totalInputTokens += response.inputTokens;
    usage_.totalCompletionTokens += response.completionTokens;
    usage_.totalRequests++;
    usage_.estimatedCostUSD += (response.inputTokens * 0.03f + response.completionTokens * 0.06f) / 1000.0f;

    return response;
}

bool OpenAIProvider::isAvailable() const {
    return !config_.apiKey.empty();
}

std::string OpenAIProvider::getProviderName() const {
    return "OpenAI";
}

LLMUsage OpenAIProvider::getTokenUsage() const {
    return usage_;
}

void OpenAIProvider::resetTokenUsage() {
    usage_ = {0, 0, 0, 0.0f, 0};
}

// ==================== LocalLlamaProvider ====================

class LocalLlamaProvider : public LLMProvider {
private:
    LLMConfig config_;
    LLMUsage usage_;

public:
    LocalLlamaProvider(const LLMConfig& config)
        : config_(config), usage_{0, 0, 0, 0.0f, 0} {}

    LLMResponse callLLM(const std::string& prompt) override;
    bool isAvailable() const override;
    LLMProviderType getProviderType() const override { return LLMProviderType::LLAMA_LOCAL; }
    std::string getProviderName() const override;
    LLMUsage getTokenUsage() const override;
    void resetTokenUsage() override;
};

LLMResponse LocalLlamaProvider::callLLM(const std::string& prompt) {
    LLMResponse response;
    response.wasSuccessful = false;
    response.provider = LLMProviderType::LLAMA_LOCAL;

    if (config_.llamaServerUrl.empty()) {
        response.errorMessage = "LLaMA server URL not configured";
        return response;
    }

    response.wasSuccessful = true;
    response.text = "LocalLLaMA response: " + prompt.substr(0, 50);
    response.inputTokens = static_cast<int>(prompt.length() / 4);
    response.completionTokens = 40;
    response.totalTokens = response.inputTokens + response.completionTokens;
    response.latencyMs = 2000;

    usage_.totalInputTokens += response.inputTokens;
    usage_.totalCompletionTokens += response.completionTokens;
    usage_.totalRequests++;

    return response;
}

bool LocalLlamaProvider::isAvailable() const {
    return !config_.llamaServerUrl.empty();
}

std::string LocalLlamaProvider::getProviderName() const {
    return "LocalLLaMA";
}

LLMUsage LocalLlamaProvider::getTokenUsage() const {
    return usage_;
}

void LocalLlamaProvider::resetTokenUsage() {
    usage_ = {0, 0, 0, 0.0f, 0};
}

// ==================== OfflineFallbackProvider ====================

class OfflineFallbackProvider : public LLMProvider {
private:
    LLMUsage usage_;

public:
    OfflineFallbackProvider()
        : usage_{0, 0, 0, 0.0f, 0} {}

    LLMResponse callLLM(const std::string& prompt) override;
    bool isAvailable() const override { return true; }
    LLMProviderType getProviderType() const override { return LLMProviderType::OFFLINE_FALLBACK; }
    std::string getProviderName() const override { return "OfflineFallback"; }
    LLMUsage getTokenUsage() const override;
    void resetTokenUsage() override;
};

LLMResponse OfflineFallbackProvider::callLLM(const std::string& prompt) {
    LLMResponse response;
    response.wasSuccessful = true;
    response.provider = LLMProviderType::OFFLINE_FALLBACK;

    if (prompt.find("crisis") != std::string::npos) {
        response.text = "Crisis detected. Settlement facing challenge.";
    } else if (prompt.find("decision") != std::string::npos) {
        response.text = "Council reviews your decision. Response incoming.";
    } else if (prompt.find("resource") != std::string::npos) {
        response.text = "Resources are being assessed. Management recommended.";
    } else {
        response.text = "Settlement acknowledges your leadership direction.";
    }

    response.inputTokens = static_cast<int>(prompt.length() / 4);
    response.completionTokens = static_cast<int>(response.text.length() / 4);
    response.totalTokens = response.inputTokens + response.completionTokens;
    response.latencyMs = 50;

    usage_.totalInputTokens += response.inputTokens;
    usage_.totalCompletionTokens += response.completionTokens;
    usage_.totalRequests++;

    return response;
}

LLMUsage OfflineFallbackProvider::getTokenUsage() const {
    return usage_;
}

void OfflineFallbackProvider::resetTokenUsage() {
    usage_ = {0, 0, 0, 0.0f, 0};
}

// ==================== LLMProviderFactory ====================

LLMProvider* LLMProviderFactory::createProvider(
    LLMProviderType type,
    const LLMConfig& config
) {
    switch (type) {
        case LLMProviderType::OPENAI:
            return new OpenAIProvider(config);
        case LLMProviderType::LLAMA_LOCAL:
            return new LocalLlamaProvider(config);
        case LLMProviderType::OLLAMA:
            return new OllamaProvider(config);
        case LLMProviderType::OFFLINE_FALLBACK:
            return new OfflineFallbackProvider();
        default:
            return nullptr;
    }
}

LLMProvider* LLMProviderFactory::createProviderFromConfig(
    const LLMConfig& config,
    LLMProviderType providerType
) {
    if (providerType == LLMProviderType::UNKNOWN) {
        providerType = config.preferredProvider;
    }
    return createProvider(providerType, config);
}

std::string LLMProviderFactory::providerTypeToString(LLMProviderType type) {
    switch (type) {
        case LLMProviderType::OPENAI:
            return "OpenAI";
        case LLMProviderType::LLAMA_LOCAL:
            return "LocalLLaMA";
        case LLMProviderType::OLLAMA:
            return "Ollama";
        case LLMProviderType::OFFLINE_FALLBACK:
            return "OfflineFallback";
        case LLMProviderType::UNKNOWN:
            return "Unknown";
        default:
            return "Invalid";
    }
}

LLMProviderType LLMProviderFactory::stringToProviderType(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find("openai") != std::string::npos) {
        return LLMProviderType::OPENAI;
    } else if (lower.find("ollama") != std::string::npos) {
        return LLMProviderType::OLLAMA;
    } else if (lower.find("llama") != std::string::npos || lower.find("local") != std::string::npos) {
        return LLMProviderType::LLAMA_LOCAL;
    } else if (lower.find("offline") != std::string::npos || lower.find("fallback") != std::string::npos) {
        return LLMProviderType::OFFLINE_FALLBACK;
    }
    return LLMProviderType::UNKNOWN;
}

}  // namespace TLS
