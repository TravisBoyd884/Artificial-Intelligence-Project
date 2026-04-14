#include "Physics/physics_world.hpp"

#include <algorithm>
#include <cmath>

PhysicsWorld::PhysicsWorld() {
    m_config     = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_config);
    m_broadphase = new btDbvtBroadphase();
    m_solver     = new btSequentialImpulseConstraintSolver();
    m_world      = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase,
                                               m_solver, m_config);
    m_world->setGravity(btVector3(0.0f, -9.8f, 0.0f));
}

PhysicsWorld::~PhysicsWorld() {
    for (auto* body : m_bodies) {
        m_world->removeRigidBody(body);
        delete body;
    }
    for (auto* ms : m_motionStates) delete ms;
    for (auto* sh : m_shapes)       delete sh;

    delete m_world;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_config;
}

void PhysicsWorld::step(float deltaTime) {
    // CCD on the ball handles tunnelling; 120 Hz substeps are enough for accuracy
    // without the 4× training overhead that 240 Hz caused.
    m_world->stepSimulation(deltaTime, 4, 1.0f / 120.0f);
}

btRigidBody* PhysicsWorld::addStaticPlane(glm::vec3 normal, float planeConstant) {
    auto* shape = new btStaticPlaneShape(
        btVector3(normal.x, normal.y, normal.z), planeConstant);
    m_shapes.push_back(shape);

    auto* ms = new btDefaultMotionState();
    m_motionStates.push_back(ms);

    btRigidBody::btRigidBodyConstructionInfo ci(0.0f, ms, shape);
    auto* body = new btRigidBody(ci);
    m_world->addRigidBody(body);
    m_bodies.push_back(body);
    return body;
}

btRigidBody* PhysicsWorld::addStaticBox(glm::vec3 halfExtents, glm::vec3 position) {
    auto* shape = new btBoxShape(
        btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
    m_shapes.push_back(shape);

    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(position.x, position.y, position.z));

    auto* ms = new btDefaultMotionState(t);
    m_motionStates.push_back(ms);

    btRigidBody::btRigidBodyConstructionInfo ci(0.0f, ms, shape);
    auto* body = new btRigidBody(ci);
    m_world->addRigidBody(body);
    m_bodies.push_back(body);
    return body;
}

btRigidBody* PhysicsWorld::addStaticRim(glm::vec3 center, float ringRadius,
                                        float tubeRadius, int segments) {
    // Approximate a torus with N capsule segments arranged in a horizontal circle.
    // Each capsule spans one chord of the ring; its axis is oriented along the tangent.
    static constexpr float kPi = 3.14159265358979f;

    float chordLen  = 2.0f * ringRadius * std::sin(kPi / segments);
    float cylHeight = std::max(0.0f, chordLen - 2.0f * tubeRadius);

    auto* compound = new btCompoundShape();

    for (int i = 0; i < segments; ++i) {
        auto* cap = new btCapsuleShape(tubeRadius, cylHeight);
        // Default margin (0.04 m) exceeds the tube radius — shrink it so the
        // GJK solver doesn't create a phantom collision shell larger than the shape.
        cap->setMargin(tubeRadius * 0.1f);
        m_shapes.push_back(cap);   // tracked for deletion

        // Centre angle of this segment
        float theta = (2.0f * kPi / segments) * (i + 0.5f);

        float px = center.x + ringRadius * std::cos(theta);
        float pz = center.z + ringRadius * std::sin(theta);

        // Rotate the capsule's Y-axis onto the tangent direction
        // (-sin θ, 0, cos θ).  The rotation axis is (cos θ, 0, sin θ)
        // and the angle is 90°, giving quaternion (w=√½, x=√½·cosθ, z=√½·sinθ).
        const float s = 0.7071068f;  // sin/cos of 45°
        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(px, center.y, pz));
        t.setRotation(btQuaternion(s * std::cos(theta), 0.0f,
                                   s * std::sin(theta), s));
        compound->addChildShape(t, cap);
    }

    // Push compound after its children so cleanup order is: children first, compound last.
    m_shapes.push_back(compound);

    btTransform identity;
    identity.setIdentity();
    auto* ms = new btDefaultMotionState(identity);
    m_motionStates.push_back(ms);

    btRigidBody::btRigidBodyConstructionInfo ci(0.0f, ms, compound);
    auto* body = new btRigidBody(ci);
    m_world->addRigidBody(body);
    m_bodies.push_back(body);
    return body;
}

btRigidBody* PhysicsWorld::addDynamicSphere(float radius, float mass,
                                             glm::vec3 position,
                                             float restitution, float friction) {
    auto* shape = new btSphereShape(radius);
    m_shapes.push_back(shape);

    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(position.x, position.y, position.z));

    btVector3 inertia(0, 0, 0);
    shape->calculateLocalInertia(mass, inertia);

    auto* ms = new btDefaultMotionState(t);
    m_motionStates.push_back(ms);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, ms, shape, inertia);
    auto* body = new btRigidBody(ci);
    body->setRestitution(restitution);
    body->setFriction(friction);

    // Continuous Collision Detection — prevents tunnelling through thin geometry
    // (e.g. the rim) when the ball moves many times its own radius per step.
    body->setCcdMotionThreshold(radius);          // engage CCD when displacement > radius
    body->setCcdSweptSphereRadius(radius * 0.9f); // conservative swept radius for CCD test

    m_world->addRigidBody(body);
    m_bodies.push_back(body);
    return body;
}
