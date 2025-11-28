#pragma once

#include "LLMProvider.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace TLS {

/**
 * @enum LLMCallType
 * @brief Type of LLM call for logging
 */
enum class LLMCallType {
    DECISION_INTERPRETATION = 0,      // Player input interpretation
    WORLD_STATE_NARRATIVE = 1,         // World state analysis
    NPC_CONVERSATION = 2,              // NPC-to-NPC dialogue
    CRISIS_GENERATION = 3,             // Crisis narrative
    UNKNOWN = 4
};

/**
 * @struct LLMCallRecord
 * @brief Single recorded LLM call for deterministic replay
 */
struct LLMCallRecord {
    int64_t tickNumber = 0;
    LLMCallType callType = LLMCallType::UNKNOWN;
    std::string prompt;
    std::string llmOutput;
    int inputTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
    int64_t latencyMs = 0;
    LLMProviderType provider = LLMProviderType::UNKNOWN;
    bool wasSuccessful = true;
    std::string errorMessage;
    int attemptNumber = 0;
    int randomSeed = 0;

    /**
     * Serialize to JSON string
     */
    std::string toJson() const;

    /**
     * Deserialize from JSON string
     */
    static LLMCallRecord fromJson(const std::string& json);
};

/**
 * @struct RandomDecisionRecord
 * @brief Record of random decision for deterministic replay
 */
struct RandomDecisionRecord {
    int64_t tickNumber = 0;
    std::string systemName;       // e.g., "emotionCalculation", "eventTriggering"
    std::string decisionName;     // e.g., "npcMoodVariance", "immigrationCheck"
    float randomValue = 0.0f;     // Raw random value [0-1]
    int seed = 0;
    bool wasUsed = true;

    std::string toJson() const;
    static RandomDecisionRecord fromJson(const std::string& json);
};

/**
 * @class DeterministicReplayLogger
 * @brief Logs LLM calls and random decisions for deterministic replay and debugging
 *
 * Features:
 * - Frame-by-frame LLM call logging
 * - RNG decision tracking for replay
 * - State divergence detection
 * - Replay validation
 *
 * Usage:
 * 1. Record mode: Log all LLM calls and RNG decisions during normal gameplay
 * 2. Replay mode: Load logs and verify simulation produces identical state
 */
class DeterministicReplayLogger {
public:
    /**
     * Singleton instance
     */
    static DeterministicReplayLogger& getInstance();

    /**
     * Enable logging mode
     */
    void enableLogging();

    /**
     * Disable logging
     */
    void disableLogging();

    /**
     * Check if logging is enabled
     */
    bool isLoggingEnabled() const { return loggingEnabled_; }

    /**
     * Record LLM call
     * @param tickNumber Current simulation tick
     * @param callType Type of LLM call
     * @param prompt Input prompt
     * @param response LLM response
     * @param tokens Token counts
     * @param latencyMs Response latency in milliseconds
     * @param provider Provider used
     */
    void recordLLMCall(
        int64_t tickNumber,
        LLMCallType callType,
        const std::string& prompt,
        const std::string& response,
        int inputTokens,
        int completionTokens,
        int64_t latencyMs,
        LLMProviderType provider
    );

    /**
     * Record failed LLM call
     */
    void recordLLMCallFailure(
        int64_t tickNumber,
        LLMCallType callType,
        const std::string& prompt,
        const std::string& errorMessage,
        int attemptNumber
    );

    /**
     * Record random decision
     * @param tickNumber Current tick
     * @param systemName System making decision (e.g., "emotionCalculation")
     * @param decisionName Decision being made (e.g., "moodVariance")
     * @param randomValue Random value generated [0-1]
     * @param seed RNG seed used
     */
    void recordRandomDecision(
        int64_t tickNumber,
        const std::string& systemName,
        const std::string& decisionName,
        float randomValue,
        int seed
    );

    /**
     * Get LLM call record for given tick
     */
    const LLMCallRecord* getLLMCallAtTick(int64_t tickNumber, LLMCallType type) const;

    /**
     * Get all LLM calls at tick
     */
    std::vector<LLMCallRecord> getLLMCallsAtTick(int64_t tickNumber) const;

    /**
     * Get random decision for tick and system
     */
    const RandomDecisionRecord* getRandomDecisionAtTick(
        int64_t tickNumber,
        const std::string& systemName
    ) const;

    /**
     * Save log to file
     * @param filepath Path to save replay log
     * @return true if successful
     */
    bool saveToFile(const std::string& filepath) const;

    /**
     * Load log from file
     * @param filepath Path to replay log file
     * @return true if successful
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * Clear all records
     */
    void clear();

    /**
     * Get total number of recorded LLM calls
     */
    size_t getLLMCallCount() const { return llmCallRecords_.size(); }

    /**
     * Get total number of random decisions recorded
     */
    size_t getRandomDecisionCount() const { return randomDecisionRecords_.size(); }

    /**
     * Get statistics
     */
    std::string getStatistics() const;

    /**
     * Validate replay: check if current state matches logged state
     * @param tickNumber Tick to validate
     * @param expectedLLMCallCount Expected number of LLM calls at this tick
     * @return true if matches
     */
    bool validateReplayAtTick(int64_t tickNumber, int expectedLLMCallCount) const;

    /**
     * Compare two replay logs for differences
     */
    static std::string compareLogs(
        const std::string& logFile1,
        const std::string& logFile2
    );

private:
    DeterministicReplayLogger() = default;

    std::vector<LLMCallRecord> llmCallRecords_;
    std::vector<RandomDecisionRecord> randomDecisionRecords_;
    bool loggingEnabled_ = true;
    int64_t lastTickNumber_ = -1;
    size_t successfulCalls_ = 0;
    size_t failedCalls_ = 0;
};

/**
 * @class ReplayValidator
 * @brief Validates simulation determinism by comparing replay with current run
 */
class ReplayValidator {
public:
    /**
     * Initialize validator with replay log
     * @param replayLogPath Path to saved replay log
     */
    explicit ReplayValidator(const std::string& replayLogPath);

    /**
     * Enable replay validation mode
     * This makes the LLM provider return logged responses instead of calling API
     */
    void enableReplayMode();

    /**
     * Disable replay mode
     */
    void disableReplayMode();

    /**
     * Check if validation is enabled
     */
    bool isReplayModeEnabled() const { return replayModeEnabled_; }

    /**
     * Get next LLM response from log
     * For replay-mode providers to use
     */
    LLMResponse getNextReplayResponse(
        int64_t tickNumber,
        LLMCallType callType,
        const std::string& prompt
    );

    /**
     * Check for divergence between current run and replay
     * @param tickNumber Current tick
     * @param callType Type of LLM call
     * @return Error message if divergence detected, empty string if no divergence
     */
    std::string checkForDivergence(int64_t tickNumber, LLMCallType callType);

    /**
     * Get validation statistics
     */
    std::string getValidationStats() const;

private:
    std::string replayLogPath_;
    std::vector<LLMCallRecord> replayLogs_;
    bool replayModeEnabled_ = false;
    size_t currentReplayIndex_ = 0;
    int divergenceCount_ = 0;
};

/**
 * @class TickSnapshot
 * @brief Snapshot of world state at a given tick for validation
 */
struct TickSnapshot {
    int64_t tickNumber = 0;
    std::string worldStateHash;        // Hash of world state
    int npcCount = 0;
    int factionCount = 0;
    float totalLoyal = 0.0f;
    int llmCallsThisTick = 0;
    std::vector<std::string> npcStates;  // Per-NPC state snapshots

    std::string toJson() const;
};

}  // namespace TLS
