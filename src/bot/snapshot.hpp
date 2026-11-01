#pragma once

#include <Geode/Geode.hpp>

#include <string>

// Snapshot of the parts of a PlayerObject that decide where the player is and
// what it is doing. Used by the practice fix to restore a checkpoint exactly.
struct PlayerSnapshot {
    cocos2d::CCPoint position;
    float rotation = 0.f;

    double yVelocity = 0.0;
    double fallSpeed = 0.0;
    double gravity = 0.0;
    float gravityMod = 1.f;

    double dashX = 0.0;
    double dashY = 0.0;
    double dashAngle = 0.0;
    double dashStartTime = 0.0;

    double slopeVelocity = 0.0;
    double currentSlopeYVelocity = 0.0;
    double slopeRotation = 0.0;

    double accelerationOrSpeed = 0.0;
    double snapDistance = 0.0;
    float rotationSpeed = 0.f;
    float playerSpeed = 1.f;
    float vehicleSize = 1.f;

    double lastFlipTime = 0.0;
    double lastSpiderFlipTime = 0.0;
    double lastLandTime = 0.0;
    double gameModeChangedTime = 0.0;

    int playerStreak = 0;
    int stateOnGround = 0;
    int stateBoostX = 0;
    int stateBoostY = 0;
    int stateScale = 0;

    bool holdingLeft = false;
    bool holdingRight = false;

    bool isShip = false;
    bool isBird = false;
    bool isBall = false;
    bool isDart = false;
    bool isRobot = false;
    bool isSpider = false;
    bool isSwing = false;
    bool isUpsideDown = false;
    bool isDashing = false;
    bool isDead = false;
    bool isOnGround = false;
    bool isOnGround2 = false;
    bool isOnGround3 = false;
    bool isOnGround4 = false;
    bool isGoingLeft = false;
    bool isSliding = false;
    bool isOnSlope = false;
    bool wasOnSlope = false;
    bool isAccelerating = false;
    bool isRotating = false;
    bool isBallRotating = false;
    bool isMoving = false;
    bool isHidden = false;
    bool isLocked = false;
    bool hasGlow = false;
    bool isBeingSpawnedByDualPortal = false;
    bool maybeIsFalling = false;
    bool maybeHasStopped = false;
    bool jumpBuffered = false;
    bool wasJumpBuffered = false;
    bool wasRobotJump = false;
    bool stateRingJump = false;
    bool stateRingJump2 = false;
    bool fixRobotJump = false;
    bool controlsDisabled = false;
    bool disablePlayerSqueeze = false;
    bool hasEverJumped = false;
    bool wasTeleported = false;
    bool decreaseBoostSlide = false;
    bool isOnIce = false;
    bool padRingRelated = false;
    bool ringJumpRelated = false;
    bool touchedRing = false;

    static PlayerSnapshot capture(PlayerObject* player);
    void apply(PlayerObject* player) const;
};

// Frames at which the recorded run passed a checkpoint. Playback uses these to
// know resuming from practice is legitimate and to repair the input stream.
struct CheckpointMarker {
    unsigned int frame = 0;
    unsigned int previousFrame = 0;
    PlayerSnapshot player1;
    PlayerSnapshot player2;
    unsigned int seed = 0;
};
