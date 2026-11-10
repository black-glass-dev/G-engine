// G-Engine — scene/world.h
// The game world: a tile map, objects (characters, crates, coins, lamps ...), their Kotha
// scripts, and the glue between scripts, physics and rendering.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/math.h"
#include "gfx/atlas_gen.h"
#include "gfx/renderer.h"
#include "physics/physics.h"
#include "script/program.h"
#include "script/vm.h"

namespace ge {

enum class ObjectKind : u8 { Character, Crate, Pickup, Hazard, Door, Lamp };

/// How a character moves. All values can be changed from scripts ("গতি ৬ করো").
struct Mover {
    f32 speed = 4.2f;       // m/s when walking; running is 1.6x
    f32 jumpHeight = 2.6f;  // m
    f32 gravityScale = 1.0f;
    i32 facing = 1;         // +1 = right, -1 = left

    // the current order
    bool moving = false;
    kotha::Dir dir = kotha::Dir::None;
    bool run = false;
    f32 scale = 1.0f;
    f32 timeLeft = 0;  // > 0: a timed move
    f32 distLeft = 0;  // > 0: a measured move
    bool done = true;
    f32 blocked = 0;

    // state
    bool onGround = false;
    bool wallLeft = false, wallRight = false;
    f32 coyote = 0;      // time after leaving the ground during which a jump still works
    f32 jumpBuffer = 0;  // time a jump order is remembered before landing
    f32 jumpScale = 1.0f;
    f32 landSquash = 0;
    f32 targetSpeed = 0;
};

struct LightPart {
    bool present = false;
    bool on = true;
    Color color{1.0f, 0.78f, 0.45f, 1.0f};
    f32 intensity = 1.0f;
    f32 radius = 8.0f;
    f32 height = 1.2f;
    f32 size = 0.3f;
    Vec2 offset;
    bool shadows = true;
    f32 flicker = 0.0f;
};

struct Object {
    i32 id = -1;
    bool alive = true;
    ObjectKind kind = ObjectKind::Crate;
    i32 script = -1;  // which script slot drives this object
    std::string key;  // canonical name: what scripts call it

    Vec2 pos, spawn;
    Vec2 before;          // position at the start of the step
    Vec2 size{1, 1};      // sprite size in metres
    Vec2 half{0.5f, 0.5f};  // collision half extents
    f32 rotation = 0;
    f32 scale = 1.0f;
    Sprite sprite = Sprite::Crate;
    Vec2 spriteOffset;
    i32 layer = 3;
    f32 emissive = 1.0f;

    BodyId body = 0;
    bool sensor = false;      // touched by overlap, does not block
    bool castsShadow = false;
    Mover mover;
    LightPart light;
    kotha::Vm vm;

    std::string sayText;
    f32 sayTime = 0;
    std::vector<i32> touching;  // objects touched on the last step
    f32 phase = 0;              // animation clock
};

struct ScriptSlot {
    std::string name;    // shown to the user
    std::string key;
    std::string source;
    std::string original;
    kotha::Program program;
};

enum class GameState : u8 { Playing, Won, Lost };

struct Bubble {
    Vec2 world;
    std::string text;
};

class World {
public:
    static constexpr f32 kStep = 1.0f / 60.0f;

    static constexpr int kMaxLevelWidth = 256;   // larger pictures are cut off (with a warning)
    static constexpr int kMaxLevelHeight = 128;
    static constexpr int kMaxObjects = 2000;

    /// The level is a picture made of characters (see games/demo/demo.cpp for the legend).
    /// Loading keeps the scripts and starts the level from the beginning.
    void load(const std::string& level);
    const std::string& level() const { return level_; }
    void restart();
    void step();
    void buildFrame(RenderFrame& frame) const;
    void setAspect(f32 aspect) { aspect_ = aspect > 0.05f ? aspect : 1.0f; }

    /// The names of everything a level picture can contain (what scripts may talk about).
    static std::vector<std::string> builtinNames();

    // ---- scripts ----
    int addScript(const std::string& name, const std::string& source);
    int scriptCount() const { return static_cast<int>(scripts_.size()); }
    const ScriptSlot& script(int index) const { return scripts_[static_cast<std::size_t>(index)]; }
    /// Compiles without changing anything (used to show notes and errors while typing).
    kotha::Program check(const std::string& source) const;
    /// Compiles and, when there are no errors, installs the script and restarts the level.
    /// A script with errors changes nothing: the one before keeps running (and stays `source`).
    kotha::Program setScript(int index, const std::string& source);
    void resetScripts();

    // ---- input & state ----
    void setButton(kotha::Button button, bool down) { buttons_[static_cast<int>(button)] = down; }
    bool button(kotha::Button button) const { return buttons_[static_cast<int>(button)]; }
    f64 variable(const std::string& key) const;
    const std::unordered_map<std::string, f64>& variables() const { return vars_; }
    GameState state() const { return state_; }
    void bubbles(std::vector<Bubble>& out) const;
    Vec2 camera() const { return camera_ + shakeOffset_; }
    f32 viewHeight() const;
    f64 time() const { return time_; }

    // ---- access (tests, editor) ----
    const std::vector<Object>& objects() const { return objects_; }
    const Object* player() const { return player_ >= 0 ? &objects_[static_cast<std::size_t>(player_)] : nullptr; }
    const Object* find(const std::string& name) const;
    int width() const { return width_; }
    int height() const { return height_; }
    bool solidAt(int x, int y) const;

private:
    friend class ObjectHost;

    Object& spawn(ObjectKind kind, const std::string& name, Vec2 pos);
    void createBody(Object& o);
    void kill(Object& o);
    void runMover(Object& o);
    void advanceOrder(Object& o, f32 moved);
    void sense(Object& o);
    void updateCamera(bool snap);
    void buildEdges();
    std::vector<std::string> knownNames() const;

    std::string level_;
    int width_ = 0, height_ = 0;
    std::vector<u8> tiles_;
    std::vector<Segment> edges_;  // outline of the solid tiles, for shadows

    std::vector<Object> objects_;
    std::vector<ScriptSlot> scripts_;
    PhysicsWorld physics_;
    i32 player_ = -1;

    bool buttons_[static_cast<int>(kotha::Button::Count)] = {};
    std::unordered_map<std::string, f64> vars_;
    std::vector<std::string> messages_, nextMessages_;
    GameState state_ = GameState::Playing;
    bool restartRequested_ = false;
    f64 time_ = 0;

    f32 aspect_ = 16.0f / 9.0f;
    Vec2 camera_;
    Vec2 shakeOffset_;
    f32 shake_ = 0, shakeTime_ = 0, shakeDuration_ = 0;
    std::vector<Contact> contactScratch_;
};

}  // namespace ge
