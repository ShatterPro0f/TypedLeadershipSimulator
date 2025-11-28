#include "ReplaySystem.h"
#include <fstream>
#include <iostream>
#include <cstdint>

using namespace TLS;

ReplaySystem::ReplaySystem()
    : isRecording_(false), isReplaying_(false), globalSeed_(0)
{
}

ReplaySystem& ReplaySystem::getInstance()
{
    static ReplaySystem instance;
    return instance;
}

void ReplaySystem::startRecording(const std::string& saveFile, uint64_t globalSeed)
{
    isRecording_ = true;
    currentSaveFile_ = saveFile;
    globalSeed_ = globalSeed;
    llmCallLog_.clear();
    rngDecisionLog_.clear();
    tickSnapshots_.clear();
}

void ReplaySystem::stopRecording()
{
    isRecording_ = false;
}

void ReplaySystem::recordLLMCall(int tick, const std::string& callType, const std::string& prompt,
                               const std::string& output, int tokens, float duration)
{
    if (!isRecording_)
        return;

    LLMCallLog log;
    log.tick = tick;
    log.callType = callType;
    log.prompt = prompt;
    log.llmOutput = output;
    log.tokensUsed = tokens;
    log.duration = duration;

    llmCallLog_[tick].push_back(log);
}

void ReplaySystem::recordRNGDecision(int tick, const std::string& context, uint64_t randomValue, uint64_t seed)
{
    if (!isRecording_)
        return;

    RNGDecision decision;
    decision.tick = tick;
    decision.context = context;
    decision.randomValue = randomValue;
    decision.seed = seed;

    rngDecisionLog_[tick].push_back(decision);
}

void ReplaySystem::recordTickSnapshot(int tick, const std::vector<class NPC*>& npcs,
                                     const std::vector<class Resource*>& resources)
{
    if (!isRecording_)
        return;

    // Placeholder implementation
    TickSnapshot snapshot;
    snapshot.tick = tick;
    snapshot.stateChecksum = calculateChecksum(npcs, resources);

    tickSnapshots_.push_back(snapshot);
}

bool ReplaySystem::saveReplayLog(const std::string& logFile)
{
    std::ofstream file(logFile, std::ios::binary);
    if (!file.is_open())
        return false;

    // Write header
    file.write(reinterpret_cast<const char*>(&globalSeed_), sizeof(globalSeed_));

    // Write LLM call logs
    for (const auto& pair : llmCallLog_)
    {
        for (const auto& log : pair.second)
        {
            writeLLMCallLog(file, log);
        }
    }

    file.close();
    return true;
}

bool ReplaySystem::loadReplayLog(const std::string& logFile)
{
    std::ifstream file(logFile, std::ios::binary);
    if (!file.is_open())
        return false;

    isReplaying_ = true;
    // Read and parse log file
    file.close();
    return true;
}

std::string ReplaySystem::getLLMCallOutput(int tick, const std::string& callType) const
{
    auto it = llmCallLog_.find(tick);
    if (it != llmCallLog_.end())
    {
        for (const auto& log : it->second)
        {
            if (log.callType == callType)
            {
                return log.llmOutput;
            }
        }
    }
    return "";
}

std::pair<bool, uint64_t> ReplaySystem::getRNGDecision(int tick, const std::string& context) const
{
    auto it = rngDecisionLog_.find(tick);
    if (it != rngDecisionLog_.end())
    {
        for (const auto& decision : it->second)
        {
            if (decision.context == context)
            {
                return {true, decision.randomValue};
            }
        }
    }
    return {false, 0};
}

bool ReplaySystem::validateDeterminism(const std::string& originalSave, const std::string& replaySave,
                                      std::string& divergenceReport)
{
    // Placeholder: In full version, compare save files byte-by-byte
    return true;
}

bool ReplaySystem::validateTickSnapshot(int tick, const std::vector<class NPC*>& npcs,
                                       const std::vector<class Resource*>& resources,
                                       std::string& divergenceReport)
{
    // Placeholder: In full version, compare checksums
    return true;
}

int ReplaySystem::getFirstDivergenceTick() const
{
    if (divergenceTicks_.empty())
        return -1;
    return *divergenceTicks_.begin();
}

void ReplaySystem::stepFrame()
{
    if (frameStepMode_)
    {
        currentDebugFrame_++;
    }
}

void ReplaySystem::dumpTickState(int tick, std::ostream& out) const
{
    out << "Tick " << tick << " state dump:\n";
    auto it = llmCallLog_.find(tick);
    if (it != llmCallLog_.end())
    {
        for (const auto& log : it->second)
        {
            out << "  LLM Call: " << log.callType << "\n";
            out << "    Output: " << log.llmOutput << "\n";
        }
    }
}

std::string ReplaySystem::getStateChangesSinceLastTick(int tick) const
{
    return "State changes for tick " + std::to_string(tick);
}

uint32_t ReplaySystem::calculateChecksum(const std::vector<class NPC*>& npcs,
                                        const std::vector<class Resource*>& resources)
{
    // Placeholder: Simple checksum (full version would use CRC32 or similar)
    return 0;
}

std::string ReplaySystem::generateDivergenceReport(int tick, const std::string& issue)
{
    return "Divergence at tick " + std::to_string(tick) + ": " + issue;
}

bool ReplaySystem::writeLLMCallLog(std::ofstream& file, const LLMCallLog& log)
{
    // Placeholder: Binary serialization
    return true;
}

bool ReplaySystem::readLLMCallLog(std::ifstream& file, LLMCallLog& log)
{
    // Placeholder: Binary deserialization
    return true;
}

bool ReplaySystem::writeRNGDecision(std::ofstream& file, const RNGDecision& decision)
{
    // Placeholder: Binary serialization
    return true;
}

bool ReplaySystem::readRNGDecision(std::ifstream& file, RNGDecision& decision)
{
    // Placeholder: Binary deserialization
    return true;
}
