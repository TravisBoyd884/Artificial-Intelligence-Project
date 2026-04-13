#include "Game/game.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {
    const glm::vec3 kBallStart (0.0f, 1.2f,  4.5f);
    const glm::vec3 kHoopCentre(0.0f, 3.0f, -5.0f);
    constexpr float kHoopRadius = 0.23f;
    constexpr float kBallRadius = 0.12f;
    constexpr float kBallMass   = 0.624f;
    constexpr float kMaxTime    = 6.0f;
}

Game::Game() {
    setupStaticBodies();
    m_ballBody  = m_physics.addDynamicSphere(kBallRadius, kBallMass, kBallStart);
    m_prevBallY = kBallStart.y;
}

void Game::setupStaticBodies() {
    m_physics.addStaticPlane(glm::vec3( 0,  1,  0), 0.0f)->setRestitution(0.4f); // floor
    m_physics.addStaticPlane(glm::vec3( 0, -1,  0), 6.0f)->setRestitution(0.2f); // ceiling
    m_physics.addStaticPlane(glm::vec3( 0,  0, -1), 6.0f)->setRestitution(0.3f); // front wall  (+Z)
    m_physics.addStaticPlane(glm::vec3( 0,  0,  1), 6.0f)->setRestitution(0.3f); // back wall   (-Z)
    m_physics.addStaticPlane(glm::vec3( 1,  0,  0), 6.0f)->setRestitution(0.3f); // left wall   (-X)
    m_physics.addStaticPlane(glm::vec3(-1,  0,  0), 6.0f)->setRestitution(0.3f); // right wall  (+X)
    m_physics.addStaticBox(glm::vec3(0.915f, 0.535f, 0.05f),
                           glm::vec3(0.0f, 3.4f, -5.1f))->setRestitution(0.5f);  // backboard
}

void Game::shoot(float yaw, float pitch, float power) {
    if (m_state != GameState::IDLE) return;

    glm::vec3 dir;
    dir.x = glm::cos(glm::radians(yaw))   * glm::cos(glm::radians(pitch));
    dir.y = glm::sin(glm::radians(pitch));
    dir.z = glm::sin(glm::radians(yaw))   * glm::cos(glm::radians(pitch));
    dir   = glm::normalize(dir);

    m_ballBody->setLinearVelocity(
        btVector3(dir.x * power, dir.y * power, dir.z * power));
    m_ballBody->activate(true);

    m_state       = GameState::IN_FLIGHT;
    m_episodeTime = 0.0f;
    m_reward      = 0.0f;
    m_prevBallY   = kBallStart.y;
}

void Game::update(float deltaTime) {
    if (m_state == GameState::IDLE || isDone()) return;

    m_physics.step(deltaTime);
    m_episodeTime += deltaTime;
    checkOutcome();
}

glm::vec3 Game::getBallPosition() const {
    btTransform t;
    m_ballBody->getMotionState()->getWorldTransform(t);
    const btVector3& o = t.getOrigin();
    return glm::vec3(o.x(), o.y(), o.z());
}

void Game::reset() {
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(kBallStart.x, kBallStart.y, kBallStart.z));
    m_ballBody->setWorldTransform(t);
    m_ballBody->getMotionState()->setWorldTransform(t);
    m_ballBody->clearForces();
    m_ballBody->setLinearVelocity(btVector3(0, 0, 0));
    m_ballBody->setAngularVelocity(btVector3(0, 0, 0));
    m_ballBody->activate(true);

    m_state       = GameState::IDLE;
    m_reward      = 0.0f;
    m_episodeTime = 0.0f;
    m_prevBallY   = kBallStart.y;
}

void Game::checkOutcome() {
    glm::vec3 pos = getBallPosition();

    float dist = glm::length(pos - kHoopCentre);
    m_reward = 0.01f / (1.0f + dist);

    if (m_prevBallY > kHoopCentre.y && pos.y <= kHoopCentre.y) {
        float dx = pos.x - kHoopCentre.x;
        float dz = pos.z - kHoopCentre.z;
        if (std::sqrt(dx * dx + dz * dz) < kHoopRadius - kBallRadius) {
            m_state  = GameState::SCORED;
            m_reward = 1.0f;
            m_prevBallY = pos.y;
            return;
        }
    }

    if (pos.y <= kBallRadius + 0.01f || m_episodeTime >= kMaxTime) {
        m_state  = GameState::MISSED;
        m_reward = -0.1f;
        m_prevBallY = pos.y;
        return;
    }

    m_prevBallY = pos.y;
}
