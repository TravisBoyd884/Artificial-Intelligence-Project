#include "Physics/physics_world.hpp"

#include <algorithm>

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
    m_world->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
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
    m_world->addRigidBody(body);
    m_bodies.push_back(body);
    return body;
}
