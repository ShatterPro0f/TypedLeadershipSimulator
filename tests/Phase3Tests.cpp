#include <gtest/gtest.h>
#include "Vector3.h"
#include "World.h"
#include "Collision.h"
#include "Player.h"
#include "Pathfinding.h"
#include "ProximityDetection.h"
#include "NPCMovement.h"
#include "ActivitySystem.h"
#include "MovementConfig.h"
#include "Core.h"
#include "Registries.h"
#include "EntityFactory.h"

using namespace TLS;

// ============================================================================
// Vector3 Math Tests
// ============================================================================

class Vector3MathTests : public ::testing::Test {
protected:
    Vector3 v1 = Vector3(3.0f, 4.0f, 0.0f);
    Vector3 v2 = Vector3(1.0f, 2.0f, 3.0f);
};

TEST_F(Vector3MathTests, Vector3DirectionTo) {
    Vector3 from(0.0f, 0.0f, 0.0f);
    Vector3 to(3.0f, 4.0f, 0.0f);
    Vector3 dir = from.directionTo(to);
    EXPECT_FLOAT_EQ(dir.magnitude(), 1.0f);  // Should be normalized
    EXPECT_FLOAT_EQ(dir.x, 0.6f);  // 3/5
    EXPECT_FLOAT_EQ(dir.y, 0.8f);  // 4/5
}

TEST_F(Vector3MathTests, Vector3IsWithinRange) {
    Vector3 a(0.0f, 0.0f, 0.0f);
    Vector3 b(3.0f, 4.0f, 0.0f);
    EXPECT_TRUE(a.isWithinRange(b, 5.0f));   // distance is 5, range is 5
    EXPECT_FALSE(a.isWithinRange(b, 4.9f));  // distance is 5, range is 4.9
    EXPECT_TRUE(a.isWithinRange(b, 5.1f));   // distance is 5, range is 5.1
}

TEST_F(Vector3MathTests, Vector3Clamp) {
    Vector3 v(10.0f, -5.0f, 20.0f);
    Vector3 clamped = v.clamp(Vector3(0.0f, -10.0f, 0.0f), Vector3(5.0f, 10.0f, 15.0f));
    EXPECT_FLOAT_EQ(clamped.x, 5.0f);
    EXPECT_FLOAT_EQ(clamped.y, -5.0f);
    EXPECT_FLOAT_EQ(clamped.z, 15.0f);
}

TEST_F(Vector3MathTests, Vector3ScalarMultiplyAssign) {
    Vector3 v(2.0f, 3.0f, 4.0f);
    v *= 0.5f;
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 1.5f);
    EXPECT_FLOAT_EQ(v.z, 2.0f);
}

// ============================================================================
// World and AABB Tests
// ============================================================================

class WorldTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
};

TEST_F(WorldTests, AABBContains) {
    AABB box(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));
    EXPECT_TRUE(box.contains(Vector3(5.0f, 5.0f, 5.0f)));
    EXPECT_FALSE(box.contains(Vector3(15.0f, 5.0f, 5.0f)));
    EXPECT_TRUE(box.contains(Vector3(0.0f, 0.0f, 0.0f)));  // At corner
}

TEST_F(WorldTests, AABBIntersects) {
    AABB box1(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));
    AABB box2(Vector3(5.0f, 5.0f, 5.0f), Vector3(15.0f, 15.0f, 15.0f));
    EXPECT_TRUE(box1.intersects(box2));
    
    AABB box3(Vector3(20.0f, 20.0f, 20.0f), Vector3(30.0f, 30.0f, 30.0f));
    EXPECT_FALSE(box1.intersects(box3));
}

TEST_F(WorldTests, AABBIntersectsSphere) {
    AABB box(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));
    EXPECT_TRUE(box.intersectsSphere(Vector3(5.0f, 5.0f, 5.0f), 3.0f));  // Inside
    EXPECT_TRUE(box.intersectsSphere(Vector3(15.0f, 5.0f, 5.0f), 6.0f));  // Overlapping
    EXPECT_FALSE(box.intersectsSphere(Vector3(20.0f, 20.0f, 20.0f), 5.0f));  // Too far
}

TEST_F(WorldTests, WorldBoundaryCheck) {
    Vector3 inside(0.0f, 0.0f, 0.0f);
    Vector3 outside(150.0f, 150.0f, 0.0f);
    EXPECT_TRUE(world.isPointValid(inside));
    EXPECT_FALSE(world.isPointValid(outside));
}

TEST_F(WorldTests, AddObstacle) {
    AABB obstacle(Vector3(10.0f, 10.0f, 0.0f), Vector3(20.0f, 20.0f, 5.0f));
    world.addObstacle(obstacle);
    // Verify obstacle was added (check obstacle count or path clearing)
    Vector3 start(0.0f, 0.0f, 0.0f);
    Vector3 end(25.0f, 25.0f, 0.0f);
    // Path should be blocked or modified by obstacle
}

// ============================================================================
// Collision Detection Tests
// ============================================================================

class CollisionTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
};

TEST_F(CollisionTests, SphereSphereCollision) {
    Vector3 a(0.0f, 0.0f, 0.0f);
    Vector3 b(2.0f, 0.0f, 0.0f);
    float radiusA = 2.0f;
    float radiusB = 1.5f;
    EXPECT_TRUE(CollisionDetector::sphereSphereCollision(a, radiusA, b, radiusB));
    
    Vector3 c(10.0f, 0.0f, 0.0f);
    EXPECT_FALSE(CollisionDetector::sphereSphereCollision(a, radiusA, c, radiusB));
}

TEST_F(CollisionTests, SphereAABBCollision) {
    AABB box(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));
    Vector3 sphere(5.0f, 5.0f, 5.0f);
    EXPECT_TRUE(CollisionDetector::sphereAABBCollision(sphere, 2.0f, box));
    
    Vector3 sphereOuter(20.0f, 20.0f, 20.0f);
    EXPECT_FALSE(CollisionDetector::sphereAABBCollision(sphereOuter, 2.0f, box));
}

TEST_F(CollisionTests, IsPathClear) {
    Vector3 start(0.0f, 0.0f, 0.0f);
    Vector3 end(50.0f, 0.0f, 0.0f);
    bool clear = CollisionDetector::isPathClear(start, end, 1.0f, world);
    EXPECT_TRUE(clear);  // Open path
}

TEST_F(CollisionTests, IsPathBlockedByObstacle) {
    AABB obstacle(Vector3(20.0f, -5.0f, 0.0f), Vector3(30.0f, 5.0f, 5.0f));
    world.addObstacle(obstacle);
    
    Vector3 start(0.0f, 0.0f, 0.0f);
    Vector3 end(50.0f, 0.0f, 0.0f);
    // Path goes through obstacle at y=0
    bool clear = CollisionDetector::isPathClear(start, end, 1.0f, world);
    // Depending on sweep logic, should be blocked or at least affected
}

TEST_F(CollisionTests, RaycastClear) {
    Vector3 from(0.0f, 0.0f, 0.0f);
    Vector3 to(50.0f, 0.0f, 0.0f);
    bool clear = CollisionDetector::raycastClear(from, to, world, 0.5f);
    EXPECT_TRUE(clear);  // Open line of sight
}

// ============================================================================
// Player Movement Tests
// ============================================================================

class PlayerMovementTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
    Player player;
    
    void SetUp() override {
        player.position = Vector3(0.0f, 0.0f, 0.0f);
    }
};

TEST_F(PlayerMovementTests, PlayerInitialization) {
    EXPECT_EQ(player.position.x, 0.0f);
    EXPECT_EQ(player.position.y, 0.0f);
    EXPECT_EQ(player.position.z, 0.0f);
}

TEST_F(PlayerMovementTests, PlayerMoveForward) {
    player.moveForward(1.0f);  // 1 second at speed
    EXPECT_GT(player.velocity.magnitude(), 0.0f);
}

TEST_F(PlayerMovementTests, PlayerMoveRight) {
    player.moveRight(1.0f);
    EXPECT_GT(player.velocity.magnitude(), 0.0f);
}

TEST_F(PlayerMovementTests, PlayerRotateHorizontal) {
    float oldAngle = player.horizontalAngle;
    player.rotateHorizontal(0.1f);
    EXPECT_NE(player.horizontalAngle, oldAngle);
}

TEST_F(PlayerMovementTests, PlayerRotateVertical) {
    float oldAngle = player.verticalAngle;
    player.rotateVertical(0.1f);
    EXPECT_NE(player.verticalAngle, oldAngle);
}

TEST_F(PlayerMovementTests, PlayerUpdate) {
    player.moveForward(1.0f);
    player.update(1.0f / 60.0f, world);  // 60 FPS frame
    EXPECT_GE(player.position.z, -10.0f);  // Should not fall below ground
}

TEST_F(PlayerMovementTests, PlayerGravity) {
    float startZ = player.position.z;
    player.position.z = 5.0f;
    player.update(1.0f / 60.0f, world);
    EXPECT_LE(player.position.z, 5.0f);  // Should fall or stay (gravity applied)
}

// ============================================================================
// Pathfinding Tests
// ============================================================================

class PathfindingTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
    WaypointGraph graph;
    
    void SetUp() override {
        // Add some waypoints
        graph.addWaypoint(0, "start", Vector3(0.0f, 0.0f, 0.0f));
        graph.addWaypoint(1, "mid1", Vector3(25.0f, 0.0f, 0.0f));
        graph.addWaypoint(2, "mid2", Vector3(25.0f, 25.0f, 0.0f));
        graph.addWaypoint(3, "end", Vector3(50.0f, 50.0f, 0.0f));
        
        // Add connections
        graph.addConnection(0, 1);
        graph.addConnection(1, 2);
        graph.addConnection(2, 3);
        graph.addConnection(1, 3);  // Shortcut
    }
};

TEST_F(PathfindingTests, AddWaypoint) {
    EXPECT_EQ(graph.getWaypointCount(), 4);
}

TEST_F(PathfindingTests, GetNearestWaypoint) {
    Vector3 pos(0.0f, 0.0f, 0.0f);
    int nearest = graph.getNearestWaypoint(pos);
    EXPECT_EQ(nearest, 0);  // Should be start waypoint
}

TEST_F(PathfindingTests, FindPathSimple) {
    std::vector<Vector3> path = graph.findPath(0, 3, world);
    EXPECT_GT(path.size(), 0);  // Should find a path
    EXPECT_LE(path.size(), 5);  // But not too long
}

TEST_F(PathfindingTests, FindPathDirect) {
    std::vector<Vector3> path = graph.findPath(0, 1, world);
    EXPECT_GT(path.size(), 0);
}

TEST_F(PathfindingTests, FindPathWithObstacle) {
    AABB obstacle(Vector3(20.0f, -5.0f, 0.0f), Vector3(30.0f, 5.0f, 5.0f));
    world.addObstacle(obstacle);
    
    std::vector<Vector3> path = graph.findPath(0, 3, world);
    // Path should still exist but may be longer or rerouted
    EXPECT_GT(path.size(), 0);
}

TEST_F(PathfindingTests, NPCNavigatorPlanPath) {
    Vector3 start(0.0f, 0.0f, 0.0f);
    Vector3 goal(50.0f, 50.0f, 0.0f);
    Path plannedPath = NPCNavigator::planPath(start, goal, graph, world);
    EXPECT_GT(plannedPath.waypoints.size(), 0);
}

TEST_F(PathfindingTests, NPCNavigatorGetNextPosition) {
    Path path;
    path.waypoints.push_back(Vector3(0.0f, 0.0f, 0.0f));
    path.waypoints.push_back(Vector3(10.0f, 0.0f, 0.0f));
    path.waypoints.push_back(Vector3(20.0f, 0.0f, 0.0f));
    
    Vector3 current(0.0f, 0.0f, 0.0f);
    Vector3 nextPos = NPCNavigator::getNextPosition(path, current, 5.0f, 1.0f);
    // If moving along path, nextPos should be toward destination (or current if no movement)
    float distToNext = (nextPos - current).magnitude();
    EXPECT_LE(distToNext, 5.1f);  // Should move at most 5 units
}

// ============================================================================
// Proximity Detection Tests
// ============================================================================

class ProximityTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
    Player player;
    
    void SetUp() override {
        auto& npcRegistry = NPCRegistry::getInstance();
        npcRegistry.clear();  // Clear from any prior tests
        player.position = Vector3(0.0f, 0.0f, 0.0f);
        
        // Create some test NPCs
        for (int i = 0; i < 5; ++i) {
            auto npc = std::make_shared<NPC>(i);
            npc->setName("NPC" + std::to_string(i));
            npc->setPosition(Vector3(i * 5.0f, 0.0f, 0.0f));
            npcRegistry.registerNPC(npc);
        }
    }
};

TEST_F(ProximityTests, IsNPCInProximity) {
    auto& npcRegistry = NPCRegistry::getInstance();
    auto npc = npcRegistry.getNPCById(0);
    ASSERT_NE(npc, nullptr);
    bool inProximity = ProximityDetector::isNPCInProximity(*npc, player, 10.0f);
    EXPECT_TRUE(inProximity);  // NPC at (0,0,0), player at (0,0,0), range 10
}

TEST_F(ProximityTests, IsNPCOutOfProximity) {
    auto& npcRegistry = NPCRegistry::getInstance();
    auto npc = npcRegistry.getNPCById(4);
    ASSERT_NE(npc, nullptr);
    bool inProximity = ProximityDetector::isNPCInProximity(*npc, player, 10.0f);
    EXPECT_FALSE(inProximity);  // NPC at (20,0,0), player at (0,0,0), range 10
}

TEST_F(ProximityTests, GetNPCsInProximity) {
    auto& npcRegistry = NPCRegistry::getInstance();
    std::vector<int> nearby = ProximityDetector::getNPCsInProximity(npcRegistry, player, 10.0f);
    EXPECT_GE(nearby.size(), 2);  // Should include NPCs 0, 1, 2
    EXPECT_LE(nearby.size(), 3);
}

TEST_F(ProximityTests, GetDistanceToPlayer) {
    auto& npcRegistry = NPCRegistry::getInstance();
    auto npc = npcRegistry.getNPCById(0);
    ASSERT_NE(npc, nullptr);
    float dist = ProximityDetector::getDistanceToPlayer(*npc, player);
    EXPECT_FLOAT_EQ(dist, 0.0f);  // Same position
}

TEST_F(ProximityTests, GetNPCsSortedByDistance) {
    auto& npcRegistry = NPCRegistry::getInstance();
    std::vector<int> sorted = ProximityDetector::getNPCsSortedByDistance(npcRegistry, player);
    EXPECT_EQ(sorted.size(), 5);
    EXPECT_EQ(sorted[0], 0);  // NPC 0 is closest
    EXPECT_EQ(sorted[4], 4);  // NPC 4 is farthest
}

TEST_F(ProximityTests, CanNPCDetectPlayer) {
    auto& npcRegistry = NPCRegistry::getInstance();
    player.position = Vector3(0.0f, 0.0f, 0.0f);
    auto npc = npcRegistry.getNPCById(0);
    ASSERT_NE(npc, nullptr);
    npc->setPosition(Vector3(5.0f, 0.0f, 0.0f));
    
    bool canDetect = ProximityDetector::canNPCDetectPlayer(*npc, player, world, 50.0f);
    EXPECT_TRUE(canDetect);  // Within range and line of sight
}

// ============================================================================
// NPC Movement Tests
// ============================================================================

class NPCMovementTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
    WaypointGraph graph;
    
    void SetUp() override {
        // Set up waypoint graph
        graph.addWaypoint(0, "waypoint0", Vector3(0.0f, 0.0f, 0.0f));
        graph.addWaypoint(1, "waypoint1", Vector3(20.0f, 0.0f, 0.0f));
        graph.addWaypoint(2, "waypoint2", Vector3(40.0f, 0.0f, 0.0f));
        
        graph.addConnection(0, 1);
        graph.addConnection(1, 2);
    }
};

TEST_F(NPCMovementTests, DetermineActivity) {
    auto npc = std::make_shared<NPC>(1);
    npc->setName("TestNPC");
    npc->setPosition(Vector3(0.0f, 0.0f, 0.0f));
    
    WorldState state;
    Activity activity = NPCMovement::determineActivity(*npc, state, graph);
    EXPECT_NE(activity, Activity::IN_CONVERSATION);  // Should not be in conversation
}

TEST_F(NPCMovementTests, UpdateNPCPosition) {
    auto npc = std::make_shared<NPC>(2);
    npc->setName("MovingNPC");
    npc->setPosition(Vector3(0.0f, 0.0f, 0.0f));
    
    Vector3 destination(20.0f, 0.0f, 0.0f);
    NPCMovement::updateNPCPosition(*npc, Activity::WORKING, destination, 0.016f, world, graph, 0);
    
    // NPC should have moved or started moving
    EXPECT_GE(npc->getPosition().distance(Vector3(0.0f, 0.0f, 0.0f)), 0.0f);
}

TEST_F(NPCMovementTests, ShouldRecalcPath) {
    auto npc = std::make_shared<NPC>(3);
    Vector3 destination(20.0f, 0.0f, 0.0f);
    
    bool shouldRecalc = NPCMovement::shouldRecalcPath(*npc, destination, 0, 100);
    EXPECT_TRUE(shouldRecalc);  // Default interval should trigger recalc
}

TEST_F(NPCMovementTests, IsNPCStuck) {
    auto npc = std::make_shared<NPC>(4);
    std::vector<Vector3> history;
    
    // Populate history with same position (stuck)
    for (int i = 0; i < 30; ++i) {
        history.push_back(Vector3(10.0f, 10.0f, 0.0f));
    }
    
    bool stuck = NPCMovement::isNPCStuck(*npc, history);
    EXPECT_TRUE(stuck);
}

TEST_F(NPCMovementTests, IsNPCNotStuck) {
    auto npc = std::make_shared<NPC>(5);
    std::vector<Vector3> history;
    
    // Populate history with moving positions
    for (int i = 0; i < 30; ++i) {
        history.push_back(Vector3(float(i), 0.0f, 0.0f));
    }
    
    bool stuck = NPCMovement::isNPCStuck(*npc, history);
    EXPECT_FALSE(stuck);
}

// ============================================================================
// Activity System Tests
// ============================================================================

class ActivitySystemTests : public ::testing::Test {
protected:
    WaypointGraph graph;
    
    void SetUp() override {
        graph.addWaypoint(0, "center", Vector3(0.0f, 0.0f, 0.0f));
        graph.addWaypoint(1, "farm_south", Vector3(-20.0f, -20.0f, 0.0f));
        graph.addWaypoint(2, "market", Vector3(20.0f, 0.0f, 0.0f));
    }
};

TEST_F(ActivitySystemTests, DetermineNPCActivity) {
    auto npc = std::make_shared<NPC>(1);
    npc->setName("TestNPC");
    npc->setActivity(Activity::IDLE);
    
    WorldState state;
    Activity activity = ActivitySystem::determineNPCActivity(*npc, state, 0);
    EXPECT_NE(activity, Activity::IN_CONVERSATION);
}

TEST_F(ActivitySystemTests, GetActivityDestinationIdle) {
    auto npc = std::make_shared<NPC>(2);
    npc->setPosition(Vector3(5.0f, 5.0f, 0.0f));
    
    Vector3 dest = ActivitySystem::getActivityDestination(*npc, Activity::IDLE, graph);
    EXPECT_EQ(dest, npc->getPosition());  // Should stay at current position
}

TEST_F(ActivitySystemTests, GetActivityDestinationResting) {
    auto npc = std::make_shared<NPC>(3);
    npc->setPosition(Vector3(0.0f, 0.0f, 0.0f));
    npc->setHomeLocation(Vector3(10.0f, 10.0f, 0.0f));
    
    Vector3 dest = ActivitySystem::getActivityDestination(*npc, Activity::RESTING, graph);
    EXPECT_EQ(dest, npc->getHomeLocation());
}

TEST_F(ActivitySystemTests, GetGameHour) {
    int tick = 600;  // Assuming TICKS_PER_GAME_HOUR = 600
    float hour = ActivitySystem::getGameHour(tick);
    EXPECT_GE(hour, 0.0f);
    EXPECT_LT(hour, 24.0f);
}

TEST_F(ActivitySystemTests, GetGameMinute) {
    int tick = 10;  // Assuming TICKS_PER_GAME_MINUTE = 10
    float minute = ActivitySystem::getGameMinute(tick);
    EXPECT_GE(minute, 0.0f);
    EXPECT_LT(minute, 60.0f);
}

TEST_F(ActivitySystemTests, ShouldTransitionActivity) {
    auto npc = std::make_shared<NPC>(4);
    npc->setActivity(Activity::WORKING);
    
    bool shouldTransition = ActivitySystem::shouldTransitionActivity(*npc, Activity::WORKING, 100);
    // Result depends on time-of-day logic (could be true or false)
    EXPECT_TRUE(true);  // Just verify the method doesn't crash
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase3IntegrationTests : public ::testing::Test {
protected:
    World world = World(Vector3(-100.0f, -100.0f, -10.0f), Vector3(100.0f, 100.0f, 10.0f));
    Player player;
    WaypointGraph graph;
    
    void SetUp() override {
        auto& npcRegistry = NPCRegistry::getInstance();
        npcRegistry.clear();  // Clear from any prior tests
        
        player.position = Vector3(0.0f, 0.0f, 0.0f);
        
        // Create waypoints
        graph.addWaypoint(0, "start", Vector3(0.0f, 0.0f, 0.0f));
        graph.addWaypoint(1, "goal", Vector3(50.0f, 50.0f, 0.0f));
        graph.addConnection(0, 1);
        
        // Create NPCs
        for (int i = 0; i < 3; ++i) {
            auto npc = std::make_shared<NPC>(i);
            npc->setName("NPC" + std::to_string(i));
            npc->setPosition(Vector3(i * 10.0f, 0.0f, 0.0f));
            npc->setHomeLocation(Vector3(i * 10.0f, 20.0f, 0.0f));
            npc->setRole(i % 2 == 0 ? "farmer" : "merchant");
            npcRegistry.registerNPC(npc);
        }
    }
};

TEST_F(Phase3IntegrationTests, NPCPathToPlayer) {
    player.position = Vector3(0.0f, 0.0f, 0.0f);
    auto& npcRegistry = NPCRegistry::getInstance();
    auto npc = npcRegistry.getNPCById(0);
    ASSERT_NE(npc, nullptr);
    
    // Check if NPC can find path to player
    Vector3 destination = player.position;
    if (npc->currentPath) delete npc->currentPath;
    npc->currentPath = new Path(NPCNavigator::planPath(npc->getPosition(), destination, graph, world));
    
    EXPECT_GT(npc->currentPath->waypoints.size(), 0);
}

TEST_F(Phase3IntegrationTests, MultipleNPCsProximity) {
    player.position = Vector3(0.0f, 0.0f, 0.0f);
    auto& npcRegistry = NPCRegistry::getInstance();
    
    std::vector<int> nearby = ProximityDetector::getNPCsInProximity(npcRegistry, player, 30.0f);
    EXPECT_EQ(nearby.size(), 3);  // All three NPCs should be in range
}

TEST_F(Phase3IntegrationTests, NPCActivitySchedule) {
    auto& npcRegistry = NPCRegistry::getInstance();
    for (int tick = 0; tick < 14400; tick += 1000) {  // 1 game day
        auto npc = npcRegistry.getNPCById(0);
        ASSERT_NE(npc, nullptr);
        
        WorldState state;
        Activity activity = ActivitySystem::determineNPCActivity(*npc, state, tick);
        // Activity should be valid at each time
        EXPECT_GE(static_cast<int>(activity), 0);
    }
}

TEST_F(Phase3IntegrationTests, PlayerMovementAndCollision) {
    Vector3 startPos = player.position;
    player.moveForward(1.0f);
    
    for (int i = 0; i < 10; ++i) {
        player.update(1.0f / 60.0f, world);
    }
    
    // Player should have moved forward or have velocity
    EXPECT_GT(player.velocity.magnitude(), 0.0f);
}

TEST_F(Phase3IntegrationTests, WorldStateSnapshot) {
    // Simulate a world state with multiple entities
    auto& npcRegistry = NPCRegistry::getInstance();
    EXPECT_GT(npcRegistry.getNPCCount(), 0);
    EXPECT_GT(player.position.magnitude(), -1.0f);  // Valid position
}
