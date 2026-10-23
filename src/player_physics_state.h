// Port of player-physics-state.js
#pragma once

struct PlayerPhysicsState {
  double y, lastY, lastGroundPosY, yVelocity;
  bool onGround, canJump, isJumping, gravityFlipped, isFlying, wasBoosted;
  double collideTop, collideBottom;
  bool onCeiling, upKeyDown, upKeyPressed, isDead;
  double lastGroundY = 0;  // dynamic property set by Player::hitGround

  PlayerPhysicsState() { reset(); }
  void reset() {
    y = 30; lastY = 30; lastGroundPosY = 30; yVelocity = 0;
    onGround = true; canJump = true; isJumping = false; gravityFlipped = false;
    isFlying = false; wasBoosted = false; collideTop = 0; collideBottom = 0;
    onCeiling = false; upKeyDown = false; upKeyPressed = false; isDead = false;
  }
};
