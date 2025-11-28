#pragma once

#include "Registries.h"
#include "LLM.h"
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <unordered_map>

namespace TLS {

/**
 * @enum IssuePriority
 * Defines priority levels for narrative issues
 */
enum class IssuePriority : uint8_t {
    CRISIS = 0,           // Immediate threat (rebellion, starvation, etc.)
    WARNING = 1,          // Significant concern (faction tension, mood decline)
    OPPORTUNITY = 2       // Positive development (immigration, discovery, etc.)
};

/**
 * @enum IssueType
 * Categorizes narrative issues by type
 */
enum class IssueType : uint8_t {
    FACTION_CONFLICT = 0,
    RESOURCE_SCARCITY = 1,
    NPC_CRISIS = 2,
    CULTURAL_SHIFT = 3,
    RELIGIOUS_CONFLICT = 4,
    ENVIRONMENTAL = 5,
    OPPORTUNITY = 6,
    IMMIGRATION = 7,
    FACTION_REBELLION = 8
};

/**
 * @struct NarrativeIssue
 * Represents a single narrative crisis or opportunity for the player to address
 */
struct NarrativeIssue {
    int id;
    std::string title;
    std::string description;
    std::string suggestion;
    IssuePriority priority;
    IssueType type;
    std::vector<int> affected_npc_ids;
    std::vector<int> affected_faction_ids;
    std::vector<int> affected_resource_ids;
    int created_tick;
    int expires_tick;  // When issue becomes irrelevant
    bool is_active;
    
    NarrativeIssue() 
        : id(-1), priority(IssuePriority::WARNING), type(IssueType::NPC_CRISIS),
          created_tick(0), expires_tick(0), is_active(true) {}
    
    NarrativeIssue(int id, const std::string& title, const std::string& description,
                   const std::string& suggestion, IssuePriority pri, IssueType typ,
                   int tick)
        : id(id), title(title), description(description), suggestion(suggestion),
          priority(pri), type(typ), created_tick(tick), 
          expires_tick(tick + 1440),  // ~1 game day default
          is_active(true) {}
};

/**
 * @class NarrativeIssueQueue
 * Manages circular buffer of active narrative issues with priority sorting
 */
class NarrativeIssueQueue {
public:
    static constexpr int MAX_ISSUES = 20;
    
    NarrativeIssueQueue();
    
    // Add issue to queue, returns issue ID
    int addIssue(const NarrativeIssue& issue);
    
    // Remove expired issues and update activeness
    void updateActiveIssues(int current_tick);
    
    // Get top N issues by priority
    std::vector<NarrativeIssue> getTopIssues(int count = 3);
    
    // Get all active issues
    const std::vector<NarrativeIssue>& getAllActiveIssues() const;
    
    // Format issues for display
    std::string formatIssuesForDisplay(int count = 3);
    
    // Clear all issues
    void clear();
    
    // Get issue count
    int getIssueCount() const { return active_issues.size(); }
    
    // Resolve specific issue
    void resolveIssue(int issue_id);

private:
    std::vector<NarrativeIssue> active_issues;
    int next_issue_id;
    
    // Sort issues by priority (crisis first)
    void sortByPriority();
};

/**
 * @class WorldStateMonitor
 * Tracks world state changes and detects significant deltas
 * Implements thresholds from copilot-instructions.md Section 12a2
 */
class WorldStateMonitor {
public:
    // Threshold constants from spec
    static constexpr float MOOD_DELTA_THRESHOLD = 0.2f;
    static constexpr float FACTION_LOYALTY_THRESHOLD = 0.15f;
    static constexpr int SCARCITY_THRESHOLD_DEFAULT = 50;
    
    WorldStateMonitor();
    
    // Update monitor with current world state
    void updateWorldState(const NPCRegistry& npc_reg,
                         const FactionRegistry& faction_reg,
                         const ResourceRegistry& resource_reg,
                         int current_tick);
    
    // Check if world state has significantly changed
    bool hasSignificantWorldStateChange() const;
    
    // Get list of NPCs with significant mood changes
    std::vector<int> getNPCsWithMoodChanges() const;
    
    // Get list of factions with loyalty changes
    std::vector<int> getFactionsWithLoyaltyChanges() const;
    
    // Get resources that crossed scarcity threshold
    std::vector<int> getResourcesWithScarcityChanges() const;
    
    // Get recent triggered events
    const std::vector<Event>& getRecentEvents() const { return recent_events; }
    
    // Clear previous state tracking
    void clearPreviousState();

private:
    struct NPCSnapshot {
        int id;
        float mood;
        float loyalty;
    };
    
    struct FactionSnapshot {
        int id;
        float average_loyalty;
        float strength;
    };
    
    struct ResourceSnapshot {
        int id;
        int quantity;
        bool is_below_scarcity;
    };
    
    std::unordered_map<int, NPCSnapshot> previous_npc_states;
    std::unordered_map<int, FactionSnapshot> previous_faction_states;
    std::unordered_map<int, ResourceSnapshot> previous_resource_states;
    
    std::vector<int> npcs_with_mood_changes;
    std::vector<int> factions_with_loyalty_changes;
    std::vector<int> resources_with_scarcity_changes;
    std::vector<Event> recent_events;
    
    int last_update_tick;
};

/**
 * @class WorldStateSnapshotBuilder
 * Builds lightweight snapshots from full world state for LLM processing
 */
class WorldStateSnapshotBuilder {
public:
    static constexpr int MAX_NPCS_IN_SNAPSHOT = 50;
    
    // Build snapshot with only significant changes
    WorldStateSnapshot buildSnapshot(const WorldStateMonitor& monitor,
                                    const NPCRegistry& npc_reg,
                                    const FactionRegistry& faction_reg,
                                    const ResourceRegistry& resource_reg,
                                    int current_tick);

private:
    // Calculate influence score for NPC relevance ranking
    float calculateNPCInfluenceScore(const NPC& npc, const FactionRegistry& faction_reg);
    
    // Prune NPCs to most influential if too many
    std::vector<int> pruneNPCsToMostInfluential(const std::vector<int>& npc_ids,
                                               const NPCRegistry& npc_reg,
                                               const FactionRegistry& faction_reg);
};

/**
 * @class LLMPromptBuilder
 * Converts world state snapshots into prompts for narrative generation
 */
class LLMPromptBuilder {
public:
    // Build prompt for narrative generation
    std::string buildNarrativePrompt(const WorldStateSnapshot& snapshot);
    
    // Build system prompt for LLM narrative generation
    static std::string getSystemPrompt();

private:
    std::string formatNPCChanges(const WorldStateSnapshot& snapshot);
    std::string formatFactionChanges(const WorldStateSnapshot& snapshot);
    std::string formatResourceChanges(const WorldStateSnapshot& snapshot);
    std::string formatTriggeredEvents(const WorldStateSnapshot& snapshot);
};

/**
 * @class NarrativeGenerator
 * Generates narrative issues from LLM or rule-based system
 */
class NarrativeGenerator {
public:
    NarrativeGenerator(std::shared_ptr<LLMProvider> llm = nullptr);
    
    // Generate narrative issues from world state snapshot
    // Calls LLM if available, falls back to rule-based
    std::vector<NarrativeIssue> generateNarratives(const WorldStateSnapshot& snapshot,
                                                   int current_tick,
                                                   bool force_rule_based = false);

private:
    std::shared_ptr<LLMProvider> llm_provider;
    
    // Generate narratives using LLM
    std::vector<NarrativeIssue> generateFromLLM(const WorldStateSnapshot& snapshot,
                                               int current_tick);
    
    // Generate narratives using rule-based templates
    std::vector<NarrativeIssue> generateFromRules(const WorldStateSnapshot& snapshot,
                                                 int current_tick);
    
    // Parse LLM response into NarrativeIssue objects
    std::vector<NarrativeIssue> parseLLMResponse(const std::string& response,
                                                int current_tick);
    
    // Create rule-based issue from NPC mood change
    NarrativeIssue createNPCMoodIssue(const NPC& npc, float mood_delta, int current_tick);
    
    // Create rule-based issue from faction changes
    NarrativeIssue createFactionTensionIssue(const Faction& faction, int current_tick);
    
    // Create rule-based issue from resource scarcity
    NarrativeIssue createResourceScarcityIssue(const Resource& resource, int current_tick);
};

/**
 * @class NarrativeIssueDisplay
 * Formats narrative issues for text-based UI display
 */
class NarrativeIssueDisplay {
public:
    // Format single issue for display
    static std::string formatIssueForDisplay(const NarrativeIssue& issue);
    
    // Format multiple issues with hierarchy
    static std::string formatMultipleIssues(const std::vector<NarrativeIssue>& issues,
                                           int max_display = 3);
    
    // Format brief issue summary (one-liner)
    static std::string formatIssueSummary(const NarrativeIssue& issue);

private:
    static std::string getPriorityTag(IssuePriority priority);
    static std::string getTypeTag(IssueType type);
};

}  // namespace TLS
