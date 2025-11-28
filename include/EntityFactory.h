#ifndef TLS_ENTITY_FACTORY_H
#define TLS_ENTITY_FACTORY_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Core.h"

namespace TLS {

// ============================================================================
// EntityFactory - Factory Pattern for Entity Creation
// ============================================================================

class EntityFactory {
public:
    static EntityFactory* instance();
    
    // ========== NPC Creation Methods ==========
    std::shared_ptr<NPC> createNPC(const std::string& name, int age, const std::string& gender,
                                    const std::string& role, int factionId);
    
    std::shared_ptr<NPC> createNPCFromData(const std::map<std::string, std::string>& data);
    
    std::shared_ptr<NPC> createRandomNPC(const std::string& roleConstraint = "", int factionId = -1);
    
    // ========== NPC Builder Pattern ==========
    class NPCBuilder {
    public:
        NPCBuilder& withName(const std::string& name);
        NPCBuilder& withAge(int age);
        NPCBuilder& withGender(const std::string& gender);
        NPCBuilder& withRole(const std::string& role);
        NPCBuilder& withFaction(int factionId);
        NPCBuilder& withLoyalty(float loyalty);
        NPCBuilder& withMood(float mood);
        
        std::shared_ptr<NPC> build();
    
    private:
        std::string name_ = "Unknown";
        int age_ = 25;
        std::string gender_ = "unknown";
        std::string role_ = "settler";
        int factionId_ = -1;
        float loyalty_ = 0.5f;
        float mood_ = 0.5f;
    };
    
    // ========== Advisor Creation Methods ==========
    std::shared_ptr<Advisor> createAdvisor(const std::string& name, Specialty specialty, int factionId);
    
    // ========== Advisor Builder Pattern ==========
    class AdvisorBuilder {
    public:
        AdvisorBuilder& withName(const std::string& name);
        AdvisorBuilder& withSpecialty(Specialty specialty);
        AdvisorBuilder& withTrust(float trust);
        AdvisorBuilder& withAgenda(Agenda agenda);
        AdvisorBuilder& withStrategy(StrategyStyle style);
        
        std::shared_ptr<Advisor> build();
    
    private:
        std::string name_ = "Unnamed Advisor";
        Specialty specialty_ = Specialty::POLITICS;
        float trust_ = 0.6f;
        Agenda agenda_ = Agenda::SHORT_TERM;
        StrategyStyle strategy_ = StrategyStyle::HONEST;
    };
    
    // ========== Resource Creation Methods ==========
    std::shared_ptr<Resource> createResource(const std::string& name, int initialQuantity);
    
    // ========== Faction Creation Methods ==========
    std::shared_ptr<Faction> createFaction(const std::string& name);
    
    // ========== Event Creation Methods ==========
    std::shared_ptr<Event> createEvent(const std::string& name, EventType type, int impactLevel);
    
    // ========== Batch Operations ==========
    void createInitialSettlement(int npcCount = 10);
    
    std::vector<std::map<std::string, std::string>> loadScenarioData(const std::string& scenarioName);

private:
    EntityFactory() = default;
    static EntityFactory* instance_;
    static int nextNPCId_;
    static int nextAdvisorId_;
    static int nextResourceId_;
    static int nextFactionId_;
    static int nextEventId_;
    
    int getNextNPCId() { return nextNPCId_++; }
    int getNextAdvisorId() { return nextAdvisorId_++; }
    int getNextResourceId() { return nextResourceId_++; }
    int getNextFactionId() { return nextFactionId_++; }
    int getNextEventId() { return nextEventId_++; }
};

} // namespace TLS

#endif // TLS_ENTITY_FACTORY_H
