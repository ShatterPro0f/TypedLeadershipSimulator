/**
 * NarrativeGeneration.cpp
 * Implementation of narrative generation systems (Phase 8)
 * 
 * Converts world state changes into narrative issues via LLM or rule-based generation
 */

#include "NarrativeGeneration.h"
#include "Registries.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace TLS {

// ============================================================================
// NarrativeIssueQueue Implementation
// ============================================================================

NarrativeIssueQueue::NarrativeIssueQueue() : next_issue_id(0) {}

int NarrativeIssueQueue::addIssue(const NarrativeIssue& issue) {
    if (active_issues.size() >= MAX_ISSUES) {
        // Remove oldest low-priority issue
        auto it = std::find_if(active_issues.begin(), active_issues.end(),
                              [](const NarrativeIssue& i) { return i.priority == IssuePriority::OPPORTUNITY; });
        if (it != active_issues.end()) {
            active_issues.erase(it);
        }
    }
    
    NarrativeIssue new_issue = issue;
    new_issue.id = next_issue_id++;
    active_issues.push_back(new_issue);
    sortByPriority();
    return new_issue.id;
}

void NarrativeIssueQueue::updateActiveIssues(int current_tick) {
    // Remove expired issues
    active_issues.erase(
        std::remove_if(active_issues.begin(), active_issues.end(),
                      [current_tick](const NarrativeIssue& i) {
                          return !i.is_active || current_tick > i.expires_tick;
                      }),
        active_issues.end()
    );
}

std::vector<NarrativeIssue> NarrativeIssueQueue::getTopIssues(int count) {
    std::vector<NarrativeIssue> top;
    for (int i = 0; i < std::min(count, (int)active_issues.size()); ++i) {
        top.push_back(active_issues[i]);
    }
    return top;
}

const std::vector<NarrativeIssue>& NarrativeIssueQueue::getAllActiveIssues() const {
    return active_issues;
}

std::string NarrativeIssueQueue::formatIssuesForDisplay(int count) {
    std::stringstream ss;
    auto top = getTopIssues(count);
    
    if (top.empty()) {
        ss << "No active issues.\n";
        return ss.str();
    }
    
    ss << "Active Issues:\n";
    for (size_t i = 0; i < top.size(); ++i) {
        ss << "\n[" << (i + 1) << "] ";
        
        // Priority tag
        if (top[i].priority == IssuePriority::CRISIS) {
            ss << "⚠ CRISIS: ";
        } else if (top[i].priority == IssuePriority::WARNING) {
            ss << "! WARNING: ";
        } else {
            ss << "✓ OPPORTUNITY: ";
        }
        
        ss << top[i].title << "\n";
        ss << "    " << top[i].description << "\n";
        if (!top[i].suggestion.empty()) {
            ss << "    Suggestion: " << top[i].suggestion << "\n";
        }
    }
    
    return ss.str();
}

void NarrativeIssueQueue::clear() {
    active_issues.clear();
    next_issue_id = 0;
}

void NarrativeIssueQueue::resolveIssue(int issue_id) {
    auto it = std::find_if(active_issues.begin(), active_issues.end(),
                          [issue_id](const NarrativeIssue& i) { return i.id == issue_id; });
    if (it != active_issues.end()) {
        it->is_active = false;
    }
}

void NarrativeIssueQueue::sortByPriority() {
    std::sort(active_issues.begin(), active_issues.end(),
             [](const NarrativeIssue& a, const NarrativeIssue& b) {
                 return static_cast<int>(a.priority) < static_cast<int>(b.priority);
             });
}

// ============================================================================
// WorldStateMonitor Implementation
// ============================================================================

WorldStateMonitor::WorldStateMonitor() : last_update_tick(-1) {}

void WorldStateMonitor::updateWorldState(const NPCRegistry& npc_reg,
                                        const FactionRegistry& faction_reg,
                                        const ResourceRegistry& resource_reg,
                                        int current_tick) {
    npcs_with_mood_changes.clear();
    factions_with_loyalty_changes.clear();
    resources_with_scarcity_changes.clear();
    recent_events.clear();
    
    // Check NPC mood changes
    auto npcs = npc_reg.getAllNPCs();
    for (const auto& npc : npcs) {
        if (!npc) continue;
        
        float current_mood = npc->getShortTermMood();
        float current_loyalty = npc->getLongTermAttitude();
        
        auto it = previous_npc_states.find(npc->getId());
        if (it != previous_npc_states.end()) {
            float mood_delta = std::abs(current_mood - it->second.mood);
            if (mood_delta > MOOD_DELTA_THRESHOLD) {
                npcs_with_mood_changes.push_back(npc->getId());
            }
        }
        
        // Update state
        previous_npc_states[npc->getId()] = {
            npc->getId(),
            current_mood,
            current_loyalty
        };
    }
    
    // Check faction loyalty changes
    auto factions = faction_reg.getAllFactions();
    for (const auto& faction : factions) {
        if (!faction) continue;
        
        // Calculate average loyalty
        float total_loyalty = 0.0f;
        int member_count = faction->getMemberIds().size();
        if (member_count > 0) {
            for (int member_id : faction->getMemberIds()) {
                auto member = npc_reg.getNPCById(member_id);
                if (member) {
                    total_loyalty += member->getLongTermAttitude();
                }
            }
            float avg_loyalty = total_loyalty / member_count;
            
            auto it = previous_faction_states.find(faction->getId());
            if (it != previous_faction_states.end()) {
                float loyalty_delta = std::abs(avg_loyalty - it->second.average_loyalty);
                if (loyalty_delta > FACTION_LOYALTY_THRESHOLD) {
                    factions_with_loyalty_changes.push_back(faction->getId());
                }
            }
            
            previous_faction_states[faction->getId()] = {
                faction->getId(),
                avg_loyalty,
                faction->getStrength()
            };
        }
    }
    
    // Check resource scarcity changes
    auto resources = resource_reg.getAllResources();
    for (const auto& resource : resources) {
        if (!resource) continue;
        
        bool is_below_scarcity = resource->getQuantity() < resource->getScarcityThreshold();
        
        auto it = previous_resource_states.find(resource->getId());
        if (it != previous_resource_states.end()) {
            if (is_below_scarcity != it->second.is_below_scarcity) {
                resources_with_scarcity_changes.push_back(resource->getId());
            }
        }
        
        previous_resource_states[resource->getId()] = {
            resource->getId(),
            resource->getQuantity(),
            is_below_scarcity
        };
    }
    
    last_update_tick = current_tick;
}

bool WorldStateMonitor::hasSignificantWorldStateChange() const {
    return !npcs_with_mood_changes.empty() ||
           !factions_with_loyalty_changes.empty() ||
           !resources_with_scarcity_changes.empty();
}

std::vector<int> WorldStateMonitor::getNPCsWithMoodChanges() const {
    return npcs_with_mood_changes;
}

std::vector<int> WorldStateMonitor::getFactionsWithLoyaltyChanges() const {
    return factions_with_loyalty_changes;
}

std::vector<int> WorldStateMonitor::getResourcesWithScarcityChanges() const {
    return resources_with_scarcity_changes;
}

void WorldStateMonitor::clearPreviousState() {
    previous_npc_states.clear();
    previous_faction_states.clear();
    previous_resource_states.clear();
}

// ============================================================================
// WorldStateSnapshotBuilder Implementation
// ============================================================================

WorldStateSnapshot WorldStateSnapshotBuilder::buildSnapshot(const WorldStateMonitor& monitor,
                                                           const NPCRegistry& npc_reg,
                                                           const FactionRegistry& faction_reg,
                                                           const ResourceRegistry& resource_reg,
                                                           int current_tick) {
    WorldStateSnapshot snapshot;
    snapshot.tickNumber = current_tick;
    
    // Add NPCs with mood changes (most influential first)
    std::vector<int> npc_ids_with_changes = monitor.getNPCsWithMoodChanges();
    std::vector<int> pruned_npcs = pruneNPCsToMostInfluential(npc_ids_with_changes, npc_reg, faction_reg);
    
    // Store IDs, not full objects
    snapshot.significantNPCIds = pruned_npcs;
    
    // Add factions with loyalty changes
    snapshot.affectedFactionIds = monitor.getFactionsWithLoyaltyChanges();
    
    // Add resources with scarcity changes
    snapshot.changedResourceIds = monitor.getResourcesWithScarcityChanges();
    
    return snapshot;
}

float WorldStateSnapshotBuilder::calculateNPCInfluenceScore(const NPC& npc, 
                                                           const FactionRegistry& faction_reg) {
    float score = 0.0f;
    
    // Factor 1: NPC loyalty (reflects player favor)
    score += npc.getLongTermAttitude() * 0.4f;
    
    // Factor 2: Faction strength (if NPC is in a faction)
    auto faction = faction_reg.getFactionById(npc.getFactionId());
    if (faction) {
        score += faction->getStrength() * 0.3f;
    }
    
    // Factor 3: Advisor influence (if NPC is an advisor)
    if (npc.getRole() == "advisor") {
        score += 0.3f;  // Advisors get boost
    }
    
    // Factor 4: Problem severity (if NPC has active problem)
    score += npc.getProblemSeverity() * 0.2f;
    
    return std::min(score, 1.0f);
}

std::vector<int> WorldStateSnapshotBuilder::pruneNPCsToMostInfluential(const std::vector<int>& npc_ids,
                                                                       const NPCRegistry& npc_reg,
                                                                       const FactionRegistry& faction_reg) {
    if (npc_ids.size() <= MAX_NPCS_IN_SNAPSHOT) {
        return npc_ids;
    }
    
    // Calculate influence score for each NPC
    std::vector<std::pair<int, float>> scored_npcs;
    for (int npc_id : npc_ids) {
        auto npc = npc_reg.getNPCById(npc_id);
        if (npc) {
            float score = calculateNPCInfluenceScore(*npc, faction_reg);
            scored_npcs.push_back({npc_id, score});
        }
    }
    
    // Sort by score (highest first)
    std::sort(scored_npcs.begin(), scored_npcs.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Keep only top MAX_NPCS_IN_SNAPSHOT
    std::vector<int> result;
    for (size_t i = 0; i < std::min((size_t)MAX_NPCS_IN_SNAPSHOT, scored_npcs.size()); ++i) {
        result.push_back(scored_npcs[i].first);
    }
    
    return result;
}

// ============================================================================
// LLMPromptBuilder Implementation
// ============================================================================

std::string LLMPromptBuilder::getSystemPrompt() {
    return R"(You are a narrative designer for a complex leadership simulation game.
Your role is to analyze the current world state and generate compelling narrative 
issues and opportunities for the player to address.

Generate 2-3 narrative issues based on the provided world state changes.
For each issue, provide:
1. Title (brief, 5-10 words)
2. Description (2-3 sentences explaining the situation)
3. Suggestion (optional - a brief suggestion for player action)
4. Priority (CRISIS, WARNING, or OPPORTUNITY)

Format your response as JSON with this structure:
[
  {
    "title": "Issue Title",
    "description": "Full description",
    "suggestion": "Optional suggestion",
    "priority": "CRISIS|WARNING|OPPORTUNITY",
    "type": "FACTION_CONFLICT|RESOURCE_SCARCITY|NPC_CRISIS|etc"
  }
]

Be specific about the situation, reference NPC names and faction names when available,
and make the issues feel emergent from the world state, not generic.
)";
}

std::string LLMPromptBuilder::buildNarrativePrompt(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    
    ss << "Current Tick: " << snapshot.tickNumber << "\n\n";
    
    if (!snapshot.significantNPCIds.empty()) {
        ss << formatNPCChanges(snapshot);
    }
    
    if (!snapshot.affectedFactionIds.empty()) {
        ss << formatFactionChanges(snapshot);
    }
    
    if (!snapshot.changedResourceIds.empty()) {
        ss << formatResourceChanges(snapshot);
    }
    
    if (!snapshot.triggeredEventIds.empty()) {
        ss << formatTriggeredEvents(snapshot);
    }
    
    ss << "\nBased on these changes, what are the most significant narrative issues? ";
    ss << "Consider consequences, cascades, and opportunities.\n";
    
    return ss.str();
}

std::string LLMPromptBuilder::formatNPCChanges(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    ss << "NPC Changes:\n";
    
    for (int npcId : snapshot.significantNPCIds) {
        auto npc = NPCRegistry::getInstance().getNPCById(npcId);
        if (!npc) continue;
        
        ss << "- " << npc->getName() << " (" << npc->getRole() << "): ";
        ss << "Mood=" << npc->getShortTermMood() << ", ";
        ss << "Loyalty=" << npc->getLongTermAttitude() << "\n";
    }
    
    ss << "\n";
    return ss.str();
}

std::string LLMPromptBuilder::formatFactionChanges(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    ss << "Faction Changes:\n";
    
    for (int factionId : snapshot.affectedFactionIds) {
        auto faction = FactionRegistry::getInstance().getFactionById(factionId);
        if (!faction) continue;
        
        ss << "- " << faction->getName() << ": ";
        ss << "Strength=" << faction->getStrength() << ", ";
        ss << "Members=" << faction->getMemberIds().size() << "\n";
    }
    
    ss << "\n";
    return ss.str();
}

std::string LLMPromptBuilder::formatResourceChanges(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    ss << "Resource Changes:\n";
    
    for (int resourceId : snapshot.changedResourceIds) {
        auto resource = ResourceRegistry::getInstance().getResourceById(resourceId);
        if (!resource) continue;
        
        ss << "- " << resource->getName() << ": ";
        ss << "Quantity=" << resource->getQuantity() << " ";
        ss << "(Scarcity Threshold: " << resource->getScarcityThreshold() << ")\n";
    }
    
    ss << "\n";
    return ss.str();
}

std::string LLMPromptBuilder::formatTriggeredEvents(const WorldStateSnapshot& snapshot) {
    std::stringstream ss;
    ss << "Recent Events:\n";
    
    for (int eventId : snapshot.triggeredEventIds) {
        auto event = EventRegistry::getInstance().getEventById(eventId);
        if (!event) continue;
        
        ss << "- " << event->getName() << " (Impact: " << event->getImpactLevel() << ")\n";
    }
    
    ss << "\n";
    return ss.str();
}

// ============================================================================
// NarrativeGenerator Implementation
// ============================================================================

NarrativeGenerator::NarrativeGenerator(std::shared_ptr<LLMProvider> llm)
    : llm_provider(llm) {}

std::vector<NarrativeIssue> NarrativeGenerator::generateNarratives(const WorldStateSnapshot& snapshot,
                                                                  int current_tick,
                                                                  bool force_rule_based) {
    if (force_rule_based || !llm_provider || !llm_provider->isAvailable()) {
        return generateFromRules(snapshot, current_tick);
    }
    
    return generateFromLLM(snapshot, current_tick);
}

std::vector<NarrativeIssue> NarrativeGenerator::generateFromLLM(const WorldStateSnapshot& snapshot,
                                                               int current_tick) {
    LLMPromptBuilder builder;
    std::string prompt = builder.buildNarrativePrompt(snapshot);
    
    // Call LLM (temperature handling is internal to provider)
    LLMResponse response = llm_provider->callLLM(prompt);
    
    if (!response.success) {
        // Fallback to rule-based if LLM fails
        return generateFromRules(snapshot, current_tick);
    }
    
    return parseLLMResponse(response.content, current_tick);
}

std::vector<NarrativeIssue> NarrativeGenerator::generateFromRules(const WorldStateSnapshot& snapshot,
                                                                 int current_tick) {
    std::vector<NarrativeIssue> issues;
    
    // Rule 1: NPC mood changes generate issues
    for (int npcId : snapshot.significantNPCIds) {
        auto npc = NPCRegistry::getInstance().getNPCById(npcId);
        if (!npc) continue;
        
        float mood = npc->getShortTermMood();
        
        if (mood < 0.3f) {
            // Unhappy/anxious NPC
            NarrativeIssue issue(
                issues.size(),
                npc->getName() + " is troubled",
                npc->getName() + " seems distressed. Their mood has declined significantly.",
                "Consider speaking with " + npc->getName() + " to understand their concerns.",
                IssuePriority::WARNING,
                IssueType::NPC_CRISIS,
                current_tick
            );
            issues.push_back(issue);
        } else if (mood > 0.8f) {
            // Very happy NPC - opportunity
            NarrativeIssue issue(
                issues.size(),
                npc->getName() + " is inspired",
                npc->getName() + " is in high spirits and may be receptive to new tasks.",
                "Assign " + npc->getName() + " to an important project while morale is high.",
                IssuePriority::OPPORTUNITY,
                IssueType::OPPORTUNITY,
                current_tick
            );
            issues.push_back(issue);
        }
    }
    
    // Rule 2: Faction changes
    for (int factionId : snapshot.affectedFactionIds) {
        auto faction = FactionRegistry::getInstance().getFactionById(factionId);
        if (!faction) continue;
        
        if (faction->getStrength() < 0.4f) {
            NarrativeIssue issue(
                issues.size(),
                faction->getName() + " faction weakening",
                "The " + faction->getName() + " faction's influence is declining.",
                "Strengthen relations with " + faction->getName() + " to stabilize their power.",
                IssuePriority::WARNING,
                IssueType::FACTION_CONFLICT,
                current_tick
            );
            issues.push_back(issue);
        }
    }
    
    // Rule 3: Resource scarcity
    for (int resourceId : snapshot.changedResourceIds) {
        auto resource = ResourceRegistry::getInstance().getResourceById(resourceId);
        if (!resource) continue;
        
        if (resource->getQuantity() < resource->getScarcityThreshold()) {
            NarrativeIssue issue(
                issues.size(),
                resource->getName() + " scarcity",
                resource->getName() + " supplies are running low. This may affect settlement morale.",
                "Prioritize acquiring more " + resource->getName() + " or reduce consumption.",
                IssuePriority::CRISIS,
                IssueType::RESOURCE_SCARCITY,
                current_tick
            );
            issues.push_back(issue);
        }
    }
    
    return issues;
}

std::vector<NarrativeIssue> NarrativeGenerator::parseLLMResponse(const std::string& response,
                                                                int current_tick) {
    std::vector<NarrativeIssue> issues;
    
    // Simple JSON parsing (in production, use a proper JSON library)
    // For now, return empty and let rule-based fallback kick in
    // TODO: Implement proper JSON parsing from LLM response
    
    return issues;
}

NarrativeIssue NarrativeGenerator::createNPCMoodIssue(const NPC& npc, float mood_delta, 
                                                     int current_tick) {
    std::string title;
    std::string description;
    IssuePriority priority;
    
    if (mood_delta < -0.3f) {
        title = npc.getName() + " is distraught";
        description = npc.getName() + " mood has plummeted. Immediate intervention may be needed.";
        priority = IssuePriority::CRISIS;
    } else if (mood_delta < 0.0f) {
        title = npc.getName() + " mood declining";
        description = npc.getName() + "'s mood is declining slowly. Monitor for problems.";
        priority = IssuePriority::WARNING;
    } else {
        title = npc.getName() + " is pleased";
        description = npc.getName() + " is in good spirits. Now is a good time to ask favors.";
        priority = IssuePriority::OPPORTUNITY;
    }
    
    return NarrativeIssue(
        -1, title, description, "", priority, IssueType::NPC_CRISIS, current_tick
    );
}

NarrativeIssue NarrativeGenerator::createFactionTensionIssue(const Faction& faction, 
                                                            int current_tick) {
    return NarrativeIssue(
        -1,
        faction.getName() + " faction tensions rise",
        "Internal conflicts within the " + faction.getName() + " faction are escalating.",
        "Mediate disputes or provide resources to stabilize the faction.",
        IssuePriority::WARNING,
        IssueType::FACTION_CONFLICT,
        current_tick
    );
}

NarrativeIssue NarrativeGenerator::createResourceScarcityIssue(const Resource& resource,
                                                              int current_tick) {
    return NarrativeIssue(
        -1,
        resource.getName() + " running low",
        resource.getName() + " supplies are critically low. Settlement functionality may suffer.",
        "Immediately focus on production of " + resource.getName() + " or trade for imports.",
        IssuePriority::CRISIS,
        IssueType::RESOURCE_SCARCITY,
        current_tick
    );
}

// ============================================================================
// NarrativeIssueDisplay Implementation
// ============================================================================

std::string NarrativeIssueDisplay::formatIssueForDisplay(const NarrativeIssue& issue) {
    std::stringstream ss;
    
    ss << getPriorityTag(issue.priority) << " " << issue.title << "\n";
    ss << "  " << issue.description << "\n";
    
    if (!issue.suggestion.empty()) {
        ss << "  → " << issue.suggestion << "\n";
    }
    
    return ss.str();
}

std::string NarrativeIssueDisplay::formatMultipleIssues(const std::vector<NarrativeIssue>& issues,
                                                       int max_display) {
    std::stringstream ss;
    
    ss << "\n=== ACTIVE ISSUES ===\n";
    
    int count = 0;
    for (const auto& issue : issues) {
        if (count >= max_display) break;
        ss << "\n" << formatIssueForDisplay(issue);
        count++;
    }
    
    if (issues.size() > (size_t)max_display) {
        ss << "\n... and " << (issues.size() - max_display) << " more issues.\n";
    }
    
    return ss.str();
}

std::string NarrativeIssueDisplay::formatIssueSummary(const NarrativeIssue& issue) {
    return getPriorityTag(issue.priority) + " " + issue.title;
}

std::string NarrativeIssueDisplay::getPriorityTag(IssuePriority priority) {
    switch (priority) {
        case IssuePriority::CRISIS:
            return "[CRISIS]";
        case IssuePriority::WARNING:
            return "[WARNING]";
        case IssuePriority::OPPORTUNITY:
            return "[OPPORTUNITY]";
        default:
            return "[ISSUE]";
    }
}

std::string NarrativeIssueDisplay::getTypeTag(IssueType type) {
    switch (type) {
        case IssueType::FACTION_CONFLICT:
            return "FACTION";
        case IssueType::RESOURCE_SCARCITY:
            return "RESOURCE";
        case IssueType::NPC_CRISIS:
            return "NPC";
        case IssueType::CULTURAL_SHIFT:
            return "CULTURE";
        case IssueType::RELIGIOUS_CONFLICT:
            return "RELIGION";
        case IssueType::ENVIRONMENTAL:
            return "ENVIRONMENT";
        case IssueType::OPPORTUNITY:
            return "OPPORTUNITY";
        case IssueType::IMMIGRATION:
            return "IMMIGRATION";
        case IssueType::FACTION_REBELLION:
            return "REBELLION";
        default:
            return "EVENT";
    }
}

}  // namespace TLS
