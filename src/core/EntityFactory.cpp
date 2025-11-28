#include "EntityFactory.h"
#include "Registries.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>

namespace TLS {

// ============================================================================
// EntityFactory Implementation
// ============================================================================

EntityFactory* EntityFactory::instance() {
    static EntityFactory factory;
    return &factory;
}

std::shared_ptr<NPC> EntityFactory::createNPC(const std::string& name, int age, const std::string& gender,
                                              const std::string& role, int factionId) {
    // Get next NPC ID from registry (starting at 1)
    int npcId = NPCRegistry::getInstance().getNextId();
    auto npc = std::make_shared<NPC>(npcId);
    
    // Use public setters only
    npc->setName(name);
    npc->setAge(age);
    npc->setGender(gender);
    npc->setRole(role);
    npc->setFactionId(factionId);
    npc->setLoyalty(0.5f);
    npc->setShortTermMood(0.5f);
    npc->setLongTermAttitude(0.5f);
    npc->setActivity(Activity::IDLE);
    npc->setPosition(Vector3(0, 0, 0));
    npc->setHomeLocation(Vector3(0, 0, 0));
    
    // Register immediately
    NPCRegistry::getInstance().registerNPC(npc);
    
    return npc;
}

std::shared_ptr<NPC> EntityFactory::createNPCFromData(const std::map<std::string, std::string>& data) {
    // Get next NPC ID from registry
    int npcId = NPCRegistry::getInstance().getNextId();
    auto npc = std::make_shared<NPC>(npcId);
    
    // Extract from data map with defaults, using public setters
    npc->setName(data.count("name") ? data.at("name") : "Unknown");
    npc->setAge(data.count("age") ? std::stoi(data.at("age")) : 25);
    npc->setGender(data.count("gender") ? data.at("gender") : "unknown");
    npc->setRole(data.count("role") ? data.at("role") : "settler");
    npc->setFactionId(data.count("factionId") ? std::stoi(data.at("factionId")) : -1);
    npc->setLoyalty(data.count("loyalty") ? std::stof(data.at("loyalty")) : 0.5f);
    npc->setShortTermMood(data.count("mood") ? std::stof(data.at("mood")) : 0.5f);
    npc->setLongTermAttitude(data.count("attitude") ? std::stof(data.at("attitude")) : 0.5f);
    
    // Parse position if provided
    if (data.count("posX") && data.count("posY") && data.count("posZ")) {
        npc->setPosition(Vector3(std::stof(data.at("posX")), std::stof(data.at("posY")), std::stof(data.at("posZ"))));
    } else {
        npc->setPosition(Vector3(0, 0, 0));
    }
    
    // Parse home location
    if (data.count("homeX") && data.count("homeY") && data.count("homeZ")) {
        npc->setHomeLocation(Vector3(std::stof(data.at("homeX")), std::stof(data.at("homeY")), std::stof(data.at("homeZ"))));
    } else {
        npc->setHomeLocation(Vector3(0, 0, 0));
    }
    
    npc->setActivity(Activity::IDLE);
    npc->setConversationState(0);
    
    // Register immediately
    NPCRegistry::getInstance().registerNPC(npc);
    
    return npc;
}

// ============================================================================
// NPCBuilder Implementation
// ============================================================================

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withName(const std::string& name) {
    name_ = name;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withAge(int age) {
    age_ = age;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withGender(const std::string& gender) {
    gender_ = gender;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withRole(const std::string& role) {
    role_ = role;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withFaction(int factionId) {
    factionId_ = factionId;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withLoyalty(float loyalty) {
    loyalty_ = loyalty;
    return *this;
}

EntityFactory::NPCBuilder& EntityFactory::NPCBuilder::withMood(float mood) {
    mood_ = mood;
    return *this;
}

std::shared_ptr<NPC> EntityFactory::NPCBuilder::build() {
    // Get next NPC ID from registry
    int npcId = NPCRegistry::getInstance().getNextId();
    auto npc = std::make_shared<NPC>(npcId);
    
    npc->setName(name_);
    npc->setAge(age_);
    npc->setGender(gender_);
    npc->setRole(role_);
    npc->setFactionId(factionId_);
    npc->setLoyalty(loyalty_);
    npc->setShortTermMood(mood_);
    npc->setLongTermAttitude(0.5f);
    npc->setActivity(Activity::IDLE);
    npc->setPosition(Vector3(0, 0, 0));
    npc->setHomeLocation(Vector3(0, 0, 0));
    
    // Register immediately
    NPCRegistry::getInstance().registerNPC(npc);
    
    return npc;
}

// ============================================================================
// AdvisorBuilder Implementation
// ============================================================================

EntityFactory::AdvisorBuilder& EntityFactory::AdvisorBuilder::withName(const std::string& name) {
    name_ = name;
    return *this;
}

EntityFactory::AdvisorBuilder& EntityFactory::AdvisorBuilder::withSpecialty(Specialty spec) {
    specialty_ = spec;
    return *this;
}

EntityFactory::AdvisorBuilder& EntityFactory::AdvisorBuilder::withTrust(float trust) {
    trust_ = trust;
    return *this;
}

EntityFactory::AdvisorBuilder& EntityFactory::AdvisorBuilder::withAgenda(Agenda agenda) {
    agenda_ = agenda;
    return *this;
}

EntityFactory::AdvisorBuilder& EntityFactory::AdvisorBuilder::withStrategy(StrategyStyle style) {
    strategy_ = style;
    return *this;
}

std::shared_ptr<Advisor> EntityFactory::AdvisorBuilder::build() {
    // Get next Advisor ID from registry (starts at 10001)
    int advisorId = AdvisorRegistry::getInstance().getNextId();
    auto advisor = std::make_shared<Advisor>(advisorId);
    
    advisor->setName(name_);
    advisor->setSpecialty(specialty_);
    advisor->setTrustLevel(trust_);
    advisor->setAgenda(agenda_);
    advisor->setStrategyStyle(strategy_);
    advisor->setRole("advisor");
    advisor->setFactionId(0);
    advisor->setLoyalty(0.7f);
    advisor->setShortTermMood(0.5f);
    advisor->setLongTermAttitude(0.5f);
    advisor->setActivity(Activity::IDLE);
    advisor->setPosition(Vector3(0, 0, 0));
    advisor->setHomeLocation(Vector3(0, 0, 0));
    
    // Register immediately
    AdvisorRegistry::getInstance().registerAdvisor(advisor);
    
    return advisor;
}

// ============================================================================
// Advisor Creation
// ============================================================================

std::shared_ptr<Advisor> EntityFactory::createAdvisor(const std::string& name, Specialty specialty, int factionId) {
    // Get next Advisor ID from registry (starts at 10001)
    int advisorId = AdvisorRegistry::getInstance().getNextId();
    auto advisor = std::make_shared<Advisor>(advisorId);
    
    advisor->setName(name);
    advisor->setSpecialty(specialty);
    advisor->setRole("advisor");
    advisor->setFactionId(factionId);
    advisor->setInfluenceScore(0.5f);
    advisor->setTrustLevel(0.6f);
    advisor->setLoyalty(0.6f);
    advisor->setShortTermMood(0.5f);
    advisor->setLongTermAttitude(0.5f);
    advisor->setAgenda(Agenda::SHORT_TERM);
    advisor->setStrategyStyle(StrategyStyle::HONEST);
    advisor->setActivity(Activity::IDLE);
    advisor->setPosition(Vector3(0, 0, 0));
    advisor->setHomeLocation(Vector3(0, 0, 0));
    
    // Register immediately
    AdvisorRegistry::getInstance().registerAdvisor(advisor);
    
    return advisor;
}

// ============================================================================
// Resource Creation
// ============================================================================

std::shared_ptr<Resource> EntityFactory::createResource(const std::string& name, int initialQuantity) {
    // Get next Resource ID from registry (starts at 30001)
    int resourceId = ResourceRegistry::getInstance().getNextId();
    auto resource = std::make_shared<Resource>(resourceId);
    
    resource->setName(name);
    resource->setQuantity(initialQuantity);
    resource->setProductionRate(5);
    resource->setConsumptionRate(2);
    resource->setScarcityThreshold(50);
    resource->setLocation(Vector3(0, 0, 0));
    
    // Register immediately
    ResourceRegistry::getInstance().registerResource(resource);
    
    return resource;
}

// ============================================================================
// Faction Creation
// ============================================================================

std::shared_ptr<Faction> EntityFactory::createFaction(const std::string& name) {
    // Get next Faction ID from registry (starts at 20001)
    int factionId = FactionRegistry::getInstance().getNextId();
    auto faction = std::make_shared<Faction>(factionId);
    
    faction->setName(name);
    faction->setAlignment(Alignment::NEUTRAL);
    faction->setStrength(0.5f);
    faction->setHomeLocation(Vector3(0, 0, 0));
    
    // Register immediately
    FactionRegistry::getInstance().registerFaction(faction);
    
    return faction;
}

// ============================================================================
// Event Creation
// ============================================================================

std::shared_ptr<Event> EntityFactory::createEvent(const std::string& name, EventType type, int impactLevel) {
    // Get next Event ID from registry (starts at 40001)
    int eventId = EventRegistry::getInstance().getNextId();
    auto event = std::make_shared<Event>(eventId);
    
    event->setName(name);
    event->setType(type);
    event->setImpactLevel(impactLevel);
    event->setLocation(Vector3(0, 0, 0));
    
    // Register immediately
    EventRegistry::getInstance().registerEvent(event);
    
    return event;
}

// ============================================================================
// Batch Operations
// ============================================================================

std::shared_ptr<NPC> EntityFactory::createRandomNPC(const std::string& roleConstraint, int factionId) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::vector<std::string> roles = {"farmer", "merchant", "warrior", "priest", "craftsman", "scholar"};
    std::vector<std::string> names = {"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Henry"};
    std::vector<std::string> genders = {"male", "female", "non-binary"};
    
    std::uniform_int_distribution<> roleIdx(0, roles.size() - 1);
    std::uniform_int_distribution<> nameIdx(0, names.size() - 1);
    std::uniform_int_distribution<> genderIdx(0, genders.size() - 1);
    std::uniform_int_distribution<> ageRange(18, 60);
    
    std::string role = roleConstraint.empty() ? roles[roleIdx(gen)] : roleConstraint;
    std::string name = names[nameIdx(gen)];
    std::string gender = genders[genderIdx(gen)];
    int age = ageRange(gen);
    
    return createNPC(name, age, gender, role, factionId);
}

void EntityFactory::createInitialSettlement(int npcCount) {
    // Create initial factions
    auto farmers = createFaction("Farmers");
    auto merchants = createFaction("Merchants");
    auto warriors = createFaction("Warriors");
    
    // Create NPCs distributed across factions
    for (int i = 0; i < npcCount; ++i) {
        int factionId = (i % 3) + 1;
        auto npc = createRandomNPC("", factionId);
        NPCRegistry::getInstance().registerNPC(npc);
    }
}

std::vector<std::map<std::string, std::string>> EntityFactory::loadScenarioData(const std::string& scenarioName) {
    std::vector<std::map<std::string, std::string>> npcData;
    
    // Predefined scenarios
    if (scenarioName == "tutorial") {
        // Tutorial scenario with 5 NPCs
        npcData.push_back({
            {"name", "Alice"}, {"age", "25"}, {"gender", "female"}, {"role", "farmer"},
            {"loyalty", "0.7"}, {"mood", "0.6"}
        });
        npcData.push_back({
            {"name", "Bob"}, {"age", "30"}, {"gender", "male"}, {"role", "warrior"},
            {"loyalty", "0.5"}, {"mood", "0.5"}
        });
        npcData.push_back({
            {"name", "Charlie"}, {"age", "35"}, {"gender", "male"}, {"role", "merchant"},
            {"loyalty", "0.6"}, {"mood", "0.7"}
        });
        npcData.push_back({
            {"name", "Diana"}, {"age", "28"}, {"gender", "female"}, {"role", "priest"},
            {"loyalty", "0.8"}, {"mood", "0.6"}
        });
        npcData.push_back({
            {"name", "Eve"}, {"age", "22"}, {"gender", "female"}, {"role", "settler"},
            {"loyalty", "0.5"}, {"mood", "0.5"}
        });
    } else if (scenarioName == "default") {
        // Default scenario with 10 NPCs
        for (int i = 0; i < 10; ++i) {
            npcData.push_back({
                {"name", "NPC_" + std::to_string(i)},
                {"age", std::to_string(20 + (i % 40))},
                {"gender", (i % 2 == 0) ? "male" : "female"},
                {"role", (i % 3 == 0) ? "farmer" : (i % 3 == 1) ? "warrior" : "merchant"},
                {"loyalty", "0.5"},
                {"mood", "0.5"}
            });
        }
    } else if (scenarioName == "challenge") {
        // Challenge scenario with diverse NPCs and factions
        for (int i = 0; i < 15; ++i) {
            npcData.push_back({
                {"name", "Settler_" + std::to_string(i)},
                {"age", std::to_string(18 + (i % 50))},
                {"gender", (i % 2 == 0) ? "male" : "female"},
                {"role", "settler"},
                {"loyalty", std::to_string(0.3f + (i % 5) * 0.1f)},
                {"mood", std::to_string(0.4f + (i % 6) * 0.08f)}
            });
        }
    }
    
    return npcData;
}

} // namespace TLS
