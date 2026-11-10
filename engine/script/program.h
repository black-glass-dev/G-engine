// G-Engine — script/program.h
// The compiled form of a Kotha (কথা) behaviour script: a list of rules ("when X happens,
// do Y") whose bodies are tiny bytecode programs. Nothing here knows about words any more —
// Bengali and English scripts compile to exactly the same Program.
#pragma once

#include <string>
#include <vector>

#include "core/base.h"

namespace ge::kotha {

enum class Dir : u8 { None, Right, Left, Up, Down, Forward, Backward, Opposite, TowardPlayer, AwayFromPlayer };
enum class Button : u8 { Left, Right, Up, Down, Jump, Action, Count };
enum class Prop : u8 { None, Speed, JumpHeight, Gravity, Size, Brightness, PlayerDistance };
enum class NumOp : u8 { Set, Add, Sub, Mul };
enum class CmpOp : u8 { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
enum class GameEvent : u8 { Win, Lose, Restart };

enum class ActionKind : u8 {
    Move,         // dir, run, scale, seconds | meters (both 0 = keep moving until told otherwise)
    Stop,
    Jump,         // scale (1 = normal)
    Flip,         // turn around
    Face,         // dir
    SetProp,      // prop, op, value
    SetVar,       // text = variable, op, value
    LightSwitch,  // value = 1 (on) / 0 (off)
    LightColor,   // value = 0xRRGGBB
    Say,          // text, seconds
    Send,         // text = message
    Destroy,      // text = object name ("" = myself)
    Respawn,      // go back to where I started
    Shake,        // camera shake: value = strength, seconds
    Game,         // value = GameEvent
};

struct Action {
    ActionKind kind = ActionKind::Stop;
    Dir dir = Dir::None;
    bool run = false;
    f32 scale = 1.0f;
    f32 seconds = 0.0f;
    f32 meters = 0.0f;
    Prop prop = Prop::None;
    NumOp op = NumOp::Set;
    f64 value = 0.0;
    std::string text;   // key (variable, message, object name) or the words to say
    std::string label;  // how the name was written, for display
    i32 line = 0;       // source line (1-based)
};

enum class TriggerKind : u8 {
    Start,           // event: the game starts
    ButtonDown,      // state: a button is held
    ButtonReleased,  // event
    Every,           // event: every `seconds`
    Touch,           // event: started touching `name` ("" = anything, "#wall", "#player")
    Landed,          // event: touched the ground after being in the air
    Message,         // event: message `name` received
    OnGround,        // state
    Compare,         // state: property or variable compared with `value`
};

struct Trigger {
    TriggerKind kind = TriggerKind::Start;
    bool negate = false;  // state triggers only
    Button button = Button::Jump;
    f32 seconds = 0.0f;
    std::string name;
    std::string label;       // how the name was written, for display
    Prop prop = Prop::None;  // Compare: None means the variable `name`
    CmpOp cmp = CmpOp::Equal;
    f64 value = 0.0;

    bool isState() const {
        return kind == TriggerKind::ButtonDown || kind == TriggerKind::OnGround || kind == TriggerKind::Compare;
    }
};

enum class Op : u8 {
    End,         // the rule body is finished
    Act,         // a = index into Program::actions
    Wait,        // f = seconds
    WaitMove,    // until the current timed / measured move has finished
    WaitLanded,  // until back on the ground
    Yield,       // until the next tick
    Jump,        // a = target
    JumpIfNot,   // a = index into Program::conditions, b = target
    LoopBegin,   // a = count
    LoopEnd,     // b = first instruction of the loop body
};

struct Instr {
    Op op = Op::End;
    i32 a = 0;
    i32 b = 0;
    f32 f = 0.0f;
};

struct Rule {
    Trigger trigger;
    i32 entry = 0;            // first instruction
    i32 line = 0;             // source line (1-based)
    bool holdsMove = false;   // body starts an open-ended move (stopped when a state trigger ends)
    bool stopOnFall = false;  // body loops forever (cancelled when a state trigger ends)
};

/// "This is how I understood your line" — shown next to the script in the editor.
struct Note {
    i32 line = 0;
    std::string bn;
    std::string en;
};

struct Diagnostic {
    i32 line = 0;
    i32 column = 0;
    std::string bn;
    std::string en;
};

struct Program {
    std::vector<Rule> rules;
    std::vector<Instr> code;
    std::vector<Action> actions;
    std::vector<Trigger> conditions;
    std::vector<Note> notes;
    std::vector<Diagnostic> errors;

    bool ok() const { return errors.empty(); }
};

}  // namespace ge::kotha
