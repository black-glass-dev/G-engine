// G-Engine — scene/world.cpp
#include "scene/world.h"

#include <algorithm>
#include <cmath>

#include "core/log.h"
#include "core/random.h"
#include "script/compiler.h"

namespace ge {

using kotha::Dir;

// ------------------------------------------------------------------------------------------
// What a script can sense and do, for one object.
// ------------------------------------------------------------------------------------------

class ObjectHost final : public kotha::Host {
public:
    ObjectHost(World& world, Object& object) : w_(world), o_(object) {}

    bool buttonDown(kotha::Button button) override { return w_.buttons_[static_cast<int>(button)]; }
    bool onGround() override { return o_.kind != ObjectKind::Character || o_.mover.onGround; }
    bool moveDone() override { return o_.mover.done; }

    f64 getProp(kotha::Prop prop) override {
        switch (prop) {
            case kotha::Prop::Speed: return o_.mover.speed;
            case kotha::Prop::JumpHeight: return o_.mover.jumpHeight;
            case kotha::Prop::Gravity: return o_.mover.gravityScale;
            case kotha::Prop::Size: return o_.scale;
            case kotha::Prop::Brightness: return o_.light.intensity;
            case kotha::Prop::PlayerDistance: {
                const Object* p = w_.player();
                return p == nullptr || p->id == o_.id || !p->alive ? 1e9 : static_cast<f64>(distance(p->pos, o_.pos));
            }
            case kotha::Prop::None: break;
        }
        return 0;
    }

    void setProp(kotha::Prop prop, f64 value) override {
        const f32 v = static_cast<f32>(value);
        switch (prop) {
            case kotha::Prop::Speed: o_.mover.speed = clamp(v, 0.0f, 40.0f); break;
            case kotha::Prop::JumpHeight: o_.mover.jumpHeight = clamp(v, 0.0f, 30.0f); break;
            case kotha::Prop::Gravity:
                o_.mover.gravityScale = clamp(v, -4.0f, 6.0f);
                w_.physics_.setGravityScale(o_.body, o_.mover.gravityScale);
                break;
            case kotha::Prop::Size: {
                const f32 scale = clamp(v, 0.3f, 3.0f);
                if (std::fabs(scale - o_.scale) < 1e-4f) break;
                const f32 feet = o_.pos.y - o_.half.y * o_.scale;
                o_.scale = scale;
                if (o_.body != 0) {
                    o_.pos.y = feet + o_.half.y * o_.scale + 0.01f;  // grow upwards, not into the floor
                    w_.createBody(o_);
                }
                break;
            }
            case kotha::Prop::Brightness:
                o_.light.present = true;
                o_.light.intensity = clamp(v, 0.0f, 30.0f);
                break;
            default: break;
        }
    }

    f64 getVar(const std::string& key) override { return w_.variable(key); }
    void setVar(const std::string& key, f64 value) override { w_.vars_[key] = value; }

    bool touchMatches(i32 other, const std::string& key) override {
        if (other < 0 || other >= static_cast<i32>(w_.objects_.size())) return false;
        if (key == "#player") return other == w_.player_;
        return w_.objects_[static_cast<std::size_t>(other)].key == key;
    }

    void move(Dir dir, bool run, f32 speedScale, f32 seconds, f32 meters) override {
        Mover& m = o_.mover;
        if (dir == Dir::Backward) {
            m.facing = -m.facing;
            dir = Dir::Forward;
        }
        m.moving = true;
        m.dir = dir;
        m.run = run;
        m.scale = speedScale;
        m.timeLeft = seconds;
        m.distLeft = meters;
        m.done = !(seconds > 0 || meters > 0);
        m.blocked = 0;
    }

    void stop() override {
        Mover& m = o_.mover;
        m.moving = false;
        m.done = true;
        m.timeLeft = m.distLeft = 0;
    }

    void jump(f32 scale) override {
        if (o_.kind == ObjectKind::Character) {
            o_.mover.jumpBuffer = 0.14f;
            o_.mover.jumpScale = scale;
        } else if (o_.kind == ObjectKind::Crate) {
            Vec2 v = w_.physics_.velocity(o_.body);
            v.y = 7.0f * scale;
            w_.physics_.setVelocity(o_.body, v);
        }
    }

    void face(Dir dir) override {
        Mover& m = o_.mover;
        switch (dir) {
            case Dir::Opposite:
            case Dir::Backward:
                m.facing = -m.facing;
                // turning round while walking means walking the other way
                if (m.dir == Dir::Right) {
                    m.dir = Dir::Left;
                } else if (m.dir == Dir::Left) {
                    m.dir = Dir::Right;
                }
                break;
            case Dir::Right: m.facing = 1; break;
            case Dir::Left: m.facing = -1; break;
            case Dir::TowardPlayer:
            case Dir::AwayFromPlayer: {
                const Object* p = w_.player();
                if (p == nullptr || p->id == o_.id) break;
                const i32 toward = p->pos.x >= o_.pos.x ? 1 : -1;
                m.facing = dir == Dir::TowardPlayer ? toward : -toward;
                break;
            }
            default: break;
        }
    }

    void lightSwitch(bool on) override {
        o_.light.present = true;
        o_.light.on = on;
    }

    void lightColor(u32 rgb) override {
        o_.light.present = true;
        o_.light.color = colorFromHex(rgb);
    }

    void say(const std::string& text, f32 seconds) override {
        o_.sayText = text;
        o_.sayTime = seconds;
    }

    void send(const std::string& message) override { w_.nextMessages_.push_back(message); }

    void destroy(const std::string& key) override {
        if (key.empty()) {
            w_.kill(o_);
            return;
        }
        // "remove the coin" means the coin I am touching; without one, every object of that name.
        bool any = false;
        for (const i32 id : o_.touching) {
            Object& t = w_.objects_[static_cast<std::size_t>(id)];
            if (t.alive && t.key == key) {
                w_.kill(t);
                any = true;
            }
        }
        if (any) return;
        for (Object& t : w_.objects_) {
            if (t.alive && t.key == key) w_.kill(t);
        }
    }

    void respawn() override {
        o_.pos = o_.spawn;
        w_.physics_.setPosition(o_.body, o_.pos);
        w_.physics_.setVelocity(o_.body, {0, 0});
    }

    void shake(f32 strength, f32 seconds) override {
        w_.shake_ = strength;
        w_.shakeTime_ = w_.shakeDuration_ = max(seconds, 0.05f);
    }

    void gameEvent(kotha::GameEvent event) override {
        switch (event) {
            case kotha::GameEvent::Win: w_.state_ = GameState::Won; break;
            case kotha::GameEvent::Lose: w_.state_ = GameState::Lost; break;
            case kotha::GameEvent::Restart: w_.restartRequested_ = true; break;
        }
    }

private:
    World& w_;
    Object& o_;
};

// ------------------------------------------------------------------------------------------
// Loading
// ------------------------------------------------------------------------------------------

bool World::solidAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return false;
    return tiles_[static_cast<std::size_t>(y) * width_ + x] != 0;
}

void World::load(const std::string& level) {
    level_ = level;
    restart();
}

void World::restart() {
    restartRequested_ = false;
    objects_.clear();
    vars_.clear();
    messages_.clear();
    nextMessages_.clear();
    state_ = GameState::Playing;
    time_ = 0;
    player_ = -1;
    shake_ = shakeTime_ = 0;
    shakeOffset_ = {};
    physics_.create({0.0f, -32.0f});

    // Split the picture into rows.
    std::vector<std::string> rows;
    std::size_t start = 0;
    while (start <= level_.size()) {
        std::size_t end = level_.find('\n', start);
        if (end == std::string::npos) end = level_.size();
        if (end > start) rows.push_back(level_.substr(start, end - start));
        start = end + 1;
    }
    // The picture can come from outside (the editor): keep its size within sane limits.
    bool cut = false;
    if (static_cast<int>(rows.size()) > kMaxLevelHeight) {
        rows.resize(static_cast<std::size_t>(kMaxLevelHeight));
        cut = true;
    }
    height_ = static_cast<int>(rows.size());
    width_ = 0;
    for (std::string& row : rows) {
        if (static_cast<int>(row.size()) > kMaxLevelWidth) {
            row.resize(static_cast<std::size_t>(kMaxLevelWidth));
            cut = true;
        }
        width_ = max(width_, static_cast<int>(row.size()));
    }
    if (cut) GE_LOG_WARN("level: the picture is larger than %d x %d and was cut off", kMaxLevelWidth, kMaxLevelHeight);
    tiles_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0);
    objects_.reserve(256);
    bool tooMany = false;

    for (int r = 0; r < height_; ++r) {
        const int y = height_ - 1 - r;
        for (int x = 0; x < static_cast<int>(rows[static_cast<std::size_t>(r)].size()); ++x) {
            char ch = rows[static_cast<std::size_t>(r)][static_cast<std::size_t>(x)];
            const Vec2 centre{x + 0.5f, y + 0.5f};
            if (ch != '#' && ch != '.' && static_cast<int>(objects_.size()) >= kMaxObjects) {
                if (!tooMany) GE_LOG_WARN("level: more than %d objects; the rest are left out", kMaxObjects);
                tooMany = true;
                ch = '.';
            }
            if (ch == 'P' && player_ >= 0) ch = '.';  // one player only: the first one counts
            switch (ch) {
                case '#': tiles_[static_cast<std::size_t>(y) * width_ + x] = 1; break;
                case 'P': {
                    Object& o = spawn(ObjectKind::Character, "খেলোয়াড়", {centre.x, y + 0.45f});
                    o.sprite = Sprite::Player;
                    o.light.present = true;
                    o.light.color = {0.55f, 0.80f, 1.0f, 1.0f};
                    o.light.intensity = 1.5f;
                    o.light.radius = 6.0f;
                    o.light.height = 0.9f;
                    o.light.size = 0.22f;
                    o.light.offset = {0, 0.2f};
                    player_ = o.id;
                    break;
                }
                case 'G': {
                    Object& o = spawn(ObjectKind::Character, "প্রহরী", {centre.x, y + 0.45f});
                    o.sprite = Sprite::Guard;
                    o.mover.speed = 2.2f;
                    o.light.present = true;
                    o.light.color = {1.0f, 0.22f, 0.10f, 1.0f};
                    o.light.intensity = 1.1f;
                    o.light.radius = 2.6f;
                    o.light.height = 0.7f;
                    o.light.shadows = false;
                    o.light.offset = {0, 0.2f};
                    break;
                }
                case 'c': {
                    Object& o = spawn(ObjectKind::Pickup, "কয়েন", centre);
                    o.sprite = Sprite::Coin;
                    o.size = {0.72f, 0.72f};
                    o.half = {0.28f, 0.28f};
                    o.phase = hash01(static_cast<u32>(x), static_cast<u32>(y)) * 6.0f;
                    o.light.present = true;
                    o.light.color = {1.0f, 0.75f, 0.25f, 1.0f};
                    o.light.intensity = 0.8f;
                    o.light.radius = 2.0f;
                    o.light.height = 0.6f;
                    o.light.shadows = false;
                    break;
                }
                case '^': {
                    Object& o = spawn(ObjectKind::Hazard, "কাঁটা", {centre.x, y + 0.24f});
                    o.sprite = Sprite::Spikes;
                    o.spriteOffset = {0, 0.26f};
                    o.half = {0.32f, 0.18f};  // a little smaller than the picture: near misses feel fair
                    o.layer = 1;
                    o.light.present = true;  // a faint glow: danger must be visible in the dark
                    o.light.color = {1.0f, 0.16f, 0.10f, 1.0f};
                    o.light.intensity = 0.55f;
                    o.light.radius = 1.7f;
                    o.light.height = 0.5f;
                    o.light.shadows = false;
                    o.light.offset = {0, 0.25f};
                    break;
                }
                case 'B': {
                    Object& o = spawn(ObjectKind::Crate, "বাক্স", {centre.x, y + 0.46f});
                    o.sprite = Sprite::Crate;
                    o.size = {0.9f, 0.9f};
                    o.half = {0.45f, 0.45f};
                    o.castsShadow = true;
                    break;
                }
                case 'D': {
                    Object& o = spawn(ObjectKind::Door, "দরজা", {centre.x, y + 1.0f});
                    o.sprite = Sprite::Door;
                    o.size = {1.0f, 2.0f};
                    o.half = {0.44f, 1.0f};
                    o.castsShadow = true;
                    break;
                }
                case 'L': {
                    Object& o = spawn(ObjectKind::Lamp, "বাতি", centre);
                    o.sprite = Sprite::Lamp;
                    o.layer = 4;
                    o.light.present = true;
                    o.light.color = {1.0f, 0.70f, 0.36f, 1.0f};
                    o.light.intensity = 3.0f;
                    o.light.radius = 9.5f;
                    o.light.height = 1.3f;
                    o.light.size = 0.38f;
                    o.light.offset = {0, -0.06f};
                    o.light.flicker = 0.07f;
                    o.phase = hash01(static_cast<u32>(x), static_cast<u32>(y), 3) * 20.0f;
                    break;
                }
                default: break;
            }
        }
    }

    // Level geometry: one wide box per horizontal run of tiles, so the floor has no seams.
    for (int y = 0; y < height_; ++y) {
        int x = 0;
        while (x < width_) {
            if (!solidAt(x, y)) {
                ++x;
                continue;
            }
            int end = x;
            while (end < width_ && solidAt(end, y)) ++end;
            BodyDesc desc;
            desc.type = BodyDesc::Type::Static;
            desc.pos = {(x + end) * 0.5f, y + 0.5f};
            desc.half = {(end - x) * 0.5f, 0.5f};
            desc.friction = 0.8f;
            physics_.addBody(desc);
            x = end;
        }
    }
    buildEdges();

    for (Object& o : objects_) {
        o.spawn = o.pos;
        createBody(o);
        if (o.script >= 0 && scripts_[static_cast<std::size_t>(o.script)].program.ok()) o.vm.load(&scripts_[static_cast<std::size_t>(o.script)].program);
    }
    updateCamera(true);
}

Object& World::spawn(ObjectKind kind, const std::string& name, Vec2 pos) {
    Object o;
    o.id = static_cast<i32>(objects_.size());
    o.kind = kind;
    o.key = kotha::canonicalName(name);
    o.pos = pos;
    switch (kind) {
        case ObjectKind::Character:
            o.half = {0.25f, 0.42f};
            o.spriteOffset = {0, 0.05f};
            o.castsShadow = true;
            o.layer = 5;
            break;
        case ObjectKind::Pickup:
        case ObjectKind::Hazard: o.sensor = true; break;
        default: break;
    }
    for (std::size_t i = 0; i < scripts_.size(); ++i) {
        if (scripts_[i].key == o.key) o.script = static_cast<i32>(i);
    }
    objects_.push_back(std::move(o));
    return objects_.back();
}

void World::createBody(Object& o) {
    if (o.body != 0) {
        physics_.removeBody(o.body);
        o.body = 0;
    }
    BodyDesc desc;
    desc.pos = o.pos;
    desc.half = o.half * o.scale;
    desc.owner = o.id;
    switch (o.kind) {
        case ObjectKind::Character:
            desc.type = BodyDesc::Type::Character;
            desc.density = 1.2f;
            break;
        case ObjectKind::Crate:
            desc.type = BodyDesc::Type::Dynamic;
            desc.density = 0.55f;
            desc.friction = 0.55f;
            break;
        case ObjectKind::Door: desc.type = BodyDesc::Type::Static; break;
        default: return;  // touched by overlap only
    }
    o.body = physics_.addBody(desc);
    if (o.kind == ObjectKind::Character) physics_.setGravityScale(o.body, o.mover.gravityScale);
}

void World::kill(Object& o) {
    if (!o.alive) return;
    o.alive = false;
    if (o.body != 0) physics_.removeBody(o.body);
    o.body = 0;
}

/// The outline of the solid tiles as long line segments: these cast the shadows.
void World::buildEdges() {
    edges_.clear();
    for (int y = 0; y <= height_; ++y) {  // horizontal edges on the line between rows y-1 and y
        for (int side = 0; side < 2; ++side) {
            int run = -1;
            for (int x = 0; x <= width_; ++x) {
                const bool below = solidAt(x, y - 1), above = solidAt(x, y);
                const bool edge = x < width_ && (side == 0 ? (below && !above) : (above && !below));
                if (edge && run < 0) run = x;
                if (!edge && run >= 0) {
                    edges_.push_back({{static_cast<f32>(run), static_cast<f32>(y)}, {static_cast<f32>(x), static_cast<f32>(y)}, -1});
                    run = -1;
                }
            }
        }
    }
    for (int x = 0; x <= width_; ++x) {  // vertical edges on the line between columns x-1 and x
        for (int side = 0; side < 2; ++side) {
            int run = -1;
            for (int y = 0; y <= height_; ++y) {
                const bool left = solidAt(x - 1, y), right = solidAt(x, y);
                const bool edge = y < height_ && (side == 0 ? (left && !right) : (right && !left));
                if (edge && run < 0) run = y;
                if (!edge && run >= 0) {
                    edges_.push_back({{static_cast<f32>(x), static_cast<f32>(run)}, {static_cast<f32>(x), static_cast<f32>(y)}, -1});
                    run = -1;
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------
// Scripts
// ------------------------------------------------------------------------------------------

std::vector<std::string> World::builtinNames() { return {"খেলোয়াড়", "প্রহরী", "কয়েন", "কাঁটা", "বাক্স", "দরজা", "বাতি"}; }

std::vector<std::string> World::knownNames() const {
    // Everything a level picture can contain, then whatever else has a script or is in the level.
    std::vector<std::string> names = builtinNames();
    for (const ScriptSlot& s : scripts_) names.push_back(s.name);
    for (const Object& o : objects_) names.push_back(o.key);
    return names;
}

int World::addScript(const std::string& name, const std::string& source) {
    for (Object& o : objects_) o.vm.load(nullptr);  // the slots are about to move in memory
    const std::string key = kotha::canonicalName(name);
    int index = -1;
    for (std::size_t i = 0; i < scripts_.size(); ++i) {
        if (scripts_[i].key == key) index = static_cast<int>(i);  // a second script for the same name replaces the first
    }
    if (index < 0) {
        scripts_.emplace_back();
        index = static_cast<int>(scripts_.size()) - 1;
    }
    ScriptSlot& slot = scripts_[static_cast<std::size_t>(index)];
    slot.name = name;
    slot.key = key;
    slot.source = slot.original = source;
    slot.program = check(source);
    if (!slot.program.ok()) GE_LOG_WARN("script '%s': %s", name.c_str(), slot.program.errors[0].en.c_str());
    if (!level_.empty()) restart();
    return index;
}

kotha::Program World::check(const std::string& source) const {
    kotha::CompileOptions options;
    options.knownNames = knownNames();
    options.strictObjects = true;  // "when touching X": X has to exist in this game
    return kotha::compile(source, options);
}

kotha::Program World::setScript(int index, const std::string& source) {
    kotha::Program program = check(source);
    if (index < 0 || index >= scriptCount()) return program;
    ScriptSlot& slot = scripts_[static_cast<std::size_t>(index)];
    if (program.ok()) {
        // Objects point into the slot's program: detach them before it is replaced.
        for (Object& o : objects_) o.vm.load(nullptr);
        slot.source = source;
        slot.program = program;
        restart();
    }
    return program;
}

void World::resetScripts() {
    for (Object& o : objects_) o.vm.load(nullptr);
    for (ScriptSlot& slot : scripts_) {
        slot.source = slot.original;
        slot.program = check(slot.source);
    }
    restart();
}

f64 World::variable(const std::string& key) const {
    const auto it = vars_.find(key);
    return it != vars_.end() ? it->second : 0.0;
}

const Object* World::find(const std::string& name) const {
    const std::string key = kotha::canonicalName(name);
    for (const Object& o : objects_) {
        if (o.alive && o.key == key) return &o;
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------
// Simulation
// ------------------------------------------------------------------------------------------

void World::step() {
    if (state_ != GameState::Playing) return;
    time_ += kStep;

    // 1. scripts
    messages_.swap(nextMessages_);
    nextMessages_.clear();
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        Object& o = objects_[i];
        if (!o.alive || !o.vm.hasProgram()) continue;
        for (const std::string& m : messages_) o.vm.notifyMessage(m);
        ObjectHost host(*this, o);
        o.vm.update(host, kStep);
    }

    // 2. movement orders -> velocities
    for (Object& o : objects_) {
        if (!o.alive) continue;
        o.before = o.pos;
        runMover(o);
    }

    // 3. physics
    physics_.step(kStep, 4);

    // 4. read back, sense the surroundings
    for (Object& o : objects_) {
        if (!o.alive) continue;
        if (o.body != 0 && o.kind != ObjectKind::Door) {
            o.pos = physics_.position(o.body);
            if (o.kind == ObjectKind::Crate) o.rotation = physics_.angle(o.body);
        }
        advanceOrder(o, distance(o.before, o.pos));
        if (o.kind == ObjectKind::Character) sense(o);
        // Anything that falls out of the level starts again.
        if (o.pos.y < -6.0f && o.body != 0) {
            o.pos = o.spawn;
            physics_.setPosition(o.body, o.pos);
            physics_.setVelocity(o.body, {0, 0});
        }
        o.phase += kStep;
        if (o.sayTime > 0) o.sayTime -= kStep;
        o.mover.landSquash = max(0.0f, o.mover.landSquash - kStep * 5.0f);
    }

    updateCamera(false);
    if (restartRequested_) restart();
}

void World::runMover(Object& o) {
    Mover& m = o.mover;
    Vec2 want{0, 0};
    if (m.moving) {
        switch (m.dir) {
            case Dir::Right: want.x = 1; break;
            case Dir::Left: want.x = -1; break;
            case Dir::Up: want.y = 1; break;
            case Dir::Down: want.y = -1; break;
            case Dir::Forward: want.x = static_cast<f32>(m.facing); break;
            case Dir::TowardPlayer:
            case Dir::AwayFromPlayer: {
                const Object* p = player();
                if (p == nullptr || p->id == o.id) break;
                const Vec2 d = p->pos - o.pos;
                if (o.kind == ObjectKind::Character) {
                    if (std::fabs(d.x) > 0.3f) want.x = d.x > 0 ? 1.0f : -1.0f;
                } else if (length(d) > 0.2f) {
                    want = normalize(d);
                }
                if (m.dir == Dir::AwayFromPlayer) want = want.x == 0 && want.y == 0 ? Vec2{static_cast<f32>(m.facing), 0} : -want;
                break;
            }
            default: break;
        }
    }
    if (want.x > 0.1f) m.facing = 1;
    if (want.x < -0.1f) m.facing = -1;
    const f32 speed = m.speed * (m.run ? 1.6f : 1.0f) * m.scale;
    m.targetSpeed = (want.x != 0 || want.y != 0) ? speed : 0.0f;

    if (o.kind == ObjectKind::Character) {
        if (o.body == 0) return;
        Vec2 v = physics_.velocity(o.body);
        v.x = moveTowards(v.x, want.x * speed, (m.onGround ? 70.0f : 36.0f) * kStep);
        m.coyote = m.onGround ? 0.1f : max(0.0f, m.coyote - kStep);
        if (m.jumpBuffer > 0) {
            if (m.coyote > 0) {
                const f32 g = -physics_.gravity().y * max(m.gravityScale, 0.05f);
                v.y = std::sqrt(2.0f * g * max(m.jumpHeight * m.jumpScale, 0.0f));
                m.coyote = 0;
                m.jumpBuffer = 0;
            } else {
                m.jumpBuffer -= kStep;
            }
        }
        physics_.setVelocity(o.body, v);
    } else if (o.body == 0 && m.targetSpeed > 0) {
        // Things without a body (lamps, coins, spikes) simply glide; a measured move ends exactly.
        f32 stepLength = speed * 0.6f * kStep;
        if (m.distLeft > 0) stepLength = min(stepLength, m.distLeft);
        o.pos += want * stepLength;
    }
}

/// Counts down timed and measured moves ("ডানে ২ সেকেন্ড হাঁটো", "৩ মিটার দৌড়াও").
void World::advanceOrder(Object& o, f32 moved) {
    Mover& m = o.mover;
    if (!m.moving) return;
    bool finished = false;
    if (m.timeLeft > 0) {
        m.timeLeft -= kStep;
        finished = m.timeLeft <= 1e-4f;
    } else if (m.distLeft > 0) {
        m.distLeft -= moved;
        // A wall in the way must not make the script wait forever.
        m.blocked = moved < m.targetSpeed * kStep * 0.25f ? m.blocked + kStep : 0.0f;
        finished = m.distLeft <= 1e-3f || m.blocked > 0.5f;
    }
    if (finished) {
        m.moving = false;
        m.done = true;
        m.timeLeft = m.distLeft = 0;
    }
}

void World::sense(Object& o) {
    Mover& m = o.mover;
    physics_.contacts(o.body, contactScratch_);
    bool ground = false, wallLeft = false, wallRight = false;
    std::vector<i32> now;
    for (const Contact& c : contactScratch_) {
        if (c.normal.y > 0.6f) ground = true;
        const bool wall = c.otherOwner < 0 || objects_[static_cast<std::size_t>(c.otherOwner)].kind == ObjectKind::Door;
        if (wall && c.normal.x > 0.7f) wallLeft = true;
        if (wall && c.normal.x < -0.7f) wallRight = true;
        if (c.otherOwner >= 0) now.push_back(c.otherOwner);
    }
    const Vec2 half = o.half * o.scale;
    for (const Object& t : objects_) {
        if (!t.alive || !t.sensor || t.id == o.id) continue;
        if (std::fabs(t.pos.x - o.pos.x) < half.x + t.half.x && std::fabs(t.pos.y - o.pos.y) < half.y + t.half.y) now.push_back(t.id);
    }
    std::sort(now.begin(), now.end());
    now.erase(std::unique(now.begin(), now.end()), now.end());

    for (const i32 id : now) {
        if (std::find(o.touching.begin(), o.touching.end(), id) != o.touching.end()) continue;
        o.vm.notifyTouch(id, false);
        Object& other = objects_[static_cast<std::size_t>(id)];
        // Another character notices the touch by itself; everything else is told here.
        if (other.kind != ObjectKind::Character) other.vm.notifyTouch(o.id, false);
    }
    if ((wallLeft && !m.wallLeft) || (wallRight && !m.wallRight)) o.vm.notifyTouch(-1, true);
    if (ground && !m.onGround && physics_.velocity(o.body).y < 0.5f) m.landSquash = 1.0f;

    m.onGround = ground;
    m.wallLeft = wallLeft;
    m.wallRight = wallRight;
    o.touching = std::move(now);
}

// ------------------------------------------------------------------------------------------
// Camera and drawing
// ------------------------------------------------------------------------------------------

/// Metres visible from top to bottom: about 15 m across, but never more than the level is tall.
f32 World::viewHeight() const {
    const f32 wanted = clamp(15.0f / aspect_, 9.5f, 17.0f);
    return height_ > 4 ? min(wanted, static_cast<f32>(height_)) : wanted;
}

void World::updateCamera(bool snap) {
    Vec2 target{width_ * 0.5f, height_ * 0.5f};
    if (const Object* p = player()) target = p->pos + Vec2{static_cast<f32>(p->mover.facing) * 0.8f, 1.2f};
    const f32 halfH = viewHeight() * 0.5f, halfW = halfH * aspect_;
    target.x = width_ > 2 * halfW ? clamp(target.x, halfW, width_ - halfW) : width_ * 0.5f;
    target.y = height_ > 2 * halfH ? clamp(target.y, halfH, height_ - halfH) : height_ * 0.5f;
    camera_ = snap ? target : Vec2{damp(camera_.x, target.x, 5.0f, kStep), damp(camera_.y, target.y, 5.0f, kStep)};

    shakeOffset_ = {};
    if (shakeTime_ > 0) {
        shakeTime_ -= kStep;
        const f32 k = shake_ * 0.45f * saturate(shakeTime_ / shakeDuration_);
        const f32 t = static_cast<f32>(time_);
        shakeOffset_ = {std::sin(t * 97.0f) * k, std::cos(t * 71.0f) * k};
    }
}

void World::bubbles(std::vector<Bubble>& out) const {
    out.clear();
    for (const Object& o : objects_) {
        if (o.alive && o.sayTime > 0 && !o.sayText.empty()) out.push_back({o.pos + Vec2{0, o.size.y * o.scale * 0.5f + 0.25f}, o.sayText});
    }
}

void World::buildFrame(RenderFrame& frame) const {
    frame.clear();
    frame.camera = camera();
    frame.viewHeight = viewHeight();
    frame.ambient = {0.070f, 0.080f, 0.125f, 1.0f};
    frame.background = {0.004f, 0.005f, 0.010f, 1.0f};
    const f32 t = static_cast<f32>(time_);

    // Tiles in view.
    const f32 halfH = frame.viewHeight * 0.5f + 1.0f, halfW = frame.viewHeight * aspect_ * 0.5f + 1.0f;
    const int x0 = max(0, static_cast<int>(std::floor(frame.camera.x - halfW))), x1 = min(width_ - 1, static_cast<int>(std::floor(frame.camera.x + halfW)));
    const int y0 = max(0, static_cast<int>(std::floor(frame.camera.y - halfH))), y1 = min(height_ - 1, static_cast<int>(std::floor(frame.camera.y + halfH)));
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            SpriteDraw s;
            s.pos = {x + 0.5f, y + 0.5f};
            const f32 pick = hash01(static_cast<u32>(x), static_cast<u32>(y), 17);
            Sprite sprite;
            if (solidAt(x, y)) {
                sprite = !solidAt(x, y + 1) && y + 1 < height_ ? Sprite::BrickTop : (pick < 0.75f ? Sprite::BrickA : Sprite::BrickB);
                s.layer = 2;
            } else {
                sprite = pick < 0.6f ? Sprite::BackA : Sprite::BackB;
                s.layer = 0;
            }
            const SpriteRegion r = spriteRegion(sprite);
            s.u0 = r.u0, s.v0 = r.v0, s.u1 = r.u1, s.v1 = r.v1;
            frame.sprites.push_back(s);
        }
    }

    for (const Object& o : objects_) {
        if (!o.alive) continue;
        SpriteDraw s;
        s.pos = o.pos + o.spriteOffset * o.scale;
        s.size = o.size * o.scale;
        s.rotation = o.rotation;
        s.layer = o.layer;
        s.emissive = o.emissive;
        const SpriteRegion r = spriteRegion(o.sprite);
        s.u0 = r.u0, s.v0 = r.v0, s.u1 = r.u1, s.v1 = r.v1;
        if (o.kind == ObjectKind::Character) {
            const Mover& m = o.mover;
            s.flipX = m.facing < 0;
            if (m.targetSpeed > 0 && m.onGround) {  // a little hop and lean while walking
                s.pos.y += std::fabs(std::sin(o.phase * 11.0f)) * 0.05f;
                s.rotation = -static_cast<f32>(m.facing) * 0.07f;
            }
            const f32 squash = m.landSquash * 0.18f;
            s.size.x *= 1.0f + squash;
            s.size.y *= 1.0f - squash;
            s.pos.y -= squash * 0.5f * o.size.y;
        } else if (o.kind == ObjectKind::Pickup) {  // coins spin and bob
            const f32 spin = std::cos(o.phase * 3.2f);
            s.size.x *= max(std::fabs(spin), 0.12f);
            s.flipX = spin < 0;
            s.pos.y += std::sin(o.phase * 2.4f) * 0.06f;
        }
        frame.sprites.push_back(s);

        if (o.kind == ObjectKind::Lamp) {
            // A lamp hangs from whatever is above it.
            const int column = static_cast<int>(std::floor(o.pos.x));
            const int row = static_cast<int>(std::floor(o.pos.y));
            for (int y = row + 1; y <= row + 8 && y < height_; ++y) {
                if (!solidAt(column, y)) continue;
                const f32 top = static_cast<f32>(y), bottom = o.pos.y + o.size.y * o.scale * 0.36f;
                if (top > bottom) {
                    SpriteDraw chain;
                    const SpriteRegion white = spriteRegion(Sprite::White);
                    chain.u0 = white.u0, chain.v0 = white.v0, chain.u1 = white.u1, chain.v1 = white.v1;
                    chain.pos = {o.pos.x, (top + bottom) * 0.5f};
                    chain.size = {0.05f, top - bottom};
                    chain.tint = {0.10f, 0.10f, 0.12f, 1.0f};
                    chain.layer = o.layer - 1;
                    frame.sprites.push_back(chain);
                }
                break;
            }
        }

        if (o.light.present && o.light.on && o.light.intensity > 0.001f) {
            LightDraw l;
            l.pos = o.pos + o.light.offset;
            l.height = o.light.height;
            f32 k = o.light.intensity;
            if (o.light.flicker > 0) k *= 1.0f + o.light.flicker * (std::sin(t * 13.0f + o.phase) * 0.6f + std::sin(t * 31.0f + o.phase * 2.3f) * 0.4f);
            l.color = {o.light.color.r * k, o.light.color.g * k, o.light.color.b * k, 1.0f};
            l.radius = o.light.radius;
            l.size = o.light.size;
            l.shadows = o.light.shadows;
            l.owner = o.id;
            frame.lights.push_back(l);
        }

        if (o.castsShadow) {
            // Slightly smaller than the object, so its lit rim stays visible.
            const Vec2 h = (o.kind == ObjectKind::Character ? Vec2{0.19f, 0.36f} : o.half * 0.94f) * o.scale;
            const Vec2 corners[4] = {{-h.x, -h.y}, {h.x, -h.y}, {h.x, h.y}, {-h.x, h.y}};
            for (int i = 0; i < 4; ++i) {
                frame.occluders.push_back({o.pos + rotate(corners[i], o.rotation), o.pos + rotate(corners[(i + 1) % 4], o.rotation), o.id});
            }
        }
    }
    frame.occluders.insert(frame.occluders.end(), edges_.begin(), edges_.end());
}

void World::objectViews(std::vector<ObjectView>& out) const {
    out.clear();
    out.reserve(objects_.size());
    for (const Object& o : objects_) {
        if (!o.alive) continue;
        ObjectView v;
        v.id = o.id;
        v.key = o.key;
        v.pos = o.pos;
        v.light = o.light.present && o.light.on;
        v.lightRadius = o.light.radius;
        v.lightIntensity = o.light.intensity;
        v.lightColor = o.light.color;
        out.push_back(std::move(v));
    }
}

i32 World::objectAt(Vec2 point) const {
    i32 best = -1;
    f32 bestDistance = 1.21f * 1.21f;  // about one tile away at most
    for (const Object& o : objects_) {
        if (!o.alive) continue;
        const f32 dx = o.pos.x - point.x;
        const f32 dy = o.pos.y - point.y;
        const f32 d = dx * dx + dy * dy;
        if (d < bestDistance) {
            bestDistance = d;
            best = o.id;
        }
    }
    return best;
}

bool World::setLight(i32 id, Color color, f32 radius, f32 intensity) {
    for (Object& o : objects_) {
        if (o.id == id && o.alive) {
            o.light.present = true;
            o.light.on = true;
            o.light.color = color;
            o.light.radius = radius > 0.25f ? (radius < 64.0f ? radius : 64.0f) : 0.25f;
            o.light.intensity = intensity >= 0.0f ? (intensity < 16.0f ? intensity : 16.0f) : 0.0f;
            return true;
        }
    }
    return false;
}

}  // namespace ge
