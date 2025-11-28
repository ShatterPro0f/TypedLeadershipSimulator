#include <gtest/gtest.h>
#include "Enums.h"
#include "Vector3.h"
#include "Core.h"
#include "Registries.h"
#include <memory>

using namespace TLS;

// ============================================================================
// Test Suite 1: Enum Conversions (Test 1.1 - 1.4)
// ============================================================================

class EnumConversions : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnumConversions, MoodConversionsAllTypes) {
    // Test all mood types convert to/from string correctly
    std::vector<Mood> moods = {
        Mood::NEUTRAL, Mood::HAPPY, Mood::CONTENT, Mood::ANXIOUS,
        Mood::ANGRY, Mood::FEARFUL, Mood::SAD, Mood::EXCITED
    };

    for (Mood mood : moods) {
        std::string str = moodToString(mood);
        Mood recovered = stringToMood(str);
        EXPECT_EQ(mood, recovered) << "Mood conversion failed for: " << str;
    }
}

TEST_F(EnumConversions, SkillConversions) {
    std::vector<Skill> skills = {
        Skill::AGRICULTURE, Skill::DIPLOMACY, Skill::COMBAT, Skill::EDUCATION,
        Skill::CRAFTSMANSHIP, Skill::LEADERSHIP, Skill::SPIRITUALITY,
        Skill::COMMERCE, Skill::MEDICINE, Skill::STEALTH
    };

    for (Skill skill : skills) {
        std::string str = skillToString(skill);
        Skill recovered = stringToSkill(str);
        EXPECT_EQ(skill, recovered) << "Skill conversion failed for: " << str;
    }
}

TEST_F(EnumConversions, SpecialtyConversions) {
    std::vector<Specialty> specialties = {
        Specialty::POLITICS, Specialty::MILITARY, Specialty::CULTURE, Specialty::RELIGION
    };

    for (Specialty spec : specialties) {
        std::string str = specialtyToString(spec);
        Specialty recovered = stringToSpecialty(str);
        EXPECT_EQ(spec, recovered);
    }
}

TEST_F(EnumConversions, EventTypeConversions) {
    std::vector<EventType> types = {
        EventType::ENVIRONMENTAL, EventType::POLITICAL, EventType::ECONOMIC,
        EventType::SOCIAL, EventType::RELIGIOUS, EventType::MILITARY
    };

    for (EventType type : types) {
        std::string str = eventTypeToString(type);
        EventType recovered = stringToEventType(str);
        EXPECT_EQ(type, recovered);
    }
}

// ============================================================================
// Test Suite 2: Vector3 Math Operations (Test 2.1 - 2.5)
// ============================================================================

class Vector3Operations : public ::testing::Test {
protected:
    Vector3 v1{3.0f, 4.0f, 0.0f};
    Vector3 v2{1.0f, 1.0f, 1.0f};
    const float epsilon = 1e-6f;
};

TEST_F(Vector3Operations, VectorAddition) {
    Vector3 result = v1 + v2;
    EXPECT_EQ(result.x, 4.0f);
    EXPECT_EQ(result.y, 5.0f);
    EXPECT_EQ(result.z, 1.0f);
}

TEST_F(Vector3Operations, VectorSubtraction) {
    Vector3 result = v1 - v2;
    EXPECT_EQ(result.x, 2.0f);
    EXPECT_EQ(result.y, 3.0f);
    EXPECT_EQ(result.z, -1.0f);
}

TEST_F(Vector3Operations, VectorScaling) {
    Vector3 result = v1 * 2.0f;
    EXPECT_EQ(result.x, 6.0f);
    EXPECT_EQ(result.y, 8.0f);
    EXPECT_EQ(result.z, 0.0f);
}

TEST_F(Vector3Operations, VectorMagnitude) {
    // v1 = {3, 4, 0} has magnitude 5
    float mag = v1.magnitude();
    EXPECT_NEAR(mag, 5.0f, epsilon);
}

TEST_F(Vector3Operations, VectorDistance) {
    Vector3 a{0, 0, 0};
    Vector3 b{3, 4, 0};
    float dist = distance(a, b);
    EXPECT_NEAR(dist, 5.0f, epsilon);
}

TEST_F(Vector3Operations, VectorNormalization) {
    Vector3 v{3.0f, 4.0f, 0.0f};
    Vector3 norm = v.normalized();
    float mag = norm.magnitude();
    EXPECT_NEAR(mag, 1.0f, epsilon);
}

TEST_F(Vector3Operations, ManhattanDistance) {
    Vector3 a{0, 0, 0};
    Vector3 b{3, 4, 5};
    float manhattan = manhattanDistance(a, b);
    EXPECT_EQ(manhattan, 12.0f);
}

TEST_F(Vector3Operations, VectorEquality) {
    Vector3 v3{3.0f, 4.0f, 0.0f};
    EXPECT_EQ(v1, v3);
    EXPECT_NE(v1, v2);
}

// ============================================================================
// Test Suite 3: NPC Data Structure (Test 3.1 - 3.5)
// ============================================================================

class NPCDataStructure : public ::testing::Test {
protected:
    void SetUp() override {
        npc = std::make_shared<NPC>(1);
    }

    std::shared_ptr<NPC> npc;
};

TEST_F(NPCDataStructure, NPCCreationAndBasicAttributes) {
    EXPECT_EQ(npc->getId(), 1);
    npc->setName("Alice");
    EXPECT_EQ(npc->getName(), "Alice");
    npc->setAge(35);
    EXPECT_EQ(npc->getAge(), 35);
}

TEST_F(NPCDataStructure, NPCPositionAndMovement) {
    Vector3 pos{100, 50, 0};
    npc->setPosition(pos);
    EXPECT_EQ(npc->getPosition(), pos);

    Vector3 dest{150, 100, 0};
    npc->setDestination(dest);
    EXPECT_EQ(npc->getDestination(), dest);
}

TEST_F(NPCDataStructure, NPCEmotionalState) {
    npc->setImmediateEmotion(0.7f);
    EXPECT_EQ(npc->getImmediateEmotion(), 0.7f);

    npc->setShortTermMood(0.6f);
    EXPECT_EQ(npc->getShortTermMood(), 0.6f);

    npc->setLongTermAttitude(0.75f);
    EXPECT_EQ(npc->getLongTermAttitude(), 0.75f);
}

TEST_F(NPCDataStructure, NPCLoyaltyClamping) {
    // Loyalty should be clamped to [0, 1]
    npc->setLoyalty(1.5f);
    EXPECT_EQ(npc->getLoyalty(), 1.0f);

    npc->setLoyalty(-0.5f);
    EXPECT_EQ(npc->getLoyalty(), 0.0f);

    npc->setLoyalty(0.75f);
    EXPECT_EQ(npc->getLoyalty(), 0.75f);
}

TEST_F(NPCDataStructure, NPCSkillsAndPersonality) {
    npc->addSkill(Skill::AGRICULTURE);
    npc->addSkill(Skill::DIPLOMACY);
    EXPECT_EQ(npc->getSkills().size(), static_cast<size_t>(2));

    npc->addPersonalityTrait("cautious");
    npc->addPersonalityTrait("ambitious");
    EXPECT_EQ(npc->getPersonality().size(), static_cast<size_t>(2));
}

// ============================================================================
// Test Suite 4: Advisor Data Structure (Test 4.1 - 4.3)
// ============================================================================

class AdvisorDataStructure : public ::testing::Test {
protected:
    void SetUp() override {
        advisor = std::make_shared<Advisor>(10001);
    }

    std::shared_ptr<Advisor> advisor;
};

TEST_F(AdvisorDataStructure, AdvisorSpecialtyAndInfluence) {
    advisor->setSpecialty(Specialty::MILITARY);
    EXPECT_EQ(advisor->getSpecialty(), Specialty::MILITARY);

    advisor->setInfluenceScore(0.85f);
    EXPECT_EQ(advisor->getInfluenceScore(), 0.85f);
}

TEST_F(AdvisorDataStructure, AdvisorTrustAndAlignment) {
    advisor->setTrustLevel(0.8f);
    EXPECT_EQ(advisor->getTrustLevel(), 0.8f);

    advisor->setFactionAlignment(0.6f);
    EXPECT_EQ(advisor->getFactionAlignment(), 0.6f);
}

TEST_F(AdvisorDataStructure, AdvisorStrategyParameters) {
    advisor->setAgenda(Agenda::LONG_TERM);
    EXPECT_EQ(advisor->getAgenda(), Agenda::LONG_TERM);

    advisor->setRiskTolerance(0.7f);
    EXPECT_EQ(advisor->getRiskTolerance(), 0.7f);

    advisor->setStrategyStyle(StrategyStyle::PERSUASIVE);
    EXPECT_EQ(advisor->getStrategyStyle(), StrategyStyle::PERSUASIVE);
}

// ============================================================================
// Test Suite 5: Resource & Faction Data (Test 5.1 - 5.4)
// ============================================================================

class ResourceAndFactionData : public ::testing::Test {
protected:
    void SetUp() override {
        resource = std::make_shared<Resource>(30001);
        faction = std::make_shared<Faction>(20001);
    }

    std::shared_ptr<Resource> resource;
    std::shared_ptr<Faction> faction;
};

TEST_F(ResourceAndFactionData, ResourceAttributes) {
    resource->setName("Food");
    resource->setQuantity(500);
    resource->setProductionRate(20);
    resource->setConsumptionRate(15);
    resource->setScarcityThreshold(100);

    EXPECT_EQ(resource->getName(), "Food");
    EXPECT_EQ(resource->getQuantity(), 500);
    EXPECT_EQ(resource->getProductionRate(), 20);
    EXPECT_FALSE(resource->isScarse());

    resource->setQuantity(50);
    EXPECT_TRUE(resource->isScarse());
}

TEST_F(ResourceAndFactionData, FactionAttributes) {
    faction->setName("Merchants");
    faction->setStrength(0.75f);
    faction->setAlignment(Alignment::PLAYER_FRIENDLY);

    EXPECT_EQ(faction->getName(), "Merchants");
    EXPECT_EQ(faction->getStrength(), 0.75f);
    EXPECT_EQ(faction->getAlignment(), Alignment::PLAYER_FRIENDLY);
}

TEST_F(ResourceAndFactionData, FactionMembership) {
    faction->addMemberId(1);
    faction->addMemberId(2);
    faction->addMemberId(5);

    EXPECT_EQ(faction->getMemberCount(), static_cast<size_t>(3));
    const auto& memberIds = faction->getMemberIds();
    EXPECT_EQ(memberIds[0], 1);
    EXPECT_EQ(memberIds[2], 5);
}

TEST_F(ResourceAndFactionData, FactionLeadership) {
    faction->addLeaderId(10);
    faction->addLeaderId(15);

    const auto& leaders = faction->getLeaderIds();
    EXPECT_EQ(leaders.size(), static_cast<size_t>(2));
}

// ============================================================================
// Test Suite 6: NPC Registry (Test 6.1 - 6.5)
// ============================================================================

class NPCRegistryTests : public ::testing::Test {
protected:
    void SetUp() override {
        RegistryManager::getInstance().clearAll();
    }

    void TearDown() override {
        RegistryManager::getInstance().clearAll();
    }
};

TEST_F(NPCRegistryTests, RegisterAndRetrieveNPC) {
    auto& registry = NPCRegistry::getInstance();
    auto npc = std::make_shared<NPC>(1);
    npc->setName("Alice");

    registry.registerNPC(npc);

    EXPECT_TRUE(registry.hasNPC(1));
    auto retrieved = registry.getNPCById(1);
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "Alice");
}

TEST_F(NPCRegistryTests, NPCRegistryFactionFiltering) {
    auto& registry = NPCRegistry::getInstance();
    
    auto npc1 = std::make_shared<NPC>(1);
    npc1->setFactionId(10);
    auto npc2 = std::make_shared<NPC>(2);
    npc2->setFactionId(10);
    auto npc3 = std::make_shared<NPC>(3);
    npc3->setFactionId(20);

    registry.registerNPC(npc1);
    registry.registerNPC(npc2);
    registry.registerNPC(npc3);

    auto faction10NPCs = registry.getNPCsByFaction(10);
    EXPECT_EQ(faction10NPCs.size(), static_cast<size_t>(2));

    auto faction20NPCs = registry.getNPCsByFaction(20);
    EXPECT_EQ(faction20NPCs.size(), static_cast<size_t>(1));
}

TEST_F(NPCRegistryTests, NPCRegistryRoleFiltering) {
    auto& registry = NPCRegistry::getInstance();
    
    auto npc1 = std::make_shared<NPC>(1);
    npc1->setRole("farmer");
    auto npc2 = std::make_shared<NPC>(2);
    npc2->setRole("warrior");

    registry.registerNPC(npc1);
    registry.registerNPC(npc2);

    auto farmers = registry.getNPCsByRole("farmer");
    EXPECT_EQ(farmers.size(), static_cast<size_t>(1));
    EXPECT_EQ(farmers[0]->getId(), 1);
}

TEST_F(NPCRegistryTests, NPCRegistryUnregistration) {
    auto& registry = NPCRegistry::getInstance();
    auto npc = std::make_shared<NPC>(1);

    registry.registerNPC(npc);
    EXPECT_TRUE(registry.hasNPC(1));

    registry.unregisterNPC(1);
    EXPECT_FALSE(registry.hasNPC(1));
}

TEST_F(NPCRegistryTests, NPCRegistryCount) {
    auto& registry = NPCRegistry::getInstance();
    
    EXPECT_EQ(registry.getNPCCount(), static_cast<size_t>(0));

    for (int i = 1; i <= 10; i++) {
        auto npc = std::make_shared<NPC>(i);
        registry.registerNPC(npc);
    }

    EXPECT_EQ(registry.getNPCCount(), static_cast<size_t>(10));
}

// ============================================================================
// Test Suite 7: Faction & Resource Registries (Test 7.1 - 7.4)
// ============================================================================

class FactionAndResourceRegistries : public ::testing::Test {
protected:
    void SetUp() override {
        RegistryManager::getInstance().clearAll();
    }

    void TearDown() override {
        RegistryManager::getInstance().clearAll();
    }
};

TEST_F(FactionAndResourceRegistries, FactionRegistryBasicOps) {
    auto& registry = FactionRegistry::getInstance();
    auto faction = std::make_shared<Faction>(1);
    faction->setName("Merchants");

    registry.registerFaction(faction);
    EXPECT_TRUE(registry.hasFaction(1));
    
    auto retrieved = registry.getFactionById(1);
    EXPECT_EQ(retrieved->getName(), "Merchants");
}

TEST_F(FactionAndResourceRegistries, FactionRegistryAlignment) {
    auto& registry = FactionRegistry::getInstance();
    
    auto f1 = std::make_shared<Faction>(1);
    f1->setAlignment(Alignment::PLAYER_FRIENDLY);
    auto f2 = std::make_shared<Faction>(2);
    f2->setAlignment(Alignment::HOSTILE);

    registry.registerFaction(f1);
    registry.registerFaction(f2);

    auto friendly = registry.getFactionsByAlignment(Alignment::PLAYER_FRIENDLY);
    EXPECT_EQ(friendly.size(), static_cast<size_t>(1));
}

TEST_F(FactionAndResourceRegistries, ResourceRegistryByName) {
    auto& registry = ResourceRegistry::getInstance();
    
    auto food = std::make_shared<Resource>(1);
    food->setName("Food");
    auto wood = std::make_shared<Resource>(2);
    wood->setName("Wood");

    registry.registerResource(food);
    registry.registerResource(wood);

    EXPECT_TRUE(registry.hasResourceByName("Food"));
    auto retrieved = registry.getResourceByName("Food");
    EXPECT_EQ(retrieved->getId(), 1);
}

TEST_F(FactionAndResourceRegistries, ResourceRegistryScarcity) {
    auto& registry = ResourceRegistry::getInstance();
    
    auto food = std::make_shared<Resource>(1);
    food->setName("Food");
    food->setQuantity(50);
    food->setScarcityThreshold(100);

    registry.registerResource(food);

    auto scareResources = registry.getScareResources();
    EXPECT_EQ(scareResources.size(), static_cast<size_t>(1));
}

// ============================================================================
// Test Suite 8: Event System (Test 8.1 - 8.3)
// ============================================================================

class EventSystemTests : public ::testing::Test {
protected:
    void SetUp() override {
        RegistryManager::getInstance().clearAll();
    }

    void TearDown() override {
        RegistryManager::getInstance().clearAll();
    }
};

TEST_F(EventSystemTests, EventCreationAndAttributes) {
    auto event = std::make_shared<Event>(1);
    event->setName("Famine");
    event->setType(EventType::ENVIRONMENTAL);
    event->setImpactLevel(8);

    EXPECT_EQ(event->getName(), "Famine");
    EXPECT_EQ(event->getType(), EventType::ENVIRONMENTAL);
    EXPECT_EQ(event->getImpactLevel(), 8);
}

TEST_F(EventSystemTests, EventAffectedEntities) {
    auto event = std::make_shared<Event>(1);
    
    event->addAffectedNPCId(1);
    event->addAffectedNPCId(2);
    event->addAffectedNPCId(5);

    event->addAffectedResourceId(30001);

    EXPECT_EQ(event->getAffectedNPCIds().size(), static_cast<size_t>(3));
    EXPECT_EQ(event->getAffectedResourceIds().size(), static_cast<size_t>(1));
}

TEST_F(EventSystemTests, EventRegistryByType) {
    auto& registry = EventRegistry::getInstance();
    
    auto env_event = std::make_shared<Event>(1);
    env_event->setType(EventType::ENVIRONMENTAL);
    
    auto pol_event = std::make_shared<Event>(2);
    pol_event->setType(EventType::POLITICAL);

    registry.registerEvent(env_event);
    registry.registerEvent(pol_event);

    auto envEvents = registry.getEventsByType(EventType::ENVIRONMENTAL);
    EXPECT_EQ(envEvents.size(), static_cast<size_t>(1));
}

// ============================================================================
// Main Test Execution
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
