#pragma once

#include "PxPhysicsAPI.h"
#include "math/Math.h"
#include <vector>
#include <iostream>

namespace MiniEngine {

using Math::Vec3;

class PhysicsWorld {
public:
    void Init() {
        m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
        if (!m_foundation) { std::cerr << "[PhysX] Foundation creation failed!" << std::endl; return; }

        m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true);
        if (!m_physics) { std::cerr << "[PhysX] Physics creation failed!" << std::endl; return; }

        physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        m_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = m_dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        m_scene = m_physics->createScene(sceneDesc);

        m_material = m_physics->createMaterial(0.5f, 0.5f, 0.3f);

        std::cout << "[PhysX] Initialized successfully" << std::endl;
    }

    void Shutdown() {
        if (m_characterBody) { m_characterBody->release(); m_characterBody = nullptr; }
        if (m_scene) { m_scene->release(); m_scene = nullptr; }
        if (m_dispatcher) { m_dispatcher->release(); m_dispatcher = nullptr; }
        if (m_physics) { m_physics->release(); m_physics = nullptr; }
        if (m_foundation) { m_foundation->release(); m_foundation = nullptr; }
        std::cout << "[PhysX] Shutdown" << std::endl;
    }

    void Step(float dt) {
        if (!m_scene) return;
        m_accumulator += dt;
        while (m_accumulator >= m_stepSize) {
            m_scene->simulate(m_stepSize);
            m_scene->fetchResults(true);
            m_accumulator -= m_stepSize;
        }
    }

    void AddGroundPlane() {
        if (!m_physics || !m_scene) return;
        physx::PxRigidStatic* ground = PxCreatePlane(*m_physics, physx::PxPlane(0, 1, 0, 0), *m_material);
        m_scene->addActor(*ground);
    }

    void AddStaticBox(Vec3 pos, Vec3 halfSize) {
        if (!m_physics || !m_scene) return;
        physx::PxRigidStatic* body = m_physics->createRigidStatic(
            physx::PxTransform(physx::PxVec3(pos.x, pos.y, pos.z)));
        physx::PxShape* shape = m_physics->createShape(
            physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), *m_material);
        body->attachShape(*shape);
        shape->release();
        m_scene->addActor(*body);
    }

    void AddDynamicBox(Vec3 pos, Vec3 halfSize, float density = 10.0f) {
        if (!m_physics || !m_scene) return;
        physx::PxRigidDynamic* body = m_physics->createRigidDynamic(
            physx::PxTransform(physx::PxVec3(pos.x, pos.y, pos.z)));
        physx::PxShape* shape = m_physics->createShape(
            physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), *m_material);
        body->attachShape(*shape);
        shape->release();
        physx::PxRigidBodyExt::updateMassAndInertia(*body, density);
        m_scene->addActor(*body);
    }

    void CreateCharacterBody(Vec3 pos, float radius, float halfHeight) {
        if (!m_physics || !m_scene) return;
        m_characterBody = m_physics->createRigidDynamic(
            physx::PxTransform(physx::PxVec3(pos.x, pos.y, pos.z)));
        m_characterBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
        physx::PxShape* shape = m_physics->createShape(
            physx::PxCapsuleGeometry(radius, halfHeight), *m_material);
        // Rotate capsule to stand upright (PhysX capsules default along X axis)
        shape->setLocalPose(physx::PxTransform(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1))));
        m_characterBody->attachShape(*shape);
        shape->release();
        m_scene->addActor(*m_characterBody);
    }

    void MoveCharacter(Vec3 newPos) {
        if (!m_characterBody) return;
        m_characterBody->setKinematicTarget(
            physx::PxTransform(physx::PxVec3(newPos.x, newPos.y, newPos.z)));
    }

    Vec3 GetCharacterPosition() {
        if (!m_characterBody) return Vec3(0, 0, 0);
        physx::PxTransform t = m_characterBody->getGlobalPose();
        return Vec3(t.p.x, t.p.y, t.p.z);
    }

    bool Raycast(Vec3 origin, Vec3 dir, float maxDist, Vec3& hitPoint, Vec3& hitNormal) {
        if (!m_scene) return false;
        physx::PxRaycastBuffer hit;
        bool status = m_scene->raycast(
            physx::PxVec3(origin.x, origin.y, origin.z),
            physx::PxVec3(dir.x, dir.y, dir.z),
            maxDist, hit);
        if (status && hit.hasBlock) {
            hitPoint = Vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            hitNormal = Vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            return true;
        }
        return false;
    }

    bool IsValid() const { return m_scene != nullptr; }

    float m_stepSize = 1.0f / 60.0f;

private:
    physx::PxDefaultAllocator m_allocator;
    physx::PxDefaultErrorCallback m_errorCallback;
    physx::PxFoundation* m_foundation = nullptr;
    physx::PxPhysics* m_physics = nullptr;
    physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
    physx::PxScene* m_scene = nullptr;
    physx::PxMaterial* m_material = nullptr;
    physx::PxRigidDynamic* m_characterBody = nullptr;
    float m_accumulator = 0;
};

} // namespace MiniEngine
