#ifndef REGISTRIES_H
#define REGISTRIES_H

#include "Core.h"
#include <map>
#include <memory>
#include <vector>

namespace TLS {

// ============================================================================
// NPC Registry - Central repository for all NPCs with O(1) lookup
// ============================================================================

class NPCRegistry {
public:
    static NPCRegistry& getInstance();

    // NPC management
    void registerNPC(const std::shared_ptr<NPC>& npc);
    void addNPC(const NPC& npc);  // Phase 12: Convenience method for testing
    void unregisterNPC(int id);
    std::shared_ptr<NPC> getNPCById(int id) const;
    bool hasNPC(int id) const;

    // Batch operations
    std::vector<std::shared_ptr<NPC>> getAllNPCs() const;
    std::vector<std::shared_ptr<NPC>> getNPCsByFaction(int factionId) const;
    std::vector<std::shared_ptr<NPC>> getNPCsByRole(const std::string& role) const;

    // Statistics
    size_t getNPCCount() const { return npcs_.size(); }
    void clear();

    // ID generation
    int getNextId();

private:
    NPCRegistry();
    NPCRegistry(const NPCRegistry&) = delete;
    NPCRegistry& operator=(const NPCRegistry&) = delete;

    std::map<int, std::shared_ptr<NPC>> npcs_;
    int nextId_ = 1;
};

// ============================================================================
// Advisor Registry - Central repository for all advisors
// ============================================================================

class AdvisorRegistry {
public:
    static AdvisorRegistry& getInstance();

    void registerAdvisor(const std::shared_ptr<Advisor>& advisor);
    void unregisterAdvisor(int id);
    std::shared_ptr<Advisor> getAdvisorById(int id) const;
    bool hasAdvisor(int id) const;

    std::vector<std::shared_ptr<Advisor>> getAllAdvisors() const;
    std::vector<std::shared_ptr<Advisor>> getAdvisorsBySpecialty(Specialty spec) const;

    size_t getAdvisorCount() const { return advisors_.size(); }
    void clear();

    int getNextId();

private:
    AdvisorRegistry();
    AdvisorRegistry(const AdvisorRegistry&) = delete;
    AdvisorRegistry& operator=(const AdvisorRegistry&) = delete;

    std::map<int, std::shared_ptr<Advisor>> advisors_;
    int nextId_ = 10001;  // Different range from NPCs
};

// ============================================================================
// Faction Registry - Central repository for all factions
// ============================================================================

class FactionRegistry {
public:
    static FactionRegistry& getInstance();

    void registerFaction(const std::shared_ptr<Faction>& faction);
    void addFaction(const Faction& faction);  // Phase 12: Convenience method
    void unregisterFaction(int id);
    std::shared_ptr<Faction> getFactionById(int id) const;
    bool hasFaction(int id) const;

    std::vector<std::shared_ptr<Faction>> getAllFactions() const;
    std::vector<std::shared_ptr<Faction>> getFactionsByAlignment(Alignment align) const;

    size_t getFactionCount() const { return factions_.size(); }
    void clear();

    int getNextId();

private:
    FactionRegistry();
    FactionRegistry(const FactionRegistry&) = delete;
    FactionRegistry& operator=(const FactionRegistry&) = delete;

    std::map<int, std::shared_ptr<Faction>> factions_;
    int nextId_ = 20001;  // Different range
};

// ============================================================================
// Resource Registry - Central repository for all resources
// ============================================================================

class ResourceRegistry {
public:
    static ResourceRegistry& getInstance();

    void registerResource(const std::shared_ptr<Resource>& resource);
    void addResource(const Resource& resource);  // Phase 12: Convenience method
    void unregisterResource(int id);
    std::shared_ptr<Resource> getResourceById(int id) const;
    std::shared_ptr<Resource> getResourceByName(const std::string& name) const;
    bool hasResource(int id) const;
    bool hasResourceByName(const std::string& name) const;

    std::vector<std::shared_ptr<Resource>> getAllResources() const;
    std::vector<std::shared_ptr<Resource>> getScareResources() const;

    size_t getResourceCount() const { return resources_.size(); }
    void clear();

    int getNextId();

private:
    ResourceRegistry();
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;

    std::map<int, std::shared_ptr<Resource>> resources_;
    std::map<std::string, int> nameToId_;
    int nextId_ = 30001;
};

// ============================================================================
// Event Registry - Central repository for all events
// ============================================================================

class EventRegistry {
public:
    static EventRegistry& getInstance();

    void registerEvent(const std::shared_ptr<Event>& event);
    void unregisterEvent(int id);
    std::shared_ptr<Event> getEventById(int id) const;
    bool hasEvent(int id) const;

    std::vector<std::shared_ptr<Event>> getAllEvents() const;
    std::vector<std::shared_ptr<Event>> getEventsByType(EventType type) const;

    size_t getEventCount() const { return events_.size(); }
    void clear();

    int getNextId();

private:
    EventRegistry();
    EventRegistry(const EventRegistry&) = delete;
    EventRegistry& operator=(const EventRegistry&) = delete;

    std::map<int, std::shared_ptr<Event>> events_;
    int nextId_ = 40001;
};

// ============================================================================
// Global Registry Manager - Convenience accessor for all registries
// ============================================================================

class RegistryManager {
public:
    static RegistryManager& getInstance();

    NPCRegistry& npcs() { return NPCRegistry::getInstance(); }
    AdvisorRegistry& advisors() { return AdvisorRegistry::getInstance(); }
    FactionRegistry& factions() { return FactionRegistry::getInstance(); }
    ResourceRegistry& resources() { return ResourceRegistry::getInstance(); }
    EventRegistry& events() { return EventRegistry::getInstance(); }

    void clearAll();

private:
    RegistryManager() = default;
    RegistryManager(const RegistryManager&) = delete;
    RegistryManager& operator=(const RegistryManager&) = delete;
};

}  // namespace TLS

#endif  // REGISTRIES_H
