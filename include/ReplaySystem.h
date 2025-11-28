#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdint>
#include <fstream>

namespace TLS {

/**
 * @class ReplaySystem
 * @brief Records and validates deterministic replay of simulation
 *
 * Features:
 * - RNG seeding per frame
 * - LLM call logging with inputs/outputs
 * - Tick-by-tick state snapshots
 * - Divergence detection and reporting
 * - Frame-by-frame debugging
 *
 * Purpose: Enable byte-identical replay for debugging, testing, and tournament play
 */
class ReplaySystem {
public:
    // LLM call recorded in replay log
    struct LLMCallLog {
        int tick;
        std::string callType;         // "decision_interpretation", "narrative_generation"
        std::string prompt;
        std::string llmOutput;
        int tokensUsed;
        float duration;
    };

    // RNG decision recorded in replay log
    struct RNGDecision {
        int tick;
        std::string context;          // "npc_mood_variance", "conversation_selection", etc.
        uint64_t randomValue;         // The random number generated
        uint64_t seed;                // Seed used for this tick
    };

    // Tick state snapshot for divergence detection
    struct TickSnapshot {
        int tick;
        uint32_t stateChecksum;       // CRC32 of world state
        std::vector<float> npcMoods;
        std::vector<float> npcLoyalties;
        std::vector<int> resourceLevels;
    };

    static ReplaySystem& getInstance();

    // Recording interface
    void startRecording(const std::string& saveFile, uint64_t globalSeed);
    void stopRecording();

    void recordLLMCall(int tick, const std::string& callType, const std::string& prompt,
                     const std::string& output, int tokens, float duration);
    void recordRNGDecision(int tick, const std::string& context, uint64_t randomValue, uint64_t seed);
    void recordTickSnapshot(int tick, const std::vector<class NPC*>& npcs,
                           const std::vector<class Resource*>& resources);

    bool saveReplayLog(const std::string& logFile);

    // Replay interface
    bool loadReplayLog(const std::string& logFile);

    std::string getLLMCallOutput(int tick, const std::string& callType) const;
    std::pair<bool, uint64_t> getRNGDecision(int tick, const std::string& context) const;

    // Validation
    bool validateDeterminism(const std::string& originalSave, const std::string& replaySave,
                            std::string& divergenceReport);

    bool validateTickSnapshot(int tick, const std::vector<class NPC*>& npcs,
                             const std::vector<class Resource*>& resources,
                             std::string& divergenceReport);

    bool didDivergeAtTick(int tick) const { return divergenceTicks_.count(tick) > 0; }
    int getFirstDivergenceTick() const;

    // Debugging
    void enableFrameStep(bool enable) { frameStepMode_ = enable; }
    void stepFrame();
    bool isFrameStepping() const { return frameStepMode_; }

    void dumpTickState(int tick, std::ostream& out) const;
    std::string getStateChangesSinceLastTick(int tick) const;

private:
    ReplaySystem();
    ReplaySystem(const ReplaySystem&) = delete;
    ReplaySystem& operator=(const ReplaySystem&) = delete;

    // Recording state
    bool isRecording_ = false;
    std::string currentSaveFile_;
    uint64_t globalSeed_ = 42;

    // Recorded data
    std::map<int, std::vector<LLMCallLog>> llmCallLog_;
    std::map<int, std::vector<RNGDecision>> rngDecisionLog_;
    std::vector<TickSnapshot> tickSnapshots_;

    // Replay state
    bool isReplaying_ = false;
    std::set<int> divergenceTicks_;
    std::string lastDivergenceReport_;

    // Debugging
    bool frameStepMode_ = false;
    int currentDebugFrame_ = 0;

    // Helper methods
    uint32_t calculateChecksum(const std::vector<class NPC*>& npcs,
                              const std::vector<class Resource*>& resources);
    std::string generateDivergenceReport(int tick, const std::string& issue);

    // Serialization
    bool writeLLMCallLog(std::ofstream& file, const LLMCallLog& log);
    bool readLLMCallLog(std::ifstream& file, LLMCallLog& log);
    bool writeRNGDecision(std::ofstream& file, const RNGDecision& decision);
    bool readRNGDecision(std::ifstream& file, RNGDecision& decision);
};

}  // namespace TLS
