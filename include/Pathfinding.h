#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "Vector3.h"
#include "World.h"
#include <vector>
#include <map>
#include <string>
#include <optional>

namespace TLS {

// Waypoint graph for navigation
class WaypointGraph {
public:
    struct Waypoint {
        int id;
        std::string name;
        Vector3 position;
    };

private:
    std::vector<Waypoint> waypoints;
    std::vector<std::vector<int>> adjacency;  // adjacency[i] = list of waypoint IDs connected to waypoint i
    std::map<std::string, int> locationToWaypoint;  // "farm" -> waypoint ID

public:
    WaypointGraph();

    // Load waypoints from JSON file
    bool loadFromFile(const std::string& filename);

    // Find nearest waypoint to a position
    int getNearestWaypoint(const Vector3& position) const;

    // Find path between two waypoints using A*
    std::vector<Vector3> findPath(
        int startWaypoint,
        int endWaypoint,
        const World& world
    ) const;

    // Get waypoint by name
    std::optional<Vector3> getWaypoint(const std::string& name) const;

    // Get waypoint by ID
    std::optional<Vector3> getWaypointById(int id) const;

    // Add waypoint
    void addWaypoint(int id, const std::string& name, const Vector3& position);

    // Add connection
    void addConnection(int from, int to);

    // Accessors
    size_t getWaypointCount() const { return waypoints.size(); }
    const std::vector<Waypoint>& getWaypoints() const { return waypoints; }

private:
    // A* search on waypoint graph
    std::vector<int> astarSearch(int start, int goal) const;

    // Heuristic: Euclidean distance between waypoints
    float heuristic(int a, int b) const;
};

// Navigation state for following a path
struct Path {
    std::vector<Vector3> waypoints;
    size_t currentWaypointIndex;
    float timeElapsed;
    bool isComplete;

    Path() : currentWaypointIndex(0), timeElapsed(0.0f), isComplete(false) {}
    Path(const std::vector<Vector3>& wp) 
        : waypoints(wp), currentWaypointIndex(0), timeElapsed(0.0f), isComplete(false) {}
};

// NPC navigation helper
class NPCNavigator {
public:
    // Plan a path from one location to another
    static Path planPath(
        const Vector3& from,
        const Vector3& to,
        const WaypointGraph& graph,
        const World& world
    );

    // Get next position along path
    static Vector3 getNextPosition(
        Path& path,
        const Vector3& currentPosition,
        float moveSpeed,
        float deltaTime
    );

    // Check if path is still valid
    static bool isPathValid(const Path& path, const World& world);

    // Get current waypoint on path
    static std::optional<Vector3> getCurrentWaypoint(const Path& path);

private:
    static constexpr float WAYPOINT_ARRIVAL_DISTANCE = 2.0f;
};

}  // namespace TLS

#endif  // PATHFINDING_H
