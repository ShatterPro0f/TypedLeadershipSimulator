#ifndef PLAYER_H
#define PLAYER_H

#include "Vector3.h"
#include "World.h"
#include <string>
#include <fstream>

namespace TLS {

// Player character controlled by the user
class Player {
public:
    // Identity
    std::string name;

    // Position and movement
    Vector3 position;
    Vector3 velocity;
    float moveSpeed;      // units per tick
    float sprintSpeed;    // faster movement while sprinting

    // Orientation / camera
    Vector3 forward;      // direction player is facing (normalized)
    Vector3 up;           // up vector (typically 0,0,1)
    Vector3 right;        // right vector (forward cross up)
    float verticalAngle;  // pitch: rotation around right axis
    float horizontalAngle; // yaw: rotation around up axis
    float fov;            // field of view in degrees

    // Collision
    float collisionRadius;
    float eyeHeight;      // height of camera above feet

    // Constructors
    Player();
    Player(const std::string& name, const Vector3& startPosition);

    // Movement input (-1 to 1)
    void moveForward(float amount);
    void moveRight(float amount);
    void moveUp(float amount);

    // Rotation input (in radians)
    void rotateHorizontal(float deltaAngle);
    void rotateVertical(float deltaAngle);

    // Physics update
    void update(float deltaTime, const World& world);

    // Queries
    Vector3 getEyePosition() const;
    bool canReach(const Vector3& target, float reachDistance) const;
    float getDistanceTo(const Vector3& target) const;

    // Update facing direction based on angles
    void updateOrientation();

    // Serialization
    bool toBinary(std::ofstream& file) const;
    static Player fromBinary(std::ifstream& file);
};

}  // namespace TLS

#endif  // PLAYER_H
