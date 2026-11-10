// G-Engine — physics/physics.cpp
#include "physics/physics.h"

#include <cstring>

#include "box2d/box2d.h"
#include "core/log.h"

namespace ge {

struct PhysicsWorld::Slot {
    b2BodyId body = b2_nullBodyId;
    i32 owner = -1;
    bool live = false;
    bool isStatic = false;
};

namespace {

b2WorldId toWorld(const u32* raw) {
    b2WorldId id;
    static_assert(sizeof(b2WorldId) <= sizeof(u32) * 2, "b2WorldId does not fit");
    std::memcpy(&id, raw, sizeof(id));
    return id;
}

}  // namespace

PhysicsWorld::~PhysicsWorld() { destroy(); }

void PhysicsWorld::create(Vec2 gravity) {
    destroy();
    b2WorldDef def = b2DefaultWorldDef();
    def.gravity = {gravity.x, gravity.y};
    def.workerCount = 1;       // single threaded: identical results on every platform
    def.enableSleep = true;
    const b2WorldId id = b2CreateWorld(&def);
    std::memset(world_, 0, sizeof(world_));
    std::memcpy(world_, &id, sizeof(id));
    gravity_ = gravity;
    slots_ = new std::vector<Slot>();
    slots_->push_back(Slot{});  // id 0 = none
    created_ = true;
    liveBodies_ = 0;
}

void PhysicsWorld::destroy() {
    if (!created_) return;
    b2DestroyWorld(toWorld(world_));
    delete slots_;
    slots_ = nullptr;
    created_ = false;
    liveBodies_ = 0;
}

BodyId PhysicsWorld::addBody(const BodyDesc& desc) {
    GE_ASSERT(created_);
    b2BodyDef bd = b2DefaultBodyDef();
    bd.type = desc.type == BodyDesc::Type::Static ? b2_staticBody : b2_dynamicBody;
    bd.position = {desc.pos.x, desc.pos.y};
    const BodyId id = static_cast<BodyId>(slots_->size());
    bd.userData = reinterpret_cast<void*>(static_cast<std::intptr_t>(id));
    if (desc.type == BodyDesc::Type::Character) {
        bd.motionLocks.angularZ = true;  // characters stay upright
        bd.enableSleep = false;
    }
    const b2BodyId body = b2CreateBody(toWorld(world_), &bd);

    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = desc.density;
    sd.material.friction = desc.friction;
    sd.material.restitution = desc.restitution;
    sd.userData = bd.userData;
    if (desc.type == BodyDesc::Type::Character) {
        // A capsule slides over the seams between floor tiles instead of catching on them.
        sd.material.friction = 0.0f;  // the controller sets the horizontal speed itself
        const f32 radius = desc.half.x;
        const f32 inner = desc.half.y > radius ? desc.half.y - radius : 0.0f;
        const b2Capsule capsule = {{0.0f, -inner}, {0.0f, inner}, radius};
        b2CreateCapsuleShape(body, &sd, &capsule);
    } else {
        const f32 rounding = desc.type == BodyDesc::Type::Dynamic ? 0.03f : 0.0f;
        const b2Polygon box = rounding > 0 ? b2MakeRoundedBox(desc.half.x - rounding, desc.half.y - rounding, rounding)
                                           : b2MakeBox(desc.half.x, desc.half.y);
        b2CreatePolygonShape(body, &sd, &box);
    }

    Slot slot;
    slot.body = body;
    slot.owner = desc.owner;
    slot.live = true;
    slot.isStatic = desc.type == BodyDesc::Type::Static;
    slots_->push_back(slot);
    ++liveBodies_;
    return id;
}

void PhysicsWorld::removeBody(BodyId id) {
    if (!created_ || id <= 0 || id >= static_cast<BodyId>(slots_->size())) return;
    Slot& slot = (*slots_)[static_cast<std::size_t>(id)];
    if (!slot.live) return;
    b2DestroyBody(slot.body);
    slot.live = false;
    --liveBodies_;
}

void PhysicsWorld::step(f32 dt, int subSteps) {
    if (created_) b2World_Step(toWorld(world_), dt, subSteps);
}

#define GE_SLOT_OR(id, fallback)                                                         \
    if (!created_ || (id) <= 0 || (id) >= static_cast<BodyId>(slots_->size())) return fallback; \
    const Slot& slot = (*slots_)[static_cast<std::size_t>(id)];                          \
    if (!slot.live) return fallback;

Vec2 PhysicsWorld::position(BodyId id) const {
    GE_SLOT_OR(id, Vec2{});
    const b2Vec2 p = b2Body_GetPosition(slot.body);
    return {p.x, p.y};
}

f32 PhysicsWorld::angle(BodyId id) const {
    GE_SLOT_OR(id, 0.0f);
    return b2Rot_GetAngle(b2Body_GetRotation(slot.body));
}

Vec2 PhysicsWorld::velocity(BodyId id) const {
    GE_SLOT_OR(id, Vec2{});
    const b2Vec2 v = b2Body_GetLinearVelocity(slot.body);
    return {v.x, v.y};
}

void PhysicsWorld::setVelocity(BodyId id, Vec2 v) {
    GE_SLOT_OR(id, );
    b2Body_SetLinearVelocity(slot.body, {v.x, v.y});
}

void PhysicsWorld::setPosition(BodyId id, Vec2 pos) {
    GE_SLOT_OR(id, );
    b2Body_SetTransform(slot.body, {pos.x, pos.y}, b2Body_GetRotation(slot.body));
    b2Body_SetAwake(slot.body, true);
}

void PhysicsWorld::setGravityScale(BodyId id, f32 scale) {
    GE_SLOT_OR(id, );
    b2Body_SetGravityScale(slot.body, scale);
}

void PhysicsWorld::contacts(BodyId id, std::vector<Contact>& out) const {
    out.clear();
    GE_SLOT_OR(id, );
    b2ContactData data[16];
    const int count = b2Body_GetContactData(slot.body, data, static_cast<int>(GE_ARRAY_COUNT(data)));
    for (int i = 0; i < count; ++i) {
        const b2ContactData& c = data[i];
        if (c.manifold.pointCount == 0) continue;
        // Speculative contacts that are not actually touching yet do not count.
        f32 separation = 1.0f;
        for (int k = 0; k < c.manifold.pointCount; ++k) separation = min(separation, c.manifold.points[k].separation);
        if (separation > 0.02f) continue;
        const BodyId a = static_cast<BodyId>(reinterpret_cast<std::intptr_t>(b2Shape_GetUserData(c.shapeIdA)));
        const BodyId b = static_cast<BodyId>(reinterpret_cast<std::intptr_t>(b2Shape_GetUserData(c.shapeIdB)));
        const bool selfIsA = a == id;
        const BodyId other = selfIsA ? b : a;
        Contact contact;
        // The manifold normal points from A to B.
        contact.normal = selfIsA ? Vec2{-c.manifold.normal.x, -c.manifold.normal.y} : Vec2{c.manifold.normal.x, c.manifold.normal.y};
        if (other > 0 && other < static_cast<BodyId>(slots_->size())) {
            contact.otherOwner = (*slots_)[static_cast<std::size_t>(other)].owner;
            contact.otherStatic = (*slots_)[static_cast<std::size_t>(other)].isStatic;
        }
        out.push_back(contact);
    }
}

#undef GE_SLOT_OR

}  // namespace ge
