// G-Engine — script/vm.cpp
#include "script/vm.h"

#include <cmath>

#include "core/log.h"

namespace ge::kotha {

namespace {

bool compare(f64 lhs, CmpOp op, f64 rhs) {
    const f64 eps = 1e-6;
    switch (op) {
        case CmpOp::Equal: return std::fabs(lhs - rhs) <= eps;
        case CmpOp::NotEqual: return std::fabs(lhs - rhs) > eps;
        case CmpOp::Less: return lhs < rhs - eps;
        case CmpOp::LessEqual: return lhs <= rhs + eps;
        case CmpOp::Greater: return lhs > rhs + eps;
        case CmpOp::GreaterEqual: return lhs >= rhs - eps;
    }
    return false;
}

f64 apply(f64 current, NumOp op, f64 value) {
    switch (op) {
        case NumOp::Set: return value;
        case NumOp::Add: return current + value;
        case NumOp::Sub: return current - value;
        case NumOp::Mul: return current * value;
    }
    return value;
}

}  // namespace

void Vm::load(const Program* program) {
    program_ = program;
    reset();
}

void Vm::reset() {
    states_.assign(program_ != nullptr ? program_->rules.size() : 0, RuleState{});
    touches_.clear();
    messages_.clear();
    started_ = false;
    wasOnGround_ = true;
    moveOwner_ = -1;
}

void Vm::notifyTouch(i32 other, bool wall) {
    if (program_ != nullptr && touches_.size() < 64) touches_.push_back({other, wall});
}

void Vm::notifyMessage(const std::string& key) {
    if (program_ != nullptr && messages_.size() < 64) messages_.push_back(key);
}

int Vm::runningTasks() const {
    int count = 0;
    for (const RuleState& s : states_) count += s.task.active ? 1 : 0;
    return count;
}

void Vm::start(std::size_t rule) {
    Task& task = states_[rule].task;
    task = Task{};
    task.active = true;
    task.pc = program_->rules[rule].entry;
}

bool Vm::test(const Trigger& t, Host& host) const {
    bool value = false;
    switch (t.kind) {
        case TriggerKind::ButtonDown: value = host.buttonDown(t.button); break;
        case TriggerKind::OnGround: value = host.onGround(); break;
        case TriggerKind::Compare: {
            const f64 lhs = t.prop != Prop::None ? host.getProp(t.prop) : host.getVar(t.name);
            value = compare(lhs, t.cmp, t.value);
            break;
        }
        default: return false;
    }
    return value != t.negate;
}

void Vm::update(Host& host, f32 dt) {
    if (program_ == nullptr) return;
    const std::vector<Rule>& rules = program_->rules;
    const bool ground = host.onGround();
    bool moveReleased = false;

    // 1. Which rules fire on this tick?
    for (std::size_t i = 0; i < rules.size(); ++i) {
        const Trigger& t = rules[i].trigger;
        RuleState& s = states_[i];
        bool fire = false;
        switch (t.kind) {
            case TriggerKind::Start: fire = !started_; break;
            case TriggerKind::ButtonReleased: {
                const bool down = host.buttonDown(t.button);
                fire = s.previous && !down;
                s.previous = down;
                break;
            }
            case TriggerKind::Every:
                s.timer += dt;
                if (s.timer >= t.seconds - 1e-4f) {
                    s.timer = std::fmod(s.timer, t.seconds);
                    if (s.timer > t.seconds - 1e-3f) s.timer = 0;
                    fire = true;
                }
                break;
            case TriggerKind::Touch:
                for (const TouchEvent& e : touches_) {
                    if (t.name.empty() || (t.name == "#wall" ? e.wall : (!e.wall && host.touchMatches(e.other, t.name)))) {
                        fire = true;
                        break;
                    }
                }
                break;
            case TriggerKind::Landed: fire = started_ && ground && !wasOnGround_; break;
            case TriggerKind::Message:
                for (const std::string& m : messages_) {
                    if (m == t.name) {
                        fire = true;
                        break;
                    }
                }
                break;
            case TriggerKind::ButtonDown:
            case TriggerKind::OnGround:
            case TriggerKind::Compare: {
                const bool now = test(t, host);
                if (now && !s.previous) fire = true;
                if (!now && s.previous) {
                    // The state ended: an open-ended move started by this rule ends with it
                    // ("ডান বোতাম চাপলে ডানে হাঁটো" stops when the button is released).
                    if (moveOwner_ == static_cast<i32>(i)) {
                        host.stop();
                        moveOwner_ = -1;
                        moveReleased = true;
                    }
                    if (rules[i].stopOnFall) s.task.active = false;
                }
                s.previous = now;
                break;
            }
        }
        if (fire) start(i);
    }

    // Another held state (e.g. the other direction button) takes over a move that just ended.
    if (moveReleased) {
        for (std::size_t i = 0; i < rules.size(); ++i) {
            if (rules[i].trigger.isState() && rules[i].holdsMove && states_[i].previous && !states_[i].task.active) start(i);
        }
    }

    started_ = true;
    wasOnGround_ = ground;
    touches_.clear();
    messages_.clear();

    // 2. Advance every running rule body.
    for (std::size_t i = 0; i < rules.size(); ++i) {
        if (states_[i].task.active) run(i, host, dt);
    }
}

void Vm::run(std::size_t rule, Host& host, f32 dt) {
    Task& task = states_[rule].task;
    const std::vector<Instr>& code = program_->code;

    switch (task.wait) {
        case WaitKind::None: break;
        case WaitKind::Time:
            task.timer -= dt;
            if (task.timer > 1e-4f) return;
            break;
        case WaitKind::Move:
            if (!host.moveDone()) return;
            break;
        case WaitKind::Landed:
            task.timer -= dt;
            if (task.timer > 0 || !host.onGround()) return;
            break;
        case WaitKind::Yield: break;
    }
    task.wait = WaitKind::None;

    // A budget keeps a badly written loop from freezing the game.
    for (int budget = 0; budget < 512 && task.active; ++budget) {
        if (task.pc < 0 || task.pc >= static_cast<i32>(code.size())) {
            task.active = false;
            return;
        }
        const Instr& in = code[static_cast<std::size_t>(task.pc++)];
        switch (in.op) {
            case Op::End: task.active = false; return;
            case Op::Act: perform(rule, program_->actions[static_cast<std::size_t>(in.a)], host); break;
            case Op::Wait:
                task.wait = WaitKind::Time;
                task.timer = in.f;
                return;
            case Op::WaitMove: task.wait = WaitKind::Move; return;
            case Op::WaitLanded:
                task.wait = WaitKind::Landed;
                task.timer = 0.15f;  // give the jump time to leave the ground
                return;
            case Op::Yield: task.wait = WaitKind::Yield; return;
            case Op::Jump: task.pc = in.a; break;
            case Op::JumpIfNot:
                if (!test(program_->conditions[static_cast<std::size_t>(in.a)], host)) task.pc = in.b;
                break;
            case Op::LoopBegin:
                if (task.loopDepth < static_cast<i32>(GE_ARRAY_COUNT(task.loops))) {
                    task.loops[task.loopDepth++] = in.a;
                } else {
                    GE_LOG_WARN("kotha: loops nested too deeply");
                    task.active = false;
                }
                break;
            case Op::LoopEnd:
                if (task.loopDepth > 0 && --task.loops[task.loopDepth - 1] > 0) {
                    task.pc = in.b;
                } else if (task.loopDepth > 0) {
                    --task.loopDepth;
                }
                break;
        }
    }
    // Out of budget: continue on the next tick.
    if (task.active && task.wait == WaitKind::None) task.wait = WaitKind::Yield;
}

void Vm::perform(std::size_t rule, const Action& a, Host& host) {
    switch (a.kind) {
        case ActionKind::Move:
            host.move(a.dir, a.run, a.scale, a.seconds, a.meters);
            moveOwner_ = (a.seconds <= 0 && a.meters <= 0) ? static_cast<i32>(rule) : -1;
            break;
        case ActionKind::Stop:
            host.stop();
            moveOwner_ = -1;
            break;
        case ActionKind::Jump: host.jump(a.scale); break;
        case ActionKind::Flip: host.face(Dir::Opposite); break;
        case ActionKind::Face: host.face(a.dir); break;
        case ActionKind::SetProp: host.setProp(a.prop, apply(host.getProp(a.prop), a.op, a.value)); break;
        case ActionKind::SetVar: host.setVar(a.text, apply(host.getVar(a.text), a.op, a.value)); break;
        case ActionKind::LightSwitch: host.lightSwitch(a.value != 0.0); break;
        case ActionKind::LightColor: host.lightColor(static_cast<u32>(a.value)); break;
        case ActionKind::Say: host.say(a.text, a.seconds); break;
        case ActionKind::Send: host.send(a.text); break;
        case ActionKind::Destroy: host.destroy(a.text); break;
        case ActionKind::Respawn: host.respawn(); break;
        case ActionKind::Shake: host.shake(static_cast<f32>(a.value), a.seconds); break;
        case ActionKind::Game: host.gameEvent(static_cast<GameEvent>(static_cast<int>(a.value))); break;
    }
}

}  // namespace ge::kotha
