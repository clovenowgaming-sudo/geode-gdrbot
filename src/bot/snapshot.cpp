#include "snapshot.hpp"

using namespace geode::prelude;

PlayerSnapshot PlayerSnapshot::capture(PlayerObject* player) {
    PlayerSnapshot s;
    if (!player) return s;

    s.position = player->getPosition();
    s.rotation = player->getRotation();

    s.yVelocity = player->m_yVelocity;
    s.fallSpeed = player->m_fallSpeed;
    s.gravity = player->m_gravity;
    s.gravityMod = player->m_gravityMod;

    s.dashX = player->m_dashX;
    s.dashY = player->m_dashY;
    s.dashAngle = player->m_dashAngle;
    s.dashStartTime = player->m_dashStartTime;

    s.slopeVelocity = player->m_slopeVelocity;
    s.currentSlopeYVelocity = player->m_currentSlopeYVelocity;
    s.slopeRotation = player->m_slopeRotation;

    s.accelerationOrSpeed = player->m_accelerationOrSpeed;
    s.snapDistance = player->m_snapDistance;
    s.rotationSpeed = player->m_rotationSpeed;
    s.playerSpeed = player->m_playerSpeed;
    s.vehicleSize = player->m_vehicleSize;

    s.lastFlipTime = player->m_lastFlipTime;
    s.lastSpiderFlipTime = player->m_lastSpiderFlipTime;
    s.lastLandTime = player->m_lastLandTime;
    s.gameModeChangedTime = player->m_gameModeChangedTime;

    s.playerStreak = player->m_playerStreak;
    s.stateOnGround = player->m_stateOnGround;
    s.stateBoostX = player->m_stateBoostX;
    s.stateBoostY = player->m_stateBoostY;
    s.stateScale = player->m_stateScale;

    s.holdingLeft = player->m_holdingLeft;
    s.holdingRight = player->m_holdingRight;

    s.isShip = player->m_isShip;
    s.isBird = player->m_isBird;
    s.isBall = player->m_isBall;
    s.isDart = player->m_isDart;
    s.isRobot = player->m_isRobot;
    s.isSpider = player->m_isSpider;
    s.isSwing = player->m_isSwing;
    s.isUpsideDown = player->m_isUpsideDown;
    s.isDashing = player->m_isDashing;
    s.isDead = player->m_isDead;
    s.isOnGround = player->m_isOnGround;
    s.isOnGround2 = player->m_isOnGround2;
    s.isOnGround3 = player->m_isOnGround3;
    s.isOnGround4 = player->m_isOnGround4;
    s.isGoingLeft = player->m_isGoingLeft;
    s.isSliding = player->m_isSliding;
    s.isOnSlope = player->m_isOnSlope;
    s.wasOnSlope = player->m_wasOnSlope;
    s.isAccelerating = player->m_isAccelerating;
    s.isRotating = player->m_isRotating;
    s.isBallRotating = player->m_isBallRotating;
    s.isMoving = player->m_isMoving;
    s.isHidden = player->m_isHidden;
    s.isLocked = player->m_isLocked;
    s.hasGlow = player->m_hasGlow;
    s.isBeingSpawnedByDualPortal = player->m_isBeingSpawnedByDualPortal;
    s.maybeIsFalling = player->m_maybeIsFalling;
    s.maybeHasStopped = player->m_maybeHasStopped;
    s.jumpBuffered = player->m_jumpBuffered;
    s.wasJumpBuffered = player->m_wasJumpBuffered;
    s.wasRobotJump = player->m_wasRobotJump;
    s.stateRingJump = player->m_stateRingJump;
    s.stateRingJump2 = player->m_stateRingJump2;
    s.fixRobotJump = player->m_fixRobotJump;
    s.controlsDisabled = player->m_controlsDisabled;
    s.disablePlayerSqueeze = player->m_disablePlayerSqueeze;
    s.hasEverJumped = player->m_hasEverJumped;
    s.wasTeleported = player->m_wasTeleported;
    s.decreaseBoostSlide = player->m_decreaseBoostSlide;
    s.isOnIce = player->m_isOnIce;
    s.padRingRelated = player->m_padRingRelated;
    s.ringJumpRelated = player->m_ringJumpRelated;
    s.touchedRing = player->m_touchedRing;

    return s;
}

void PlayerSnapshot::apply(PlayerObject* player) const {
    if (!player) return;

    player->setPosition(position);
    player->setRotation(rotation);

    player->m_yVelocity = yVelocity;
    player->m_fallSpeed = fallSpeed;
    player->m_gravity = gravity;
    player->m_gravityMod = gravityMod;

    player->m_dashX = dashX;
    player->m_dashY = dashY;
    player->m_dashAngle = dashAngle;
    player->m_dashStartTime = dashStartTime;

    player->m_slopeVelocity = static_cast<float>(slopeVelocity);
    player->m_currentSlopeYVelocity = currentSlopeYVelocity;
    player->m_slopeRotation = slopeRotation;

    player->m_accelerationOrSpeed = accelerationOrSpeed;
    player->m_snapDistance = snapDistance;
    player->m_rotationSpeed = rotationSpeed;
    player->m_playerSpeed = playerSpeed;
    player->m_vehicleSize = vehicleSize;

    player->m_lastFlipTime = lastFlipTime;
    player->m_lastSpiderFlipTime = lastSpiderFlipTime;
    player->m_lastLandTime = lastLandTime;
    player->m_gameModeChangedTime = gameModeChangedTime;

    player->m_playerStreak = playerStreak;
    player->m_stateOnGround = stateOnGround;
    player->m_stateBoostX = stateBoostX;
    player->m_stateBoostY = stateBoostY;
    player->m_stateScale = stateScale;

    player->m_holdingLeft = holdingLeft;
    player->m_holdingRight = holdingRight;

    player->m_isShip = isShip;
    player->m_isBird = isBird;
    player->m_isBall = isBall;
    player->m_isDart = isDart;
    player->m_isRobot = isRobot;
    player->m_isSpider = isSpider;
    player->m_isSwing = isSwing;
    player->m_isUpsideDown = isUpsideDown;
    player->m_isDashing = isDashing;
    player->m_isDead = isDead;
    player->m_isOnGround = isOnGround;
    player->m_isOnGround2 = isOnGround2;
    player->m_isOnGround3 = isOnGround3;
    player->m_isOnGround4 = isOnGround4;
    player->m_isGoingLeft = isGoingLeft;
    player->m_isSliding = isSliding;
    player->m_isOnSlope = isOnSlope;
    player->m_wasOnSlope = wasOnSlope;
    player->m_isAccelerating = isAccelerating;
    player->m_isRotating = isRotating;
    player->m_isBallRotating = isBallRotating;
    player->m_isMoving = isMoving;
    player->m_isHidden = isHidden;
    player->m_isLocked = isLocked;
    player->m_hasGlow = hasGlow;
    player->m_isBeingSpawnedByDualPortal = isBeingSpawnedByDualPortal;
    player->m_maybeIsFalling = maybeIsFalling;
    player->m_maybeHasStopped = maybeHasStopped;
    player->m_jumpBuffered = jumpBuffered;
    player->m_wasJumpBuffered = wasJumpBuffered;
    player->m_wasRobotJump = wasRobotJump;
    player->m_stateRingJump = stateRingJump;
    player->m_stateRingJump2 = stateRingJump2;
    player->m_fixRobotJump = fixRobotJump;
    player->m_controlsDisabled = controlsDisabled;
    player->m_disablePlayerSqueeze = disablePlayerSqueeze;
    player->m_hasEverJumped = hasEverJumped;
    player->m_wasTeleported = wasTeleported;
    player->m_decreaseBoostSlide = decreaseBoostSlide;
    player->m_isOnIce = isOnIce;
    player->m_padRingRelated = padRingRelated;
    player->m_ringJumpRelated = ringJumpRelated;
    player->m_touchedRing = touchedRing;
}
