#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <vector>

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void step(float deltaTime);

    btRigidBody* addStaticPlane  (glm::vec3 normal, float planeConstant);
    btRigidBody* addStaticBox    (glm::vec3 halfExtents, glm::vec3 position);
    btRigidBody* addDynamicSphere(float radius, float mass, glm::vec3 position,
                                  float restitution = 0.76f, float friction = 0.8f);

private:
    btDefaultCollisionConfiguration*     m_config;
    btCollisionDispatcher*               m_dispatcher;
    btBroadphaseInterface*               m_broadphase;
    btSequentialImpulseConstraintSolver* m_solver;
    btDiscreteDynamicsWorld*             m_world;

    std::vector<btRigidBody*>      m_bodies;
    std::vector<btCollisionShape*> m_shapes;
    std::vector<btMotionState*>    m_motionStates;
};
