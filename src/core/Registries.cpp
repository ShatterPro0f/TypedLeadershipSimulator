#include "Registries.h"
#include <algorithm>

namespace TLS {

// ============================================================================
// NPCRegistry Implementation
// ============================================================================

NPCRegistry& NPCRegistry::getInstance() {
    static NPCRegistry instance;
    return instance;
}

NPCRegistry::NPCRegistry() : nextId_(1) {}

void NPCRegistry::registerNPC(const std::shared_ptr<NPC>& npc) {
    if (npc) {
        npcs_[npc->getId()] = npc;
    }
}

// Phase 12: Convenience method for testing
void NPCRegistry::addNPC(const NPC& npc) {
    auto sharedNPC = std::make_shared<NPC>(npc);
    registerNPC(sharedNPC);
}

void NPCRegistry::unregisterNPC(int id) {
    npcs_.erase(id);
}

std::shared_ptr<NPC> NPCRegistry::getNPCById(int id) const {
    auto it = npcs_.find(id);
    return (it != npcs_.end()) ? it->second : nullptr;
}

bool NPCRegistry::hasNPC(int id) const {
    return npcs_.find(id) != npcs_.end();
}

std::vector<std::shared_ptr<NPC>> NPCRegistry::getAllNPCs() const {
    std::vector<std::shared_ptr<NPC>> result;
    for (const auto& pair : npcs_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<NPC>> NPCRegistry::getNPCsByFaction(int factionId) const {
    std::vector<std::shared_ptr<NPC>> result;
    for (const auto& pair : npcs_) {
        if (pair.second && pair.second->getFactionId() == factionId) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<NPC>> NPCRegistry::getNPCsByRole(const std::string& role) const {
    std::vector<std::shared_ptr<NPC>> result;
    for (const auto& pair : npcs_) {
        if (pair.second && pair.second->getRole() == role) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void NPCRegistry::clear() {
    npcs_.clear();
    nextId_ = 1;
}

int NPCRegistry::getNextId() {
    return nextId_++;
}

// ============================================================================
// AdvisorRegistry Implementation
// ============================================================================

AdvisorRegistry& AdvisorRegistry::getInstance() {
    static AdvisorRegistry instance;
    return instance;
}

AdvisorRegistry::AdvisorRegistry() : nextId_(10001) {}

void AdvisorRegistry::registerAdvisor(const std::shared_ptr<Advisor>& advisor) {
    if (advisor) {
        advisors_[advisor->getId()] = advisor;
    }
}

void AdvisorRegistry::unregisterAdvisor(int id) {
    advisors_.erase(id);
}

std::shared_ptr<Advisor> AdvisorRegistry::getAdvisorById(int id) const {
    auto it = advisors_.find(id);
    return (it != advisors_.end()) ? it->second : nullptr;
}

bool AdvisorRegistry::hasAdvisor(int id) const {
    return advisors_.find(id) != advisors_.end();
}

std::vector<std::shared_ptr<Advisor>> AdvisorRegistry::getAllAdvisors() const {
    std::vector<std::shared_ptr<Advisor>> result;
    for (const auto& pair : advisors_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<Advisor>> AdvisorRegistry::getAdvisorsBySpecialty(Specialty spec) const {
    std::vector<std::shared_ptr<Advisor>> result;
    for (const auto& pair : advisors_) {
        if (pair.second && pair.second->getSpecialty() == spec) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void AdvisorRegistry::clear() {
    advisors_.clear();
    nextId_ = 10001;
}

int AdvisorRegistry::getNextId() {
    return nextId_++;
}

// ============================================================================
// FactionRegistry Implementation
// ============================================================================

FactionRegistry& FactionRegistry::getInstance() {
    static FactionRegistry instance;
    return instance;
}

FactionRegistry::FactionRegistry() : nextId_(20001) {}

void FactionRegistry::registerFaction(const std::shared_ptr<Faction>& faction) {
    if (faction) {
        factions_[faction->getId()] = faction;
    }
}

void FactionRegistry::addFaction(const Faction& faction) {
    auto sharedFaction = std::make_shared<Faction>(faction);
    registerFaction(sharedFaction);
}

void FactionRegistry::unregisterFaction(int id) {
    factions_.erase(id);
}

std::shared_ptr<Faction> FactionRegistry::getFactionById(int id) const {
    auto it = factions_.find(id);
    return (it != factions_.end()) ? it->second : nullptr;
}

bool FactionRegistry::hasFaction(int id) const {
    return factions_.find(id) != factions_.end();
}

std::vector<std::shared_ptr<Faction>> FactionRegistry::getAllFactions() const {
    std::vector<std::shared_ptr<Faction>> result;
    for (const auto& pair : factions_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<Faction>> FactionRegistry::getFactionsByAlignment(Alignment align) const {
    std::vector<std::shared_ptr<Faction>> result;
    for (const auto& pair : factions_) {
        if (pair.second && pair.second->getAlignment() == align) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void FactionRegistry::clear() {
    factions_.clear();
    nextId_ = 20001;
}

int FactionRegistry::getNextId() {
    return nextId_++;
}

// ============================================================================
// ResourceRegistry Implementation
// ============================================================================

ResourceRegistry& ResourceRegistry::getInstance() {
    static ResourceRegistry instance;
    return instance;
}

ResourceRegistry::ResourceRegistry() : nextId_(30001) {}

void ResourceRegistry::registerResource(const std::shared_ptr<Resource>& resource) {
    if (resource) {
        resources_[resource->getId()] = resource;
        nameToId_[resource->getName()] = resource->getId();
    }
}

void ResourceRegistry::addResource(const Resource& resource) {
    auto sharedResource = std::make_shared<Resource>(resource);
    registerResource(sharedResource);
}

void ResourceRegistry::unregisterResource(int id) {
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        nameToId_.erase(it->second->getName());
        resources_.erase(it);
    }
}

std::shared_ptr<Resource> ResourceRegistry::getResourceById(int id) const {
    auto it = resources_.find(id);
    return (it != resources_.end()) ? it->second : nullptr;
}

std::shared_ptr<Resource> ResourceRegistry::getResourceByName(const std::string& name) const {
    auto it = nameToId_.find(name);
    if (it != nameToId_.end()) {
        return getResourceById(it->second);
    }
    return nullptr;
}

bool ResourceRegistry::hasResource(int id) const {
    return resources_.find(id) != resources_.end();
}

bool ResourceRegistry::hasResourceByName(const std::string& name) const {
    return nameToId_.find(name) != nameToId_.end();
}

std::vector<std::shared_ptr<Resource>> ResourceRegistry::getAllResources() const {
    std::vector<std::shared_ptr<Resource>> result;
    for (const auto& pair : resources_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<Resource>> ResourceRegistry::getScareResources() const {
    std::vector<std::shared_ptr<Resource>> result;
    for (const auto& pair : resources_) {
        if (pair.second && pair.second->isScarse()) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void ResourceRegistry::clear() {
    resources_.clear();
    nameToId_.clear();
    nextId_ = 30001;
}

int ResourceRegistry::getNextId() {
    return nextId_++;
}

// ============================================================================
// EventRegistry Implementation
// ============================================================================

EventRegistry& EventRegistry::getInstance() {
    static EventRegistry instance;
    return instance;
}

EventRegistry::EventRegistry() : nextId_(40001) {}

void EventRegistry::registerEvent(const std::shared_ptr<Event>& event) {
    if (event) {
        events_[event->getId()] = event;
    }
}

void EventRegistry::unregisterEvent(int id) {
    events_.erase(id);
}

std::shared_ptr<Event> EventRegistry::getEventById(int id) const {
    auto it = events_.find(id);
    return (it != events_.end()) ? it->second : nullptr;
}

bool EventRegistry::hasEvent(int id) const {
    return events_.find(id) != events_.end();
}

std::vector<std::shared_ptr<Event>> EventRegistry::getAllEvents() const {
    std::vector<std::shared_ptr<Event>> result;
    for (const auto& pair : events_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<Event>> EventRegistry::getEventsByType(EventType type) const {
    std::vector<std::shared_ptr<Event>> result;
    for (const auto& pair : events_) {
        if (pair.second && pair.second->getType() == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void EventRegistry::clear() {
    events_.clear();
    nextId_ = 40001;
}

int EventRegistry::getNextId() {
    return nextId_++;
}

// ============================================================================
// RegistryManager Implementation
// ============================================================================

RegistryManager& RegistryManager::getInstance() {
    static RegistryManager instance;
    return instance;
}

void RegistryManager::clearAll() {
    NPCRegistry::getInstance().clear();
    AdvisorRegistry::getInstance().clear();
    FactionRegistry::getInstance().clear();
    ResourceRegistry::getInstance().clear();
    EventRegistry::getInstance().clear();
}

}  // namespace TLS
