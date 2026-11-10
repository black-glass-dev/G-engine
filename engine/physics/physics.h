// G-Engine — physics/physics.h
// A small layer over Box2D v3: the rest of the engine never includes Box2D headers.
#pragma once

#include <vector>

#include "core/math.h"

namespace ge {

using BodyId = i32;  // 0 = no body

struct BodyDesc {
    enum class Type : u8 { Static, Dynamic, Character };
    Type type = Type::Static;
    Vec2 pos;
    Vec2 half{0.5f, 0.5f};
    f32 density = 1.0f;
    f32 friction = 0.6f;
    f32 restitution = 0.0f;
    i32 owner = -1;  // the game object this body belongs to
};

/// One thing a body is touching right now.
struct Contact {
    i32 otherOwner = -1;       // owner of the other body (-1: level geometry)
    bool otherStatic = false;
    Vec2 normal;               // points from the other body towards this one
};

class PhysicsWorld {
public:
    PhysicsWorld() = default;
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void create(Vec2 gravity);
    void destroy();
    bool valid() const { return created_; }

    BodyId addBody(const BodyDesc& desc);
    void removeBody(BodyId id);
    void step(f32 dt, int subSteps = 4);

    Vec2 position(BodyId id) const;
    f32 angle(BodyId id) const;
    Vec2 velocity(BodyId id) const;
    void setVelocity(BodyId id, Vec2 v);
    void setPosition(BodyId id, Vec2 pos);
    void setGravityScale(BodyId id, f32 scale);
    Vec2 gravity() const { return gravity_; }

    /// What the body touches after the last step.
    void contacts(BodyId id, std::vector<Contact>& out) const;

    int bodyCount() const { return liveBodies_; }

private:
    struct Slot;
    std::vector<Slot>* slots_ = nullptr;
    bool created_ = false;
    Vec2 gravity_{0, -30};
    int liveBodies_ = 0;
    u32 world_[2] = {};  // b2WorldId, opaque
};

}  // namespace ge
