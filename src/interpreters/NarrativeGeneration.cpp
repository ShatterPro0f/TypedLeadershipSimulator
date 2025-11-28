#include "NarrativeGeneration.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace TLS {

// ============================================================================
// NarrativeIssueQueue Implementation
// ============================================================================

NarrativeIssueQueue::NarrativeIssueQueue() : next_issue_id(1) {}

int NarrativeIssueQueue::addIssue(const NarrativeIssue& issue) {
    NarrativeIssue new_issue = issue;
    new_issue.id = next_issue_id++;
    active_issues.push_back(new_issue);
    
    // Keep only most recent issues if exceeding capacity
    if (active_issues.size() > MAX_ISSUES) {
        active_issues.erase(active_issues.begin());
    }
    
    sortByPriority();
    return new_issue.id;
}

void NarrativeIssueQueue::updateActiveIssues(int current_tick) {
    // Remove expired issues
    active_issues.erase(
        std::remove_if(active_issues.begin(), active_issues.end(),
                      [current_tick](const NarrativeIssue& issue) {
                          return current_tick >= issue.expires_tick;
                      }),
        active_issues.end()
    );
    
    sortByPriority();
}

std::vector<NarrativeIssue> NarrativeIssueQueue::getTopIssues(int count) {
    updateActiveIssues(0);  // Update expiry, passing 0 to preserve issues
    std::vector<NarrativeIssue> result;
    for (int i = 0; i < std::min(count, static_cast<int>(active_issues.size())); ++i) {
        result.push_back(active_issues[i]);
    }
    return result;
}

const std::vector<NarrativeIssue>& NarrativeIssueQueue::getAllActiveIssues() const {
    return active_issues;
}

std::string NarrativeIssueQueue::formatIssuesForDisplay(int count) {
    std::ostringstream ss;
    auto top_issues = getTopIssues(count);
    
    if (top_issues.empty()) {
        return "[No active issues]";
    }
    
    for (const auto& issue : top_issues) {
        ss << NarrativeIssueDisplay::formatIssueForDisplay(issue) << "\n";
    }
    
    return ss.str();
}

void NarrativeIssueQueue::clear() {
    active_issues.clear();
    next_issue_id = 1;
}

void NarrativeIssueQueue::resolveIssue(int issue_id) {
    active_issues.erase(
        std::remove_if(active_issues.begin(), active_issues.end(),
                      [issue_id](const NarrativeIssue& issue) {
                          return issue.id == issue_id;
                      }),
        active_issues.end()
    );
}

void NarrativeIssueQueue::sortByPriority() {
    std::sort(active_issues.begin(), active_issues.end(),
             [](const NarrativeIssue& a, const NarrativeIssue& b) {
                 // Lower enum value = higher priority
                 if (a.priority != b.priority) {
                     return a.priority < b.priority;
                 }
                 // If same priority, newer issues first
                 return a.created_tick > b.created_tick;
             });
}

// ============================================================================
// WorldStateMonitor Implementation
// ============================================================================

WorldStateMonitor::WorldStateMonitor() : last_update_tick(0) {}

void WorldStateMonitor::updateWorldState(const NPCRegistry& npc_reg,
                                         const FactionRegistry& faction_reg,
                                         const ResourceRegistry& resource_reg,
                                         int current_tick) {
    npcs_with_mood_changes.clear();
    factions_with_loyalty_changes.clear();
    resources_with_scarcity_changes.clear();
    recent_events.clear();
    
    // Check NPC mood changes
    const auto& all_npcs = npc_reg.getAllNPCs();
    for (const auto& npc : all_npcs) {
        if (!npc) continue;
        auto it = previous_npc_states.find(npc->getId());
        if (it != previous_npc_states.end()) {
            float mood_delta = std::abs(npc->getShortTermMood() - it->second.mood);
            if (mood_delta > MOOD_DELTA_THRESHOLD) {
                npcs_with_mood_changes.push_back(npc->getId());
            }
        }
        // Update current state
        NPCSnapshot snapshot;
        snapshot.id = npc->getId();
        snapshot.mood = npc->getShortTermMood();
        snapshot.loyalty = npc->getLoyalty();
        previous_npc_states[npc->getId()] = snapshot;
    }
    
    // Check faction loyalty changes
    const auto& all_factions = faction_reg.getAllFactions();
    for (const auto& faction : all_factions) {
        if (!faction) continue;
        // Use faction strength as proxy for average loyalty
        float faction_strength = faction->getStrength();
        auto it = previous_faction_states.find(faction->getId());
        if (it != previous_faction_states.end()) {
            float loyalty_delta = std::abs(faction_strength - it->second.average_loyalty);
            if (loyalty_delta > FACTION_LOYALTY_THRESHOLD) {
                factions_with_loyalty_changes.push_back(faction->getId());
            }
        }
        // Update current state
        FactionSnapshot snapshot;
        snapshot.id = faction->getId();
        snapshot.average_loyalty = faction_strength;
        snapshot.strength = faction->getStrength();
        previous_faction_states[faction->getId()] = snapshot;
    }
    
    // Check resource scarcity changes
    const auto& all_resources = resource_reg.getAllResources();
    for (const auto& resource : all_resources) {
        if (!resource) continue;
        auto it = previous_resource_states.find(resource->getId());
        // Assume scarcity threshold of 50
        bool is_currently_scarce = resource->getQuantity() < 50;
        
        if (it != previous_resource_states.end()) {
            bool was_scarce = it->second.is_below_scarcity;
            if (is_currently_scarce != was_scarce) {
                resources_with_scarcity_changes.push_back(resource->getId());
            }
        }
        // Update current state
        ResourceSnapshot snapshot;
        snapshot.id = resource->getId();
        snapshot.quantity = resource->getQuantity();
        snapshot.is_below_scarcity = is_currently_scarce;
        previous_resource_states[resource->getId()] = snapshot;
    }
    
    last_update_tick = current_tick;
}

bool WorldStateMonitor::hasSignificantWorldStateChange() const {
    return !npcs_with_mood_changes.empty() ||
           !factions_with_loyalty_changes.empty() ||
           !resources_with_scarcity_changes.empty() ||
           !recent_events.empty();
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
    snapshot.tick_number = current_tick;
    
    // Get NPCs with mood changes
    auto npc_ids = monitor.getNPCsWithMoodChanges();
    if (npc_ids.size() > MAX_NPCS_IN_SNAPSHOT) {
        npc_ids = pruneNPCsToMostInfluential(npc_ids, npc_reg, faction_reg);
    }
    
    for (int npc_id : npc_ids) {
        auto npc = npc_reg.getNPCById(npc_id);
        if (npc) {
            snapshot.significant_npcs.push_back(*npc);
        }
    }
    
    // Get affected factions
    auto faction_ids = monitor.getFactionsWithLoyaltyChanges();
    for (int faction_id : faction_ids) {
        auto faction = faction_reg.getFactionById(faction_id);
        if (faction) {
            snapshot.affected_factions.push_back(*faction);
        }
    }
    
    // Get resources with scarcity changes
    auto resource_ids = monitor.getResourcesWithScarcityChanges();
    for (int resource_id : resource_ids) {
        auto resource = resource_reg.getResourceById(resource_id);
        if (resource) {
            snapshot.changed_resources.push_back(*resource);
        }
    }
    
    return snapshot;
}

float WorldStateSnapshotBuilder::calculateNPCInfluenceScore(const NPC& npc,
                                                           const FactionRegistry& faction_reg) {
    // Score based on loyalty and faction power
    auto faction = faction_reg.getFactionById(npc.getFactionId());
    float faction_power = faction ? faction->getStrength() : 0.5f;
    return npc.getLoyalty() * 0.6f + faction_power * 0.4f;
}

std::vector<int> WorldStateSnapshotBuilder::pruneNPCsToMostInfluential(
    const std::vector<int>& npc_ids,
    const NPCRegistry& npc_reg,
    const FactionRegistry& faction_reg) {
    
    // Sort NPCs by influence score
    std::vector<std::pair<float, int>> scored_ids;
    for (int npc_id : npc_ids) {
        auto npc = npc_reg.getNPCById(npc_id);
        if (npc) {
            float score = calculateNPCInfluenceScore(*npc, faction_reg);
            scored_ids.push_back({score, npc_id});
        }
    }
    
    // Sort descending by score
    std::sort(scored_ids.begin(), scored_ids.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top MAX_NPCS_IN_SNAPSHOT
    std::vector<int> result;
    for (int i = 0; i < std::min(static_cast<int>(scored_ids.size()), MAX_NPCS_IN_SNAPSHOT); ++i) {
        result.push_back(scored_ids[i].second);
    }
    return result;
}

// ============================================================================
// LLMPromptBuilder Implementation
// ============================================================================

std::string LLMPromptBuilder::buildNarrativePrompt(const WorldStateSnapshot& snapshot) {
    std::ostringstream ss;
    
    ss << "The settlement has undergone significant changes:\n\n";
    
    if (!snapshot.significant_npcs.empty()) {
        ss << "NPCs Status:\n" << formatNPCChanges(snapshot) << "\n";
    }
    
    if (!snapshot.affected_factions.empty()) {
        ss << "Factions:\n" << formatFactionChanges(snapshot) << "\n";
    }
    
    if (!snapshot.changed_resources.empty()) {
        ss << "Resources:\n" << formatResourceChanges(snapshot) << "\n";
    }
    
    if (!snapshot.triggered_events.empty()) {
        ss << "Events:\n" << formatTriggeredEvents(snapshot) << "\n";
    }
    
    ss << "\nBased on these changes, generate 3-5 specific narrative issues or crises ";
    ss << "that the player should address. For each issue, provide:\n";
    ss << "1. A clear title\n";
    ss << "2. A description of the situation\n";
    ss << "3. A suggested action\n";
    ss << "4. Priority level (CRISIS, WARNING, or OPPORTUNITY)\n\n";
    ss << "Format each issue as: ISSUE: {title} | {description} | {suggestion} | {priority}";
    
    return ss.str();
}

std::string LLMPromptBuilder::getSystemPrompt() {
    return "You are the narrative engine for a leadership simulation game. Your role is to "
           "generate compelling, realistic narrative crises and opportunities based on world state. "
           "Issues should be specific, actionable, and tied to NPCs, factions, or resources. "
           "Keep descriptions concise but evocative.";
}

std::string LLMPromptBuilder::formatNPCChanges(const WorldStateSnapshot& snapshot) {
    std::ostringstream ss;
    for (const auto& npc : snapshot.significant_npcs) {
        ss << "- " << npc.getName() << " (" << npc.getRole() << "): "
           << "Mood=" << npc.getShortTermMood() << ", "
           << "Loyalty=" << npc.getLoyalty() << "\n";
    }
    return ss.str();
}

std::string LLMPromptBuilder::formatFactionChanges(const WorldStateSnapshot& snapshot) {
    std::ostringstream ss;
    for (const auto& faction : snapshot.affected_factions) {
        ss << "- " << faction.getName() << ": "
           << "Strength=" << faction.getStrength() << "\n";
    }
    return ss.str();
}

std::string LLMPromptBuilder::formatResourceChanges(const WorldStateSnapshot& snapshot) {
    std::ostringstream ss;
    for (const auto& resource : snapshot.changed_resources) {
        ss << "- " << resource.getName() << ": "
           << resource.getQuantity() << " units "
           << "(Scarcity threshold: 50)\n";
    }
    return ss.str();
}

std::string LLMPromptBuilder::formatTriggeredEvents(const WorldStateSnapshot& snapshot) {
    std::ostringstream ss;
    for (const auto& event : snapshot.triggered_events) {
        ss << "- " << event.getName() << " (Impact: " << event.getImpactLevel() << ")\n";
    }
    return ss.str();
}

// ============================================================================
// NarrativeGenerator Implementation
// ============================================================================

NarrativeGenerator::NarrativeGenerator(std::shared_ptr<LLMProvider> llm)
    : llm_provider(llm) {}

std::vector<NarrativeIssue> NarrativeGenerator::generateNarratives(
    const WorldStateSnapshot& snapshot,
    int current_tick,
    bool force_rule_based) {
    
    if (force_rule_based || !llm_provider || !llm_provider->isAvailable()) {
        return generateFromRules(snapshot, current_tick);
    }
    
    try {
        return generateFromLLM(snapshot, current_tick);
    } catch (...) {
        // Fallback to rule-based on LLM error
        return generateFromRules(snapshot, current_tick);
    }
}

std::vector<NarrativeIssue> NarrativeGenerator::generateFromLLM(
    const WorldStateSnapshot& snapshot,
    int current_tick) {
    
    // Build prompt and call LLM
    LLMPromptBuilder prompt_builder;
    std::string prompt = prompt_builder.buildNarrativePrompt(snapshot);
    
    // Call LLM
    LLMResponse response = llm_provider->callLLM(prompt);
    
    if (response.wasSuccessful) {
        return parseLLMResponse(response.text, current_tick);
    }
    return generateFromRules(snapshot, current_tick);
}

std::vector<NarrativeIssue> NarrativeGenerator::generateFromRules(
    const WorldStateSnapshot& snapshot,
    int current_tick) {
    
    std::vector<NarrativeIssue> issues;
    
    // Create issues from NPC mood changes
    for (const auto& npc : snapshot.significant_npcs) {
        float mood = npc.getShortTermMood();
        float mood_delta = std::abs(mood - 0.5f);  // Deviation from neutral
        
        if (mood < 0.3f) {
            issues.push_back(createNPCMoodIssue(npc, mood_delta, current_tick));
        } else if (mood > 0.8f) {
            NarrativeIssue issue;
            issue.title = npc.getName() + " reports increased morale";
            issue.description = npc.getName() + " is feeling very positive about their work.";
            issue.suggestion = "Capitalize on this morale boost to tackle challenging projects.";
            issue.priority = IssuePriority::OPPORTUNITY;
            issue.type = IssueType::NPC_CRISIS;
            issue.affected_npc_ids.push_back(npc.getId());
            issue.created_tick = current_tick;
            issues.push_back(issue);
        }
    }
    
    // Create issues from faction changes
    for (const auto& faction : snapshot.affected_factions) {
        if (faction.getStrength() < 0.3f) {
            issues.push_back(createFactionTensionIssue(faction, current_tick));
        }
    }
    
    // Create issues from resource scarcity
    for (const auto& resource : snapshot.changed_resources) {
        if (resource.getQuantity() < 50) {
            issues.push_back(createResourceScarcityIssue(resource, current_tick));
        }
    }
    
    return issues;
}

std::vector<NarrativeIssue> NarrativeGenerator::parseLLMResponse(
    const std::string& response,
    int current_tick) {
    
    std::vector<NarrativeIssue> issues;
    std::istringstream ss(response);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.find("ISSUE:") != std::string::npos) {
            // Parse format: ISSUE: {title} | {description} | {suggestion} | {priority}
            size_t pos1 = line.find("ISSUE:") + 6;
            size_t pos2 = line.find("|", pos1);
            size_t pos3 = line.find("|", pos2 + 1);
            size_t pos4 = line.find("|", pos3 + 1);
            
            if (pos2 != std::string::npos && pos3 != std::string::npos && pos4 != std::string::npos) {
                std::string title = line.substr(pos1, pos2 - pos1);
                std::string desc = line.substr(pos2 + 1, pos3 - pos2 - 1);
                std::string suggest = line.substr(pos3 + 1, pos4 - pos3 - 1);
                std::string priority_str = line.substr(pos4 + 1);
                
                // Trim whitespace
                title.erase(0, title.find_first_not_of(" \t"));
                title.erase(title.find_last_not_of(" \t") + 1);
                
                NarrativeIssue issue(
                    -1,  // ID auto-assigned
                    title,
                    desc,
                    suggest,
                    priority_str.find("CRISIS") != std::string::npos ? IssuePriority::CRISIS :
                    priority_str.find("OPPORTUNITY") != std::string::npos ? IssuePriority::OPPORTUNITY :
                    IssuePriority::WARNING,
                    IssueType::NPC_CRISIS,
                    current_tick
                );
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}

NarrativeIssue NarrativeGenerator::createNPCMoodIssue(const NPC& npc, float /*mood_delta*/, int current_tick) {
    NarrativeIssue issue;
    issue.title = npc.getName() + " reports deep concern";
    issue.description = npc.getName() + " (" + npc.getRole() + ") is troubled and needs attention.";
    issue.suggestion = "Talk to " + npc.getName() + " or improve conditions for their role.";
    issue.priority = IssuePriority::WARNING;
    issue.type = IssueType::NPC_CRISIS;
    issue.affected_npc_ids.push_back(npc.getId());
    issue.created_tick = current_tick;
    return issue;
}

NarrativeIssue NarrativeGenerator::createFactionTensionIssue(const Faction& faction, int current_tick) {
    NarrativeIssue issue;
    issue.title = faction.getName() + " faction morale declining";
    issue.description = "The " + faction.getName() + " faction is losing faith and may take action.";
    issue.suggestion = "Address faction concerns or provide resources to improve morale.";
    issue.priority = IssuePriority::CRISIS;
    issue.type = IssueType::FACTION_CONFLICT;
    issue.affected_faction_ids.push_back(faction.getId());
    issue.created_tick = current_tick;
    return issue;
}

NarrativeIssue NarrativeGenerator::createResourceScarcityIssue(const Resource& resource, int current_tick) {
    NarrativeIssue issue;
    issue.title = resource.getName() + " supplies running low";
    issue.description = "Settlement " + resource.getName() + " stores have fallen below critical levels.";
    issue.suggestion = "Increase production or trade for additional supplies.";
    issue.priority = IssuePriority::CRISIS;
    issue.type = IssueType::RESOURCE_SCARCITY;
    issue.affected_resource_ids.push_back(resource.getId());
    issue.created_tick = current_tick;
    return issue;
}

// ============================================================================
// NarrativeIssueDisplay Implementation
// ============================================================================

std::string NarrativeIssueDisplay::formatIssueForDisplay(const NarrativeIssue& issue) {
    std::ostringstream ss;
    ss << "[" << getPriorityTag(issue.priority) << "] " << issue.title << "\n";
    ss << "   " << issue.description << "\n";
    ss << "   Suggestion: " << issue.suggestion << "\n";
    return ss.str();
}

std::string NarrativeIssueDisplay::formatMultipleIssues(const std::vector<NarrativeIssue>& issues,
                                                       int max_display) {
    std::ostringstream ss;
    int count = 0;
    for (const auto& issue : issues) {
        if (count >= max_display) break;
        ss << formatIssueForDisplay(issue);
        if (count < std::min(max_display - 1, static_cast<int>(issues.size()) - 1)) {
            ss << "\n";
        }
        count++;
    }
    return ss.str();
}

std::string NarrativeIssueDisplay::formatIssueSummary(const NarrativeIssue& issue) {
    std::ostringstream ss;
    ss << "[" << getPriorityTag(issue.priority) << "] " << issue.title;
    return ss.str();
}

std::string NarrativeIssueDisplay::getPriorityTag(IssuePriority priority) {
    switch (priority) {
        case IssuePriority::CRISIS: return "⚠ CRISIS";
        case IssuePriority::WARNING: return "⚡ WARNING";
        case IssuePriority::OPPORTUNITY: return "✓ OPPORTUNITY";
        default: return "?";
    }
}

std::string NarrativeIssueDisplay::getTypeTag(IssueType type) {
    switch (type) {
        case IssueType::FACTION_CONFLICT: return "[FACTION]";
        case IssueType::RESOURCE_SCARCITY: return "[RESOURCE]";
        case IssueType::NPC_CRISIS: return "[NPC]";
        case IssueType::CULTURAL_SHIFT: return "[CULTURE]";
        case IssueType::RELIGIOUS_CONFLICT: return "[RELIGION]";
        case IssueType::ENVIRONMENTAL: return "[ENV]";
        case IssueType::OPPORTUNITY: return "[OPPORTUNITY]";
        case IssueType::IMMIGRATION: return "[IMMIGRATION]";
        case IssueType::FACTION_REBELLION: return "[REBELLION]";
        default: return "[?]";
    }
}

}  // namespace TLS
