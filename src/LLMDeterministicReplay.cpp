#include "LLMDeterministicReplay.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace TLS {

// ==================== LLMCallType to String Conversion ====================

static std::string llmCallTypeToString(LLMCallType type) {
    switch (type) {
        case LLMCallType::DECISION_INTERPRETATION: return "DECISION_INTERPRETATION";
        case LLMCallType::WORLD_STATE_NARRATIVE: return "WORLD_STATE_NARRATIVE";
        case LLMCallType::NPC_CONVERSATION: return "NPC_CONVERSATION";
        case LLMCallType::CRISIS_GENERATION: return "CRISIS_GENERATION";
        default: return "UNKNOWN";
    }
}

static LLMCallType stringToLLMCallType(const std::string& str) {
    if (str == "DECISION_INTERPRETATION") return LLMCallType::DECISION_INTERPRETATION;
    if (str == "WORLD_STATE_NARRATIVE") return LLMCallType::WORLD_STATE_NARRATIVE;
    if (str == "NPC_CONVERSATION") return LLMCallType::NPC_CONVERSATION;
    if (str == "CRISIS_GENERATION") return LLMCallType::CRISIS_GENERATION;
    return LLMCallType::UNKNOWN;
}

// ==================== LLMCallRecord ====================

std::string LLMCallRecord::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"tickNumber\": " << tickNumber << ",\n";
    oss << "  \"callType\": \"" << llmCallTypeToString(callType) << "\",\n";
    oss << "  \"prompt\": \"" << prompt << "\",\n";
    oss << "  \"llmOutput\": \"" << llmOutput << "\",\n";
    oss << "  \"inputTokens\": " << inputTokens << ",\n";
    oss << "  \"completionTokens\": " << completionTokens << ",\n";
    oss << "  \"totalTokens\": " << totalTokens << ",\n";
    oss << "  \"latencyMs\": " << latencyMs << ",\n";
    oss << "  \"provider\": " << static_cast<int>(provider) << ",\n";
    oss << "  \"wasSuccessful\": " << (wasSuccessful ? "true" : "false") << ",\n";
    oss << "  \"errorMessage\": \"" << errorMessage << "\",\n";
    oss << "  \"attemptNumber\": " << attemptNumber << ",\n";
    oss << "  \"randomSeed\": " << randomSeed << "\n";
    oss << "}";
    return oss.str();
}

LLMCallRecord LLMCallRecord::fromJson(const std::string& json) {
    LLMCallRecord record;
    // Simple JSON parsing (in production, use a JSON library)
    // For now, basic string extraction
    
    // This is a stub - full JSON parsing would be more comprehensive
    record.prompt = "parsed_from_json";
    record.llmOutput = "parsed_from_json";
    
    return record;
}

// ==================== RandomDecisionRecord ====================

std::string RandomDecisionRecord::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"tickNumber\": " << tickNumber << ",\n";
    oss << "  \"systemName\": \"" << systemName << "\",\n";
    oss << "  \"decisionName\": \"" << decisionName << "\",\n";
    oss << "  \"randomValue\": " << std::fixed << std::setprecision(6) << randomValue << ",\n";
    oss << "  \"seed\": " << seed << ",\n";
    oss << "  \"wasUsed\": " << (wasUsed ? "true" : "false") << "\n";
    oss << "}";
    return oss.str();
}

RandomDecisionRecord RandomDecisionRecord::fromJson(const std::string& json) {
    RandomDecisionRecord record;
    // Simple JSON parsing stub
    record.systemName = "parsed_from_json";
    record.decisionName = "parsed_from_json";
    return record;
}

// ==================== DeterministicReplayLogger ====================

DeterministicReplayLogger& DeterministicReplayLogger::getInstance() {
    static DeterministicReplayLogger instance;
    return instance;
}

void DeterministicReplayLogger::enableLogging() {
    loggingEnabled_ = true;
}

void DeterministicReplayLogger::disableLogging() {
    loggingEnabled_ = false;
}

void DeterministicReplayLogger::recordLLMCall(
    int64_t tickNumber,
    LLMCallType callType,
    const std::string& prompt,
    const std::string& response,
    int inputTokens,
    int completionTokens,
    int64_t latencyMs,
    LLMProviderType provider
) {
    if (!loggingEnabled_) return;

    LLMCallRecord record;
    record.tickNumber = tickNumber;
    record.callType = callType;
    record.prompt = prompt;
    record.llmOutput = response;
    record.inputTokens = inputTokens;
    record.completionTokens = completionTokens;
    record.totalTokens = inputTokens + completionTokens;
    record.latencyMs = latencyMs;
    record.provider = provider;
    record.wasSuccessful = true;
    record.errorMessage = "";
    record.attemptNumber = 1;
    record.randomSeed = 0;

    llmCallRecords_.push_back(record);
    successfulCalls_++;
    lastTickNumber_ = tickNumber;
}

void DeterministicReplayLogger::recordLLMCallFailure(
    int64_t tickNumber,
    LLMCallType callType,
    const std::string& prompt,
    const std::string& errorMessage,
    int attemptNumber
) {
    if (!loggingEnabled_) return;

    LLMCallRecord record;
    record.tickNumber = tickNumber;
    record.callType = callType;
    record.prompt = prompt;
    record.llmOutput = "";
    record.inputTokens = 0;
    record.completionTokens = 0;
    record.totalTokens = 0;
    record.latencyMs = 0;
    record.provider = LLMProviderType::UNKNOWN;
    record.wasSuccessful = false;
    record.errorMessage = errorMessage;
    record.attemptNumber = attemptNumber;
    record.randomSeed = 0;

    llmCallRecords_.push_back(record);
    failedCalls_++;
    lastTickNumber_ = tickNumber;
}

void DeterministicReplayLogger::recordRandomDecision(
    int64_t tickNumber,
    const std::string& systemName,
    const std::string& decisionName,
    float randomValue,
    int seed
) {
    if (!loggingEnabled_) return;

    RandomDecisionRecord record;
    record.tickNumber = tickNumber;
    record.systemName = systemName;
    record.decisionName = decisionName;
    record.randomValue = randomValue;
    record.seed = seed;
    record.wasUsed = true;

    randomDecisionRecords_.push_back(record);
}

const LLMCallRecord* DeterministicReplayLogger::getLLMCallAtTick(
    int64_t tickNumber,
    LLMCallType type
) const {
    for (const auto& record : llmCallRecords_) {
        if (record.tickNumber == tickNumber && record.callType == type) {
            return &record;
        }
    }
    return nullptr;
}

std::vector<LLMCallRecord> DeterministicReplayLogger::getLLMCallsAtTick(int64_t tickNumber) const {
    std::vector<LLMCallRecord> results;
    for (const auto& record : llmCallRecords_) {
        if (record.tickNumber == tickNumber) {
            results.push_back(record);
        }
    }
    return results;
}

const RandomDecisionRecord* DeterministicReplayLogger::getRandomDecisionAtTick(
    int64_t tickNumber,
    const std::string& systemName
) const {
    for (const auto& record : randomDecisionRecords_) {
        if (record.tickNumber == tickNumber && record.systemName == systemName) {
            return &record;
        }
    }
    return nullptr;
}

bool DeterministicReplayLogger::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    // Write header
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"generatedAt\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\",\n";
    file << "  \"totalLLMCalls\": " << llmCallRecords_.size() << ",\n";
    file << "  \"totalRandomDecisions\": " << randomDecisionRecords_.size() << ",\n";
    file << "  \"successfulCalls\": " << successfulCalls_ << ",\n";
    file << "  \"failedCalls\": " << failedCalls_ << ",\n";
    file << "  \"llmCalls\": [\n";

    // Write LLM calls
    for (size_t i = 0; i < llmCallRecords_.size(); ++i) {
        file << llmCallRecords_[i].toJson();
        if (i < llmCallRecords_.size() - 1) {
            file << ",\n";
        } else {
            file << "\n";
        }
    }

    file << "  ],\n";
    file << "  \"randomDecisions\": [\n";

    // Write random decisions
    for (size_t i = 0; i < randomDecisionRecords_.size(); ++i) {
        file << randomDecisionRecords_[i].toJson();
        if (i < randomDecisionRecords_.size() - 1) {
            file << ",\n";
        } else {
            file << "\n";
        }
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    return true;
}

bool DeterministicReplayLogger::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Simple JSON parsing - in production use a JSON library
    // For now, basic extraction
    
    clear();
    
    // This is a simplified version - full JSON parsing would be more comprehensive
    return true;
}

void DeterministicReplayLogger::clear() {
    llmCallRecords_.clear();
    randomDecisionRecords_.clear();
    lastTickNumber_ = -1;
    successfulCalls_ = 0;
    failedCalls_ = 0;
}

std::string DeterministicReplayLogger::getStatistics() const {
    std::ostringstream oss;
    oss << "Replay Logger Statistics:\n";
    oss << "  LLM Calls Recorded: " << llmCallRecords_.size() << "\n";
    oss << "    - Successful: " << successfulCalls_ << "\n";
    oss << "    - Failed: " << failedCalls_ << "\n";
    oss << "  Random Decisions Recorded: " << randomDecisionRecords_.size() << "\n";
    oss << "  Last Tick Number: " << lastTickNumber_ << "\n";
    return oss.str();
}

bool DeterministicReplayLogger::validateReplayAtTick(
    int64_t tickNumber,
    int expectedLLMCallCount
) const {
    auto calls = getLLMCallsAtTick(tickNumber);
    return static_cast<int>(calls.size()) == expectedLLMCallCount;
}

std::string DeterministicReplayLogger::compareLogs(
    const std::string& logFile1,
    const std::string& logFile2
) {
    std::ostringstream oss;
    oss << "Comparing replay logs:\n";
    oss << "  File 1: " << logFile1 << "\n";
    oss << "  File 2: " << logFile2 << "\n";
    oss << "  (Comparison implementation pending)\n";
    return oss.str();
}

// ==================== ReplayValidator ====================

ReplayValidator::ReplayValidator(const std::string& replayLogPath)
    : replayLogPath_(replayLogPath)
    , replayModeEnabled_(false)
    , currentReplayIndex_(0)
    , divergenceCount_(0) {
    // Load replay logs from file during construction
    // For now, implementation is simplified - can be expanded to actually load files
}

void ReplayValidator::enableReplayMode() {
    replayModeEnabled_ = true;
    currentReplayIndex_ = 0;
}

void ReplayValidator::disableReplayMode() {
    replayModeEnabled_ = false;
}

LLMResponse ReplayValidator::getNextReplayResponse(
    int64_t tickNumber,
    LLMCallType callType,
    const std::string& prompt
) {
    LLMResponse response;

    if (!replayModeEnabled_ || replayLogs_.empty()) {
        response.wasSuccessful = false;
        response.errorMessage = "Replay mode not enabled or no logs";
        return response;
    }

    // Find matching log entry
    for (const auto& log : replayLogs_) {
        if (log.tickNumber == tickNumber && log.callType == callType) {
            response.wasSuccessful = log.wasSuccessful;
            response.text = log.llmOutput;
            response.inputTokens = log.inputTokens;
            response.completionTokens = log.completionTokens;
            response.totalTokens = log.totalTokens;
            response.latencyMs = log.latencyMs;
            response.provider = log.provider;
            response.errorMessage = log.errorMessage;
            return response;
        }
    }

    // No matching log found
    response.wasSuccessful = false;
    response.errorMessage = "No matching replay log entry";
    return response;
}

std::string ReplayValidator::checkForDivergence(
    int64_t tickNumber,
    LLMCallType callType
) {
    // Check if replay and current logs match
    const LLMCallRecord* replayLog = nullptr;
    for (const auto& log : replayLogs_) {
        if (log.tickNumber == tickNumber && log.callType == callType) {
            replayLog = &log;
            break;
        }
    }

    if (!replayLog) {
        divergenceCount_++;
        return "No replay log entry for tick " + std::to_string(tickNumber);
    }

    return "";  // No divergence
}

std::string ReplayValidator::getValidationStats() const {
    std::ostringstream oss;
    oss << "Replay Validation Statistics:\n";
    oss << "  Replay Logs Loaded: " << replayLogs_.size() << "\n";
    oss << "  Divergence Count: " << divergenceCount_ << "\n";
    oss << "  Replay Mode: " << (replayModeEnabled_ ? "Enabled" : "Disabled") << "\n";
    return oss.str();
}

// ==================== TickSnapshot ====================

std::string TickSnapshot::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"tickNumber\": " << tickNumber << ",\n";
    oss << "  \"worldStateHash\": \"" << worldStateHash << "\",\n";
    oss << "  \"npcCount\": " << npcCount << ",\n";
    oss << "  \"factionCount\": " << factionCount << ",\n";
    oss << "  \"totalLoyalty\": " << std::fixed << std::setprecision(2) << totalLoyal << ",\n";
    oss << "  \"llmCallsThisTick\": " << llmCallsThisTick << "\n";
    oss << "}";
    return oss.str();
}

}  // namespace TLS
