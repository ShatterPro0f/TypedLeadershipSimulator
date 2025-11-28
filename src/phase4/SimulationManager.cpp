#include "SimulationManager.h"
#include "InputParser.h"
#include "DialogueSystem.h"
#include "GameTime.h"
#include "UIFormatter.h"
#include "ReplaySystem.h"
#include "ProximityDetection.h"
#include "NPCMovement.h"
#include "ActivitySystem.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

using namespace TLS;

SimulationManager::SimulationManager() 
    : player_("Player", Vector3(0, 0, 0)), rng_(42) {}

SimulationManager& SimulationManager::getInstance()
{
    static SimulationManager instance;
    return instance;
}

void SimulationManager::initialize(NPCRegistry& registry, FactionRegistry& factionRegistry,
                                  ResourceRegistry& resourceRegistry, AdvisorRegistry& advisorRegistry,
                                  const World& world, const WaypointGraph& graph,
                                  const LLMConfig& llmConfig)
{
    registry_ = &registry;
    factionRegistry_ = &factionRegistry;
    resourceRegistry_ = &resourceRegistry;
    advisorRegistry_ = &advisorRegistry;
    world_ = &world;
    graph_ = &graph;

    player_.position = Vector3(0.0f, 0.0f, 0.0f);
    player_.moveSpeed = 5.0f;
    player_.collisionRadius = 0.5f;
    player_.eyeHeight = 1.7f;

    worldState_.setMoodThreshold(0.2f);
    worldState_.setLoyaltyThreshold(0.15f);
    worldState_.setResourceThreshold(50);

    // TODO: Initialize LLM manager
    // llmManager_ = std::make_unique<LLMManager>(llmConfig);
    llmManager_ = LLMManager::instance();

    // Initialize dialogue system
    DialogueSystem::getInstance().initialize(0.3f);

    // Take initial NPC state snapshots
    auto allNPCs = registry_->getAllNPCs();
    for (const auto& npc : allNPCs)
    {
        if (npc)
        {
            NPCStateSnapshot snapshot;
            snapshot.mood = npc->getShortTermMood();
            snapshot.loyalty = npc->getLoyalty();
            snapshot.immediateEmotion = npc->getImmediateEmotion();
            snapshot.position = npc->getPosition();
            snapshot.lastStateChangeTick = 0;
            previousNPCState_[npc->getId()] = snapshot;

            npcPositionHistory_[npc->getId()] = std::vector<Vector3>();
        }
    }
}

void SimulationManager::tick(float deltaTime)
{
    tick_++;
    gameTime_ += deltaTime;

    // Seed RNG for this tick (deterministic randomness)
    rng_.seed(globalSeed_ + tick_);

    // Phase 12: Process queued player input (non-blocking)
    // Process one input per tick to avoid overwhelming the system
    if (hasPlayerInput() && !isInConversation())
    {
        std::string command = getNextPlayerInput();
        if (!command.empty())
        {
            processPlayerInput(command);
        }
    }

    // Update all systems in order
    updatePlayerMovement(deltaTime);
    updateNPCMovement(deltaTime);
    updateNPCActivities();
    updateNPCEmotions();
    
    // Check for player-NPC interactions
    checkProximityInteractions();
    
    // Monitor world state and trigger LLM narrative generation if needed
    monitorWorldStateChanges();
    
    // Process conversation queue and handle NPC dialogue
    processConversationQueue();

    // TODO: Update LLM manager (process async callbacks)
    // if (llmManager_)
    // {
    //     llmManager_->update();
    // }

    // TODO: Log tick for replay system if enabled
    // if (replayMode_)
    // {
    //     ReplaySystem::getInstance().recordTickSnapshot(tick_, registry_->getAllNPCs(),
    //                                                    resourceRegistry_->getAllResources());
    // }
}

int SimulationManager::getGameHour() const
{
    return GameTime::getGameHour(static_cast<int>(gameTime_));
}

int SimulationManager::getGameMinute() const
{
    return GameTime::getGameMinute(static_cast<int>(gameTime_));
}

int SimulationManager::getGameDay() const
{
    return GameTime::getGameDay(static_cast<int>(gameTime_));
}

int SimulationManager::getGameSeason() const
{
    return GameTime::getGameSeason(static_cast<int>(gameTime_));
}

std::string SimulationManager::getTimeString() const
{
    return GameTime::formatTime(static_cast<int>(gameTime_));
}

void SimulationManager::processPlayerInput(const std::string& command)
{
    if (!llmManager_)
        return;

    // Parse player input
    InputParser& parser = InputParser::getInstance();
    ParseResult parseResult = parser.parseInput(command, ActionRegistry::getInstance());

    // Validate parameters
    if (!parser.validateParameters(parseResult, *registry_, *factionRegistry_, *resourceRegistry_))
    {
        UIFormatter::getInstance().formatLowConfidencePrompt(command);
        return;
    }

    // High confidence: execute immediately
    if (parseResult.confidence >= 0.9f)
    {
        // Send to LLM for decision interpretation (TODO: wire to LLMManager)
        // For now, use a stub interpretation
        InterpretedDecision decision;
        decision.action = parseResult.action;
        decision.tone = InputParser::getInstance().detectTone(command);
        onDecisionInterpretationComplete(decision);
    }
    // Medium confidence: ask for confirmation
    else if (parseResult.confidence >= 0.7f)
    {
        // TODO: Prompt player for confirmation
        // For now, proceed with execution (stub)
        InterpretedDecision decision;
        decision.action = parseResult.action;
        decision.tone = InputParser::getInstance().detectTone(command);
        onDecisionInterpretationComplete(decision);
    }
    // Ambiguous: present options
    else if (parseResult.confidence >= 0.6f)
    {
        auto ambiguous = parser.findAmbiguousMatches(command, ActionRegistry::getInstance());
        // TODO: Present options to player
    }
    // Low confidence: ask to rephrase
    else
    {
        // TODO: Suggest closest matches
    }
}

// Phase 12: Player input queue methods
void SimulationManager::queuePlayerInput(const std::string& command)
{
    playerInputQueue_.push(command);
}

bool SimulationManager::hasPlayerInput() const
{
    return !playerInputQueue_.empty();
}

std::string SimulationManager::getNextPlayerInput()
{
    if (playerInputQueue_.empty())
        return "";
    
    std::string command = playerInputQueue_.front();
    playerInputQueue_.pop();
    return command;
}

// Phase 12: Conversation state setter
void SimulationManager::setInConversation(bool inConversation)
{
    if (inConversation)
    {
        // Set a dummy NPC id (0) to mark as in conversation
        // Tests can override this by setting currentConversationNpcId_ directly if needed
        currentConversationNpcId_ = 0;
    }
    else
    {
        currentConversationNpcId_ = -1;
    }
}

void SimulationManager::respondToCurrentConversation(const std::string& response)
{
    if (currentConversationNpcId_ == -1)
        return;

    auto npc = registry_->getNPCById(currentConversationNpcId_);
    if (!npc)
        return;

    // Parse player response as a decision
    InputParser& parser = InputParser::getInstance();
    ParseResult parseResult = parser.parseInput(response, ActionRegistry::getInstance());

    // Create interpreted decision from player response (stub)
    InterpretedDecision decision;
    decision.action = parseResult.action;
    decision.tone = parser.detectTone(response);
    onDecisionInterpretationComplete(decision);
}

void SimulationManager::endCurrentConversation()
{
    if (currentConversationNpcId_ != -1)
    {
        auto npc = registry_->getNPCById(currentConversationNpcId_);
        if (npc)
        {
            DialogueSystem::getInstance().resolveProblem(*npc);
        }
    }

    currentConversationNpcId_ = -1;
    currentConversationText_ = "";
    conversationDelayTicks_ = CONVERSATION_DELAY;
}

void SimulationManager::updatePlayerMovement(float deltaTime)
{
    if (!world_)
        return;

    player_.update(deltaTime, *world_);

    // Clamp player position to reasonable world bounds
    Vector3 pos = player_.position;
    
    // Simple bounds clamping (tune these values per your world size)
    if (pos.x < -500.0f) pos.x = -500.0f;
    if (pos.x > 500.0f) pos.x = 500.0f;
    if (pos.z < -500.0f) pos.z = -500.0f;
    if (pos.z > 500.0f) pos.z = 500.0f;
    if (pos.y < -10.0f) pos.y = -10.0f;
    if (pos.y > 50.0f) pos.y = 50.0f;

    player_.position = pos;
    player_.updateOrientation();
}

void SimulationManager::updateNPCMovement(float deltaTime)
{
    if (!registry_ || !world_ || !graph_)
        return;

    auto allNPCs = registry_->getAllNPCs();
    for (auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        Activity currentActivity = npc->getActivity();
        Vector3 destination = npc->getDestination();

        NPCMovement::updateNPCPosition(*npc, currentActivity, destination, deltaTime,
                                      *world_, *graph_, tick_);

        // Track position history for stuck detection
        auto& posHistory = npcPositionHistory_[npc->getId()];
        posHistory.push_back(npc->getPosition());
        if (posHistory.size() > MAX_POSITION_HISTORY)
        {
            posHistory.erase(posHistory.begin());
        }

        // Check for stuck condition
        if (NPCMovement::isNPCStuck(*npc, posHistory))
        {
            NPCMovement::recoverFromStuck(*npc, destination, *graph_, *world_);
        }
    }
}

void SimulationManager::updateNPCActivities()
{
    if (!registry_ || !world_ || !graph_)
        return;

    auto allNPCs = registry_->getAllNPCs();
    for (auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        Activity newActivity = NPCMovement::determineActivity(*npc, worldState_, *graph_);
        npc->setActivity(newActivity);
    }
}

void SimulationManager::updateNPCEmotions()
{
    if (!registry_)
        return;

    auto allNPCs = registry_->getAllNPCs();
    for (auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        updateNPCMood(*npc);
        updateNPCAttitude(*npc);

        // Clamp emotional states to [0, 1]
        float mood = npc->getShortTermMood();
        mood = std::max(0.0f, std::min(1.0f, mood));
        npc->setShortTermMood(mood);

        float attitude = npc->getLongTermAttitude();
        attitude = std::max(0.0f, std::min(1.0f, attitude));
        npc->setLongTermAttitude(attitude);
    }
}

void SimulationManager::updateNPCMood(NPC& npc)
{
    // M_s(t) = α*E_i + (1-α)*M_s(t-1)  where α ≈ 0.1
    const float ALPHA = 0.1f;
    float currentMood = npc.getShortTermMood();
    float immediateEmotion = npc.getImmediateEmotion();

    float newMood = ALPHA * immediateEmotion + (1.0f - ALPHA) * currentMood;
    npc.setShortTermMood(newMood);
}

void SimulationManager::updateNPCAttitude(NPC& npc)
{
    // A_l(t) = A_l(t-1) + β*M_s(t)  where β ≈ 0.01
    const float BETA = 0.01f;
    float currentAttitude = npc.getLongTermAttitude();
    float currentMood = npc.getShortTermMood();

    float newAttitude = currentAttitude + BETA * currentMood;
    npc.setLongTermAttitude(newAttitude);
}

void SimulationManager::checkProximityInteractions()
{
    if (!registry_)
        return;

    const float PROXIMITY_RANGE = 10.0f;

    std::vector<int> nearbyNPCIds;
    auto allNPCs = registry_->getAllNPCs();
    for (const auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        float distToPlayer = player_.getDistanceTo(npc->getPosition());
        if (distToPlayer < PROXIMITY_RANGE)
        {
            nearbyNPCIds.push_back(npc->getId());
        }
    }

    // Calculate problem severity for nearby NPCs
    for (int npcId : nearbyNPCIds)
    {
        auto npc = registry_->getNPCById(npcId);
        if (!npc)
            continue;

        auto prevIt = previousNPCState_.find(npc->getId());
        if (prevIt == previousNPCState_.end())
            continue;

        NPCStateSnapshot& prevState = prevIt->second;

        float moodDelta = npc->getShortTermMood() - prevState.mood;
        float loyaltyDelta = npc->getLoyalty() - prevState.loyalty;

        float severity = calculateProblemSeverity(*npc, std::abs(moodDelta), std::abs(loyaltyDelta));
        // Check if NPC should initiate dialogue
        if (DialogueSystem::getInstance().shouldInitiateDialogue(*npc, tick_) && !isInConversation())
        {
            // For now, use simple loyalty-based influence score
            // TODO: Enhance with Advisor influence calculation
            float influence = npc->getLoyalty() * 0.7f;

            addNPCToConversationQueue(*npc, severity, influence, 0.0f);
        }
    }
}

void SimulationManager::processConversationQueue()
{
    if (conversationDelayTicks_ > 0)
    {
        conversationDelayTicks_--;
        return;
    }

    if (currentConversationNpcId_ == -1 && !conversationQueue_.empty())
    {
        // Dequeue next highest priority NPC
        sortConversationQueue();
        ConversationQueueEntry entry = conversationQueue_.front();
        conversationQueue_.erase(conversationQueue_.begin());

        currentConversationNpcId_ = entry.npcId;

        // Generate dialogue text
        auto npc = registry_->getNPCById(entry.npcId);
        if (npc)
        {
            currentConversationText_ = DialogueSystem::getInstance().generateDialogueText(
                *npc, DialogueSystem::getInstance().getProblemState(*npc));
        }
    }
}

void SimulationManager::monitorWorldStateChanges()
{
    if (!registry_)
        return;

    hasSignificantChange_ = detectSignificantWorldStateChange();

    // Update previous NPC states for next frame's delta detection
    auto allNPCs = registry_->getAllNPCs();
    for (const auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        auto prevIt = previousNPCState_.find(npc->getId());
        if (prevIt == previousNPCState_.end())
        {
            NPCStateSnapshot snapshot;
            snapshot.mood = npc->getShortTermMood();
            snapshot.loyalty = npc->getLoyalty();
            snapshot.immediateEmotion = npc->getImmediateEmotion();
            snapshot.position = npc->getPosition();
            snapshot.lastStateChangeTick = tick_;
            previousNPCState_[npc->getId()] = snapshot;
        }
        else
        {
            NPCStateSnapshot& prevState = prevIt->second;
            prevState.mood = npc->getShortTermMood();
            prevState.loyalty = npc->getLoyalty();
            prevState.immediateEmotion = npc->getImmediateEmotion();
            prevState.position = npc->getPosition();
        }
    }

    // Trigger narrative generation if significant change and enough time has passed
    if (hasSignificantChange_ && (tick_ - lastLLMSnapshotTick_) >= LLM_SNAPSHOT_DEBOUNCE)
    {
        triggerNarrativeGeneration();
        lastLLMSnapshotTick_ = tick_;
    }
}

bool SimulationManager::detectSignificantWorldStateChange()
{
    if (!registry_)
        return false;

    const float MOOD_THRESHOLD = 0.2f;
    const float LOYALTY_THRESHOLD = 0.15f;

    auto allNPCs = registry_->getAllNPCs();
    for (const auto& npc : allNPCs)
    {
        if (!npc)
            continue;

        auto prevIt = previousNPCState_.find(npc->getId());
        if (prevIt == previousNPCState_.end())
            continue;

        const NPCStateSnapshot& prevState = prevIt->second;

        float moodDelta = std::abs(npc->getShortTermMood() - prevState.mood);
        float loyaltyDelta = std::abs(npc->getLoyalty() - prevState.loyalty);

        if (moodDelta > MOOD_THRESHOLD || loyaltyDelta > LOYALTY_THRESHOLD)
        {
            return true;
        }
    }

    return false;
}

void SimulationManager::triggerNarrativeGeneration()
{
    if (!llmManager_)
        return;

    std::vector<int> affectedNpcs;
    auto allNpcs = registry_->getAllNPCs();
    for (const auto& npc : allNpcs)
    {
        if (npc)
        {
            affectedNpcs.push_back(npc->getId());
        }
    }

    // TODO: Call LLMManager for narrative generation
    // For now, use stub (no narrative issues generated)
    std::vector<std::string> narrativeIssues;
    onNarrativeGenerationComplete(narrativeIssues);
}

float SimulationManager::calculateProblemSeverity(const NPC& npc, float moodDelta, float loyaltyDelta) const
{
    // severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
    return 0.5f * moodDelta + 0.5f * loyaltyDelta;
}

void SimulationManager::addNPCToConversationQueue(const NPC& npc, float severity, float influence, float leadershipBonus)
{
    // Check if already in queue
    for (const auto& entry : conversationQueue_)
    {
        if (entry.npcId == npc.getId())
            return;
    }

    ConversationQueueEntry entry;
    entry.npcId = npc.getId();
    entry.problemSeverity = severity;
    entry.influenceScore = influence + leadershipBonus;
    entry.tickArrived = tick_;

    conversationQueue_.push_back(entry);
    sortConversationQueue();

    // Cap queue at 5 entries
    if (conversationQueue_.size() > 5)
    {
        conversationQueue_.pop_back();
    }
}

void SimulationManager::sortConversationQueue()
{
    const float WEIGHT_SEVERITY = 0.6f;
    const float WEIGHT_INFLUENCE = 0.4f;

    std::sort(conversationQueue_.begin(), conversationQueue_.end(),
              [WEIGHT_SEVERITY, WEIGHT_INFLUENCE](const ConversationQueueEntry& a, const ConversationQueueEntry& b) {
                  float priorityA = a.problemSeverity * WEIGHT_SEVERITY + a.influenceScore * WEIGHT_INFLUENCE;
                  float priorityB = b.problemSeverity * WEIGHT_SEVERITY + b.influenceScore * WEIGHT_INFLUENCE;
                  return priorityA > priorityB;
              });
}

void SimulationManager::onDecisionInterpretationComplete(const InterpretedDecision& decision)
{
    if (decision.targetNpcId == -1)
        return;

    auto npc = registry_->getNPCById(decision.targetNpcId);
    if (!npc)
        return;

    applyDecisionConsequences(*npc, decision);

    // Log for replay
    if (replayMode_)
    {
        logLLMCall("decision_interpretation", decision.context, decision.action);
    }
}

void SimulationManager::onNarrativeGenerationComplete(const std::vector<std::string>& narrativeIssues)
{
    activeNarrativeIssues_ = narrativeIssues;
}

void SimulationManager::applyDecisionConsequences(const NPC& npc, const InterpretedDecision& decision)
{
    auto mutableNpc = const_cast<NPC*>(&npc);

    // Update NPC emotion based on tone
    updateNPCEmotion(*mutableNpc, decision.tone);

    // Update mood and attitude
    updateNPCMood(*mutableNpc);
    updateNPCAttitude(*mutableNpc);

    // Update faction loyalties
    if (decision.targetFactionId != -1)
    {
        updateFactionLoyalties(npc.getId(), 0.1f * (decision.tone == "positive" ? 1.0f : -1.0f));
    }

    // Apply resource allocation if applicable
    if (!decision.targetResource.empty() && decision.quantity > 0)
    {
        auto resource = resourceRegistry_->getResourceByName(decision.targetResource);
        if (resource)
        {
            resource->addQuantity(-static_cast<int>(decision.quantity));
        }
    }
}

void SimulationManager::updateNPCEmotion(NPC& npc, const std::string& tone)
{
    // E_i = f(tone, relevance, bias, socialPressure)
    // Simplified: tone determines emotional shift
    float immediateEmotion = npc.getImmediateEmotion();

    if (tone == "positive")
    {
        immediateEmotion = std::min(1.0f, immediateEmotion + 0.2f);
    }
    else if (tone == "negative")
    {
        immediateEmotion = std::max(0.0f, immediateEmotion - 0.2f);
    }
    // neutral: no change

    npc.setImmediateEmotion(immediateEmotion);
}

void SimulationManager::updateFactionLoyalties(int targetNpcId, float loyaltyDelta)
{
    auto npc = registry_->getNPCById(targetNpcId);
    if (!npc)
        return;

    int factionId = npc->getFactionId();
    if (factionId == -1)
        return;

    auto faction = factionRegistry_->getFactionById(factionId);
    if (!faction)
        return;

    // Update all faction members' loyalty
    auto factionMembers = registry_->getNPCsByFaction(factionId);
    for (auto& member : factionMembers)
    {
        if (member)
        {
            float newLoyalty = member->getLoyalty() + loyaltyDelta;
            member->setLoyalty(newLoyalty);
        }
    }
}

void SimulationManager::logLLMCall(const std::string& callType, const std::string& prompt, const std::string& response)
{
    if (replayMode_)
    {
        ReplaySystem::getInstance().recordLLMCall(tick_, callType, prompt, response, 0, 0.0f);
    }
}

bool SimulationManager::saveToBinary(const std::string& filepath) const
{
    // TODO: Implement binary serialization
    return false;
}

bool SimulationManager::loadFromBinary(const std::string& filepath)
{
    // TODO: Implement binary deserialization
    return false;
}
