// G-Engine — script/vm.h
// Runs a compiled Kotha (কথা) Program. The VM knows nothing about the engine: everything it
// reads or does goes through the Host interface, so it can be tested without a game.
#pragma once

#include <string>
#include <vector>

#include "script/program.h"

namespace ge::kotha {

/// What a script can sense and do. Implemented by the game object that owns the script.
class Host {
public:
    virtual ~Host() = default;

    // ---- senses ----
    virtual bool buttonDown(Button button) = 0;
    virtual bool onGround() = 0;
    virtual bool moveDone() = 0;  // has the last timed / measured move finished?
    virtual f64 getProp(Prop prop) = 0;
    virtual f64 getVar(const std::string& key) = 0;
    virtual bool touchMatches(i32 other, const std::string& key) = 0;  // key: normalised name, or "#player"

    // ---- actions ----
    virtual void move(Dir dir, bool run, f32 speedScale, f32 seconds, f32 meters) = 0;
    virtual void stop() = 0;
    virtual void jump(f32 scale) = 0;
    virtual void face(Dir dir) = 0;  // Dir::Opposite = turn around
    virtual void setProp(Prop prop, f64 value) = 0;
    virtual void setVar(const std::string& key, f64 value) = 0;
    virtual void lightSwitch(bool on) = 0;
    virtual void lightColor(u32 rgb) = 0;
    virtual void say(const std::string& text, f32 seconds) = 0;
    virtual void send(const std::string& message) = 0;
    virtual void destroy(const std::string& key) = 0;  // "" = myself
    virtual void respawn() = 0;
    virtual void shake(f32 strength, f32 seconds) = 0;
    virtual void gameEvent(GameEvent event) = 0;
};

class Vm {
public:
    /// The program must outlive the VM (or the next load()).
    void load(const Program* program);
    void reset();

    /// Events from the engine; they are delivered to the rules on the next update().
    void notifyTouch(i32 other, bool wall);
    void notifyMessage(const std::string& key);

    /// One fixed simulation step.
    void update(Host& host, f32 dt);

    int runningTasks() const;
    bool hasProgram() const { return program_ != nullptr; }

private:
    enum class WaitKind : u8 { None, Time, Move, Landed, Yield };

    struct Task {
        bool active = false;
        i32 pc = 0;
        WaitKind wait = WaitKind::None;
        f32 timer = 0;
        i32 loops[8] = {};
        i32 loopDepth = 0;
    };

    struct RuleState {
        bool previous = false;  // state triggers: value on the last tick; ButtonReleased: was down
        f32 timer = 0;          // Every
        Task task;
    };

    struct TouchEvent {
        i32 other;
        bool wall;
    };

    void start(std::size_t rule);
    void run(std::size_t rule, Host& host, f32 dt);
    void perform(std::size_t rule, const Action& action, Host& host);
    bool test(const Trigger& trigger, Host& host) const;

    const Program* program_ = nullptr;
    std::vector<RuleState> states_;
    std::vector<TouchEvent> touches_;
    std::vector<std::string> messages_;
    bool started_ = false;
    bool wasOnGround_ = true;
    i32 moveOwner_ = -1;  // rule whose open-ended move is currently in effect
};

}  // namespace ge::kotha
