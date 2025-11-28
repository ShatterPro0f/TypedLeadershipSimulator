#include "LLMResponseCache.h"
#include "LLMProvider.h"
#include <functional>
#include <chrono>
#include <sstream>
#include <algorithm>

namespace TLS {

// ==================== LLMResponseCache ====================

LLMResponseCache::LLMResponseCache(int ttlSeconds)
    : ttlSeconds_(ttlSeconds)
    , totalHits_(0)
    , totalMisses_(0) {
}

const LLMResponse* LLMResponseCache::get(const std::string& prompt) {
    std::string hash = computeHash(prompt);
    
    auto it = cache_.find(hash);
    if (it == cache_.end()) {
        totalMisses_++;
        return nullptr;
    }

    // Check if expired
    int64_t now = getCurrentTime();
    if (it->second.expiryTime > 0 && now > it->second.expiryTime) {
        // Expired
        cache_.erase(it);
        totalMisses_++;
        return nullptr;
    }

    // Hit
    it->second.hitCount++;
    totalHits_++;
    return &it->second.response;
}

bool LLMResponseCache::put(const std::string& prompt, const LLMResponse& response) {
    // Check cache size
    if (cache_.size() >= MAX_CACHE_SIZE) {
        return false;  // Cache full
    }

    std::string hash = computeHash(prompt);
    int64_t now = getCurrentTime();
    int64_t expiry = (ttlSeconds_ > 0) ? now + (ttlSeconds_ * 1000) : 0;

    CachedLLMResponse cached;
    cached.response = response;
    cached.cacheTime = now;
    cached.expiryTime = expiry;
    cached.hitCount = 0;

    cache_[hash] = cached;
    return true;
}

bool LLMResponseCache::contains(const std::string& prompt) {
    std::string hash = computeHash(prompt);
    
    auto it = cache_.find(hash);
    if (it == cache_.end()) {
        return false;
    }

    // Check if expired
    int64_t now = getCurrentTime();
    if (it->second.expiryTime > 0 && now > it->second.expiryTime) {
        cache_.erase(it);
        return false;
    }

    return true;
}

void LLMResponseCache::invalidate(const std::string& prompt) {
    std::string hash = computeHash(prompt);
    auto it = cache_.find(hash);
    if (it != cache_.end()) {
        cache_.erase(it);
    }
}

void LLMResponseCache::clear() {
    cache_.clear();
    totalHits_ = 0;
    totalMisses_ = 0;
}

std::string LLMResponseCache::getStatistics() const {
    std::ostringstream oss;
    oss << "Cache Statistics:\n";
    oss << "  Size: " << cache_.size() << "/" << MAX_CACHE_SIZE << "\n";
    oss << "  Hits: " << totalHits_ << "\n";
    oss << "  Misses: " << totalMisses_ << "\n";
    oss << "  Hit Rate: " << (getHitRate() * 100.0f) << "%\n";
    oss << "  TTL: " << ttlSeconds_ << " seconds\n";
    return oss.str();
}

float LLMResponseCache::getHitRate() const {
    int total = totalHits_ + totalMisses_;
    if (total == 0) return 0.0f;
    return static_cast<float>(totalHits_) / static_cast<float>(total);
}

void LLMResponseCache::evictExpired() {
    int64_t now = getCurrentTime();
    
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (it->second.expiryTime > 0 && now > it->second.expiryTime) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string LLMResponseCache::computeHash(const std::string& prompt) const {
    // Simple hash function (in production, use SHA256 or similar)
    std::hash<std::string> hasher;
    size_t hash = hasher(prompt);
    
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

int64_t LLMResponseCache::getCurrentTime() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

// ==================== OfflineLLMFallback ====================

std::string OfflineLLMFallback::generateNpcNarrative(
    const std::string& problemType,
    float severity
) {
    std::string narrative = getTemplateForProblem(problemType);
    return applySeverityScaling(narrative, severity);
}

std::string OfflineLLMFallback::interpretDecision(
    const std::string& playerInput,
    const std::string& context
) {
    // Simple keyword-based decision interpretation
    std::string decision;
    
    if (playerInput.find("allocate") != std::string::npos || playerInput.find("give") != std::string::npos) {
        decision = "{\"action\": \"allocate\", \"confidence\": 0.8}";
    } else if (playerInput.find("delegate") != std::string::npos || playerInput.find("assign") != std::string::npos) {
        decision = "{\"action\": \"delegate\", \"confidence\": 0.75}";
    } else if (playerInput.find("negotiate") != std::string::npos || playerInput.find("talk") != std::string::npos) {
        decision = "{\"action\": \"negotiate\", \"confidence\": 0.7}";
    } else if (playerInput.find("inspire") != std::string::npos || playerInput.find("motivate") != std::string::npos) {
        decision = "{\"action\": \"inspire\", \"confidence\": 0.8}";
    } else if (playerInput.find("suppress") != std::string::npos || playerInput.find("restrict") != std::string::npos) {
        decision = "{\"action\": \"suppress\", \"confidence\": 0.7}";
    } else {
        decision = "{\"action\": \"unknown\", \"confidence\": 0.5}";
    }
    
    return decision;
}

std::string OfflineLLMFallback::analyzeWorldState(const std::string& worldStateJson) {
    // Simple analysis based on keywords in world state
    std::string analysis;
    
    if (worldStateJson.find("\"food\"") != std::string::npos) {
        analysis += "Food resources are noteworthy. ";
    }
    if (worldStateJson.find("\"conflict\"") != std::string::npos) {
        analysis += "Faction tensions are rising. ";
    }
    if (worldStateJson.find("\"immigration\"") != std::string::npos) {
        analysis += "Population changes affecting settlement. ";
    }
    if (worldStateJson.find("\"religion\"") != std::string::npos) {
        analysis += "Religious movements reshaping society. ";
    }
    
    if (analysis.empty()) {
        analysis = "Settlement conditions remain relatively stable at the moment.";
    }
    
    return analysis;
}

std::string OfflineLLMFallback::generateNpcDialogue(
    const std::string& npcName,
    const std::string& npcRole,
    const std::string& topic
) {
    // Role-based dialogue templates
    std::string dialogue = npcName + " (" + npcRole + "): ";
    
    if (npcRole.find("Farmer") != std::string::npos || npcRole.find("farmer") != std::string::npos) {
        if (topic.find("food") != std::string::npos) {
            dialogue += "\"The harvest depends on good weather and hard work. We must plan ahead.\"";
        } else {
            dialogue += "\"There's much work to be done in the fields.\"";
        }
    } else if (npcRole.find("Warrior") != std::string::npos || npcRole.find("warrior") != std::string::npos) {
        if (topic.find("conflict") != std::string::npos) {
            dialogue += "\"We must remain vigilant. Threats are everywhere.\"";
        } else {
            dialogue += "\"Strength and discipline keep our settlement safe.\"";
        }
    } else if (npcRole.find("Merchant") != std::string::npos || npcRole.find("merchant") != std::string::npos) {
        if (topic.find("trade") != std::string::npos) {
            dialogue += "\"Trade brings prosperity and connections to distant lands.\"";
        } else {
            dialogue += "\"Commerce is the lifeblood of civilization.\"";
        }
    } else {
        dialogue += "\"I await your guidance, leader.\"";
    }
    
    return dialogue;
}

std::string OfflineLLMFallback::generateCrisisNarrative(
    const std::string& crisisType,
    float severity
) {
    std::string narrative;
    
    if (crisisType.find("famine") != std::string::npos || crisisType.find("food") != std::string::npos) {
        narrative = "The settlement faces food scarcity. Crops have failed and stores are depleting. ";
    } else if (crisisType.find("conflict") != std::string::npos || crisisType.find("faction") != std::string::npos) {
        narrative = "Faction tensions escalate. Different groups struggle for influence. ";
    } else if (crisisType.find("disease") != std::string::npos) {
        narrative = "Illness spreads through the settlement. Morale suffers. ";
    } else if (crisisType.find("rebellion") != std::string::npos) {
        narrative = "Discontent grows into outright rebellion. Authority is questioned. ";
    } else {
        narrative = "An unexpected crisis threatens settlement stability. ";
    }
    
    return applySeverityScaling(narrative, severity);
}

std::string OfflineLLMFallback::getTemplateForProblem(const std::string& problemType) {
    if (problemType.find("RESOURCE_SCARCITY") != std::string::npos) {
        return "Resources are becoming scarce. We must act quickly to prevent shortages.";
    } else if (problemType.find("FACTION_CONFLICT") != std::string::npos) {
        return "Tensions between factions are rising. Political divisions threaten unity.";
    } else if (problemType.find("MORAL_CRISIS") != std::string::npos) {
        return "The settlement faces a moral dilemma. Values are at stake.";
    } else if (problemType.find("PERSONAL_GRIEVANCE") != std::string::npos) {
        return "An individual harbors resentment. Personal matters affect the community.";
    } else {
        return "The settlement faces an uncertain situation. Careful leadership is needed.";
    }
}

std::string OfflineLLMFallback::applySeverityScaling(
    const std::string& template_text,
    float severity
) {
    // Modify template based on severity
    if (severity < 0.3f) {
        return template_text + " (Minor concern)";
    } else if (severity < 0.6f) {
        return template_text + " (Moderate concern)";
    } else if (severity < 0.9f) {
        return template_text + " (Serious concern)";
    } else {
        return template_text + " (CRITICAL - Immediate action required!)";
    }
}

}  // namespace TLS
