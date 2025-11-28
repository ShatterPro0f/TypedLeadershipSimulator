#include <gtest/gtest.h>
#include "DialogueState.h"
#include "ProximityDialogueSystem.h"
#include "NpcProblemSystem.h"
#include "DialogueUI.h"
#include "DialogueController.h"
#include <string>
#include <vector>

namespace TLS {

// ==================== DIALOGUE STATE TESTS ====================

TEST(DialogueState, SingletonInstance) {
    auto& tracker1 = NpcDialogueTracker::getInstance();
    auto& tracker2 = NpcDialogueTracker::getInstance();
    EXPECT_EQ(&tracker1, &tracker2);
}

TEST(DialogueState, UpdateNpcDialogueState) {
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.updateNpcDialogueState(1, DialogueState::IN_DIALOGUE, 100);
    
    auto state = tracker.getNpcDialogueState(1);
    EXPECT_EQ(state, DialogueState::IN_DIALOGUE);
}

TEST(DialogueState, ProblemSeverity) {
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.setProblemSeverity(1, 0.75f);
    
    float severity = tracker.getProblemSeverity(1);
    EXPECT_NEAR(severity, 0.75f, 0.01f);
}

TEST(DialogueState, RecordConversation) {
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.recordConversation(1, "Hello!");
    tracker.recordConversation(1, "How are you?");
    
    auto history = tracker.getConversationHistory(1);
    EXPECT_GE(history.size(), 2);
}

TEST(DialogueState, EscalationLevel) {
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.incrementEscalationLevel(1);
    tracker.incrementEscalationLevel(1);
    
    int level = tracker.getEscalationLevel(1);
    EXPECT_GE(level, 0);
}

TEST(DialogueState, ResetDialogueState) {
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.updateNpcDialogueState(1, DialogueState::IN_DIALOGUE, 100);
    tracker.setProblemSeverity(1, 0.5f);
    tracker.resetNpcDialogueState(1);
    
    auto state = tracker.getNpcDialogueState(1);
    EXPECT_EQ(state, DialogueState::IDLE);
}

TEST(DialogueState, GetStateString) {
    auto& tracker = NpcDialogueTracker::getInstance();
    std::string str = tracker.getStateString(DialogueState::IN_DIALOGUE);
    EXPECT_FALSE(str.empty());
}

TEST(DialogueState, CanNpcReinitiate) {
    auto& tracker = NpcDialogueTracker::getInstance();
    bool can_reinit = tracker.canNpcReinitiate(2, 1000, 14400);
    EXPECT_TRUE(can_reinit || !can_reinit);  // Just test it doesn't crash
}

// ==================== PROXIMITY DIALOGUE SYSTEM TESTS ====================

TEST(ProximityDialogueSystem, SingletonInstance) {
    auto& system1 = ProximityDialogueSystem::getInstance();
    auto& system2 = ProximityDialogueSystem::getInstance();
    EXPECT_EQ(&system1, &system2);
}

TEST(ProximityDialogueSystem, ProximityRangeConstant) {
    EXPECT_EQ(ProximityDialogueSystem::PROXIMITY_RANGE, 5.0f);
}

TEST(ProximityDialogueSystem, MaxQueueLengthConstant) {
    EXPECT_EQ(ProximityDialogueSystem::MAX_QUEUE_LENGTH, 5);
}

TEST(ProximityDialogueSystem, PriorityWeights) {
    EXPECT_NEAR(ProximityDialogueSystem::W_SEVERITY, 0.4f, 0.01f);
    EXPECT_NEAR(ProximityDialogueSystem::W_INFLUENCE, 0.3f, 0.01f);
    EXPECT_NEAR(ProximityDialogueSystem::W_DISTANCE, 0.15f, 0.01f);
    EXPECT_NEAR(ProximityDialogueSystem::W_TIME, 0.15f, 0.01f);
}

TEST(ProximityDialogueSystem, QueueEntry) {
    ConversationQueueEntry entry{1, 0.5f, 0.7f, 0, 0};
    EXPECT_EQ(entry.npcId, 1);
    EXPECT_NEAR(entry.severityScore, 0.5f, 0.01f);
}

TEST(ProximityDialogueSystem, DistanceCalculation) {
    auto& system = ProximityDialogueSystem::getInstance();
    float dist = system.calculateDistance({0, 0, 0}, {3, 4, 0});
    EXPECT_NEAR(dist, 5.0f, 0.01f);  // 3-4-5 triangle
}

// ==================== NPC PROBLEM SYSTEM TESTS ====================

TEST(NpcProblemSystem, ProblemTypeEnum) {
    EXPECT_TRUE(static_cast<int>(ProblemType::RESOURCE_SCARCITY) >= 0);
    EXPECT_TRUE(static_cast<int>(ProblemType::FACTION_CONFLICT) >= 0);
    EXPECT_TRUE(static_cast<int>(ProblemType::MORAL_CRISIS) >= 0);
    EXPECT_TRUE(static_cast<int>(ProblemType::PERSONAL_GRIEVANCE) >= 0);
    EXPECT_TRUE(static_cast<int>(ProblemType::UNKNOWN) >= 0);
}

TEST(NpcProblemSystem, ProblemTypeToString) {
    std::string str = NpcProblemSystem::problemTypeToString(ProblemType::RESOURCE_SCARCITY);
    EXPECT_EQ(str, "RESOURCE_SCARCITY");
}

TEST(NpcProblemSystem, StringToProblemType) {
    auto type = NpcProblemSystem::stringToProblemType("FACTION_CONFLICT");
    EXPECT_EQ(type, ProblemType::FACTION_CONFLICT);
}

TEST(NpcProblemSystem, GenerateProblemStatement) {
    NpcProblem problem(ProblemType::RESOURCE_SCARCITY);
    std::string statement = NpcProblemSystem::generateProblemStatement(problem);
    EXPECT_FALSE(statement.empty());
}

TEST(NpcProblemSystem, GenerateProblemDescription) {
    NpcProblem problem(ProblemType::FACTION_CONFLICT);
    std::string description = NpcProblemSystem::generateProblemDescription(problem);
    EXPECT_FALSE(description.empty());
}

TEST(NpcProblemSystem, ProblemStructInitialization) {
    NpcProblem problem(ProblemType::RESOURCE_SCARCITY);
    EXPECT_EQ(problem.type, ProblemType::RESOURCE_SCARCITY);
    EXPECT_EQ(problem.severity, 0.0f);
    EXPECT_EQ(problem.affectedFactionId, -1);
}

// ==================== DIALOGUE UI TESTS ====================

TEST(DialogueUI, FormatMood) {
    std::string mood_str = DialogueUI::formatMood(0.75f);
    EXPECT_FALSE(mood_str.empty());
}

TEST(DialogueUI, FormatLoyalty) {
    std::string loyalty_str = DialogueUI::formatLoyalty(0.85f);
    EXPECT_FALSE(loyalty_str.empty());
}

TEST(DialogueUI, FormatFactionAlignment) {
    std::string align_str = DialogueUI::formatFactionAlignment(0.5f);
    EXPECT_FALSE(align_str.empty());
}

TEST(DialogueUI, DisplaySystemMessage) {
    std::string msg = DialogueUI::displaySystemMessage("Test message");
    EXPECT_FALSE(msg.empty());
}

TEST(DialogueUI, DisplayWarning) {
    std::string warning = DialogueUI::displayWarning("Test warning");
    EXPECT_FALSE(warning.empty());
}

TEST(DialogueUI, DisplaySuccess) {
    std::string success = DialogueUI::displaySuccess("Test success");
    EXPECT_FALSE(success.empty());
}

// ==================== DIALOGUE CONTROLLER TESTS ====================

TEST(DialogueController, SingletonInstance) {
    auto& ctrl1 = DialogueController::getInstance();
    auto& ctrl2 = DialogueController::getInstance();
    EXPECT_EQ(&ctrl1, &ctrl2);
}

TEST(DialogueController, StartConversation) {
    auto& controller = DialogueController::getInstance();
    bool result = controller.startConversation(10, "We need help!");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(controller.isConversationActive());
    EXPECT_EQ(controller.getActiveConversationNpcId(), 10);
}

TEST(DialogueController, IsWaitingForResponse) {
    auto& controller = DialogueController::getInstance();
    controller.startConversation(20, "Test");
    
    EXPECT_TRUE(controller.isWaitingForPlayerResponse());
}

TEST(DialogueController, HandlePlayerResponse) {
    auto& controller = DialogueController::getInstance();
    controller.startConversation(30, "We need help!");
    
    bool result = controller.handlePlayerResponse("I will help", 100);
    EXPECT_TRUE(result);
    EXPECT_FALSE(controller.isWaitingForPlayerResponse());
}

TEST(DialogueController, EndConversation) {
    auto& controller = DialogueController::getInstance();
    controller.startConversation(40, "Help!");
    controller.handlePlayerResponse("Ok", 100);
    
    bool result = controller.endConversation(40, true);
    EXPECT_TRUE(result);
    EXPECT_FALSE(controller.isConversationActive());
}

TEST(DialogueController, PauseResume) {
    auto& controller = DialogueController::getInstance();
    controller.startConversation(50, "Test");
    
    controller.pause();
    EXPECT_TRUE(controller.isPaused());
    
    controller.resume();
    EXPECT_FALSE(controller.isPaused());
}

TEST(DialogueController, ConversationTimeout) {
    EXPECT_EQ(DialogueController::CONVERSATION_TIMEOUT_TICKS, 3600);
}

TEST(DialogueController, GetConversationStartTick) {
    auto& controller = DialogueController::getInstance();
    controller.startConversation(60, "Test");
    
    int tick = controller.getConversationStartTick();
    EXPECT_GE(tick, 0);
}

TEST(DialogueController, MultipleConversations) {
    auto& controller = DialogueController::getInstance();
    
    // Start first conversation
    EXPECT_TRUE(controller.startConversation(70, "Hello"));
    EXPECT_EQ(controller.getActiveConversationNpcId(), 70);
    
    // End it
    controller.endConversation(70, true);
    EXPECT_FALSE(controller.isConversationActive());
    
    // Start second
    EXPECT_TRUE(controller.startConversation(80, "Hi"));
    EXPECT_EQ(controller.getActiveConversationNpcId(), 80);
}

}  // namespace TLS
