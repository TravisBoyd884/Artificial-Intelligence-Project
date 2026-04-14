#pragma once

#include "Physics/physics_world.hpp"
#include "Net/net.hpp"
#include <glm/glm.hpp>
#include <random>

enum class GameState { IDLE, IN_FLIGHT, SCORED, MISSED };

class Game {
public:
    Game();

    void shoot(float yaw, float pitch, float power);
    void update(float deltaTime);
    void reset();

    GameState  getState()        const { return m_state; }
    float      getReward()       const { return m_reward; }
    bool       isDone()          const { return m_state == GameState::SCORED ||
                                                m_state == GameState::MISSED; }
    glm::vec3  getBallPosition() const;
    glm::vec3  getBallStart()    const { return m_ballStart; }
    const Net& getNet()          const { return m_net; }
    void       setFixedSpawn(bool fixed) { m_fixedSpawn = fixed; }

private:
    PhysicsWorld m_physics;
    btRigidBody* m_ballBody = nullptr;
    Net          m_net;

    GameState m_state         = GameState::IDLE;
    float     m_reward        = 0.0f;
    float     m_episodeTime   = 0.0f;
    float     m_prevBallY     = 0.0f;
    float     m_minDistToHoop = 0.0f;
    glm::vec3 m_ballStart     = glm::vec3(0.0f, 1.2f, 4.5f);

    std::mt19937                          m_rng;
    std::uniform_real_distribution<float> m_distX{ -2.5f, 2.5f };
    std::uniform_real_distribution<float> m_distZ{  2.5f, 5.5f };
    std::uniform_real_distribution<float> m_distY{  1.0f, 1.5f };

    void      setupStaticBodies();
    void      checkOutcome();
    glm::vec3 randomStart();

    bool m_fixedSpawn = false;
};
