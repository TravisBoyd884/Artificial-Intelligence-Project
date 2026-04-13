#pragma once

#include "Physics/physics_world.hpp"
#include <glm/glm.hpp>

enum class GameState { IDLE, IN_FLIGHT, SCORED, MISSED };

class Game {
public:
    Game();

    void shoot(float yaw, float pitch, float power);
    void update(float deltaTime);
    void reset();

    GameState getState()        const { return m_state; }
    float     getReward()       const { return m_reward; }
    bool      isDone()          const { return m_state == GameState::SCORED ||
                                               m_state == GameState::MISSED; }
    glm::vec3 getBallPosition() const;

private:
    PhysicsWorld m_physics;
    btRigidBody* m_ballBody     = nullptr;

    GameState m_state       = GameState::IDLE;
    float     m_reward      = 0.0f;
    float     m_episodeTime = 0.0f;
    float     m_prevBallY   = 0.0f;

    void setupStaticBodies();
    void checkOutcome();
};
