// G-Engine — tests/test_game.cpp
// The whole engine without a screen: physics, character controller, scripts driving objects,
// and the renderer's bookkeeping (drawing goes to the do-nothing graphics backend).
#include <cmath>
#include <cstdio>
#include <string>

#include "app/app.h"
#include "gfx/atlas_gen.h"
#include "physics/physics.h"
#include "scene/world.h"
#include "script/compiler.h"
#include "test_framework.h"

using namespace ge;
using kotha::Button;

namespace {

void run(World& world, f32 seconds) {
    const int steps = static_cast<int>(seconds * 60.0f + 0.5f);
    for (int i = 0; i < steps; ++i) world.step();
}

/// A flat room with one object of each kind that the tests need.
const char* const kRoom =
    "####################\n"
    "#..................#\n"
    "#..................#\n"
    "#..................#\n"
    "#..P.....c.........#\n"
    "####################\n";

void makeRoom(World& world, const std::string& playerScript, const char* level = kRoom) {
    world.addScript("খেলোয়াড়", playerScript);
    world.addScript("কয়েন", "খেলোয়াড়কে ছুঁলে মুছে যাও\n");
    world.addScript("প্রহরী", "");
    world.addScript("দরজা", "");
    world.addScript("বাক্স", "");
    world.load(level);
}

}  // namespace

GE_TEST(physics_box_falls_and_rests) {
    PhysicsWorld physics;
    physics.create({0, -10});
    BodyDesc ground;
    ground.pos = {0, -1};
    ground.half = {20, 1};
    physics.addBody(ground);
    BodyDesc box;
    box.type = BodyDesc::Type::Dynamic;
    box.pos = {0, 5};
    box.half = {0.5f, 0.5f};
    box.owner = 7;
    const BodyId id = physics.addBody(box);
    for (int i = 0; i < 240; ++i) physics.step(1.0f / 60.0f);
    CHECK_NEAR(physics.position(id).y, 0.5, 0.03);
    CHECK_NEAR(physics.velocity(id).y, 0.0, 0.05);
    std::vector<Contact> contacts;
    physics.contacts(id, contacts);
    CHECK(!contacts.empty());
    CHECK(!contacts.empty() && contacts[0].normal.y > 0.9f);  // the ground pushes the box up
    CHECK(!contacts.empty() && contacts[0].otherStatic);
    physics.removeBody(id);
    CHECK_EQ(physics.bodyCount(), 1);
}

GE_TEST(game_character_stands_walks_and_stops) {
    World world;
    makeRoom(world, "ডান বোতাম চাপলে ডানে হাঁটো\nবাম বোতাম চাপলে বামে হাঁটো\nলাফ বোতাম চাপলে লাফ দাও\n");
    run(world, 0.5f);
    const Object* p = world.player();
    CHECK(p != nullptr);
    CHECK(p->mover.onGround);
    const f32 x0 = p->pos.x;
    CHECK_NEAR(p->pos.y, 1.42, 0.03);  // standing on the floor (top at y = 1), half height 0.42

    world.setButton(Button::Right, true);
    run(world, 1.0f);
    const f32 x1 = world.player()->pos.x;
    CHECK_MSG(x1 - x0 > 3.5f && x1 - x0 < 4.4f, "walked " + std::to_string(x1 - x0) + " m in 1 s at 4.2 m/s");
    CHECK_EQ(world.player()->mover.facing, 1);

    world.setButton(Button::Right, false);
    run(world, 0.5f);
    const f32 x2 = world.player()->pos.x;
    run(world, 0.5f);
    CHECK_NEAR(world.player()->pos.x, x2, 0.01);  // really stopped

    world.setButton(Button::Left, true);
    run(world, 0.5f);
    CHECK(world.player()->pos.x < x2 - 1.5f);
    CHECK_EQ(world.player()->mover.facing, -1);
}

GE_TEST(game_jump_reaches_the_configured_height) {
    World world;
    makeRoom(world, "লাফ বোতাম চাপলে লাফ দাও\n");
    run(world, 0.5f);
    const f32 ground = world.player()->pos.y;
    world.setButton(Button::Jump, true);
    f32 top = ground;
    bool leftGround = false;
    for (int i = 0; i < 120; ++i) {
        world.step();
        top = max(top, world.player()->pos.y);
        leftGround = leftGround || !world.player()->mover.onGround;
    }
    CHECK(leftGround);
    CHECK_MSG(std::fabs((top - ground) - 2.6f) < 0.25f, "jumped " + std::to_string(top - ground) + " m, expected about 2.6");
    CHECK(world.player()->mover.onGround);                    // back on the floor
    CHECK_NEAR(world.player()->pos.y, ground, 0.03);          // holding the button does not bounce forever
}

GE_TEST(game_a_jump_ordered_just_before_landing_still_happens) {
    const std::string script = "লাফ বোতাম চাপলে লাফ দাও\n";
    // One jump; `again` steps after the first press the button is pressed a second time (never,
    // when negative). Returns how high the player got after landing; `landedAt` is the landing step.
    auto secondJump = [&](int again, int& landedAt) {
        World world;
        makeRoom(world, script);
        run(world, 0.5f);
        const f32 ground = world.player()->pos.y;
        world.setButton(Button::Jump, true);
        landedAt = -1;
        bool left = false;
        f32 top = ground;
        for (int i = 0; i < 240; ++i) {
            world.step();
            if (i == 2) world.setButton(Button::Jump, false);
            if (i == again) world.setButton(Button::Jump, true);
            const bool onGround = world.player()->mover.onGround;
            left = left || !onGround;
            if (landedAt < 0 && left && onGround) landedAt = i;
            if (landedAt >= 0) top = max(top, world.player()->pos.y);
        }
        return top - ground;
    };
    int landing = -1, unused = -1;
    CHECK(secondJump(-1, landing) < 0.05f);  // one press, one jump
    CHECK_MSG(landing > 30, "landed at step " + std::to_string(landing));
    const f32 justBefore = secondJump(landing - 5, unused);  // 0.08 s before touching down: remembered
    const f32 tooEarly = secondJump(landing - 25, unused);   // 0.42 s before: forgotten by then
    CHECK_MSG(justBefore > 2.0f, "pressed 5 steps before landing, then jumped " + std::to_string(justBefore) + " m");
    CHECK_MSG(tooEarly < 0.05f, "pressed 25 steps before landing, then jumped " + std::to_string(tooEarly) + " m");
}

GE_TEST(game_jump_still_works_just_after_walking_off_a_ledge) {
    const char* const ledge =
        "####################\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "#..P...............#\n"
        "######.............#\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "####################\n";
    const std::string script = "ডান বোতাম চাপলে ডানে হাঁটো\nলাফ বোতাম চাপলে লাফ দাও\n";
    // Walks right, off the ledge; `after` steps after leaving the ground the jump button is pressed
    // (never, when negative). Returns how far the player rose in the next 10 steps.
    auto rise = [&](int after, int& leftAt) {
        World world;
        makeRoom(world, script, ledge);
        run(world, 0.5f);
        world.setButton(Button::Right, true);
        leftAt = -1;
        f32 from = 0, top = 0;
        for (int i = 0; i < 240; ++i) {
            world.step();
            const Object* p = world.player();
            if (leftAt < 0 && !p->mover.onGround) leftAt = i;
            if (leftAt < 0 || after < 0) continue;
            if (i == leftAt + after) {
                world.setButton(Button::Jump, true);
                from = top = p->pos.y;
            } else if (i > leftAt + after && i <= leftAt + after + 10) {
                top = max(top, p->pos.y);
            }
        }
        return top - from;
    };
    int leftAt = -1;
    rise(-1, leftAt);
    CHECK_MSG(leftAt > 10, "left the ground at step " + std::to_string(leftAt));
    const f32 soon = rise(3, leftAt);   // 0.05 s past the edge: inside the 0.1 s of grace
    const f32 late = rise(12, leftAt);  // 0.2 s past the edge: too late, the fall goes on
    CHECK_MSG(soon > 1.0f, "pressed 3 steps past the edge, rose " + std::to_string(soon) + " m");
    CHECK_MSG(late < 0.001f, "pressed 12 steps past the edge, rose " + std::to_string(late) + " m");
}

GE_TEST(game_timed_and_measured_moves) {
    {
        World world;
        makeRoom(world, "ডানে ৩ মিটার হাঁটো\n\"পৌঁছেছি\" বলো\n");
        const f32 x0 = world.player()->pos.x;
        run(world, 3.0f);
        CHECK_MSG(std::fabs(world.player()->pos.x - x0 - 3.0f) < 0.35f, "moved " + std::to_string(world.player()->pos.x - x0) + " m, expected 3");
        CHECK_EQ(world.player()->sayText, std::string("পৌঁছেছি"));
    }
    {
        // A wall in the way does not block the script forever.
        World world;
        makeRoom(world, "বামে ১০ মিটার হাঁটো\n\"দেয়াল\" বলো\n");
        run(world, 3.0f);
        CHECK_EQ(world.player()->sayText, std::string("দেয়াল"));
        CHECK(world.player()->pos.x < 1.6f);
    }
}

GE_TEST(game_touching_things) {
    World world;
    makeRoom(world, "ডানে হাঁটো\nকয়েন ছুঁলে স্কোর ১ বাড়াও\nদেয়ালে ধাক্কা লাগলে ঘুরে যাও\n");
    CHECK(world.find("কয়েন") != nullptr);
    CHECK(world.find("coin") != nullptr);  // the English name means the same object
    run(world, 2.5f);
    CHECK(world.find("কয়েন") == nullptr);  // the coin's own script removed it
    CHECK_NEAR(world.variable(kotha::canonicalName("score")), 1.0, 1e-9);
    run(world, 2.0f);                      // t = 4.5 s: reached the right wall (after 3.6 s) and turned round
    CHECK_EQ(world.player()->mover.facing, -1);
    CHECK(world.player()->pos.x < 17.5f);
    run(world, 4.0f);                      // t = 8.5 s: and again at the left wall (after 7.8 s)
    CHECK_EQ(world.player()->mover.facing, 1);
    CHECK_NEAR(world.variable(kotha::canonicalName("score")), 1.0, 1e-9);
}

GE_TEST(game_lights_and_properties_from_scripts) {
    World world;
    makeRoom(world, "আলো নেভাও\n১ সেকেন্ড অপেক্ষা করো\nআলোর রং লাল করো\nআলো জ্বালাও\nগতি ৮ করো\nডানে ১ সেকেন্ড হাঁটো\n");
    run(world, 0.5f);
    CHECK(!world.player()->light.on);
    const f32 x0 = world.player()->pos.x;
    run(world, 1.0f);
    CHECK(world.player()->light.on);
    CHECK(world.player()->light.color.r > 0.8f && world.player()->light.color.b < 0.1f);
    run(world, 1.5f);
    CHECK_MSG(world.player()->pos.x - x0 > 6.5f, "at speed 8 the player covers about 8 m in 1 s");
}

GE_TEST(game_things_without_a_body_glide) {
    const char* level =
        "####################\n"
        "#......L...........#\n"
        "#..................#\n"
        "#..P...............#\n"
        "####################\n";
    World world;
    world.addScript("বাতি", "বারবার:\n    ডানে ৩ মিটার যাও\n    উপরে ১ মিটার যাও\n    বামে ৩ মিটার যাও\n    নিচে ১ মিটার যাও\n");
    world.load(level);
    const Vec2 start = world.find("বাতি")->pos;
    f32 maxX = start.x, maxY = start.y;
    for (int i = 0; i < 60 * 8; ++i) {
        world.step();
        maxX = max(maxX, world.find("বাতি")->pos.x);
        maxY = max(maxY, world.find("বাতি")->pos.y);
    }
    CHECK_NEAR(maxX - start.x, 3.0, 0.02);  // exactly the distance that was asked for
    CHECK_NEAR(maxY - start.y, 1.0, 0.02);
    // After a whole number of rounds the lamp is back on its path (no drift).
    const Vec2 now = world.find("বাতি")->pos;
    CHECK(now.x >= start.x - 0.02f && now.x <= start.x + 3.02f && now.y >= start.y - 0.02f && now.y <= start.y + 1.02f);
}

GE_TEST(game_messages_between_objects) {
    const char* level =
        "####################\n"
        "#..................#\n"
        "#..P.........D.....#\n"
        "####################\n";
    World world;
    world.addScript("খেলোয়াড়", "২ সেকেন্ড অপেক্ষা করো\n\"খোলো\" বার্তা পাঠাও\n");
    world.addScript("দরজা", "\"খোলো\" বার্তা পেলে মুছে যাও\n");
    world.load(level);
    run(world, 1.5f);
    CHECK(world.find("দরজা") != nullptr);
    run(world, 1.0f);
    CHECK(world.find("দরজা") == nullptr);
}

GE_TEST(game_changing_a_script_restarts_the_level) {
    World world;
    makeRoom(world, "ডানে হাঁটো\n");
    run(world, 1.0f);
    const f32 moved = world.player()->pos.x;
    kotha::Program bad = world.setScript(0, "ডানে হাঁটোো\n");
    CHECK(!bad.ok());
    run(world, 0.2f);
    CHECK(world.player()->pos.x > moved);  // the broken script was not installed: the old one keeps running
    CHECK(world.script(0).source == "ডানে হাঁটো\n" && world.script(0).program.ok());

    kotha::Program good = world.setScript(0, "বামে হাঁটো\n");
    CHECK(good.ok());
    CHECK(world.script(0).source == "বামে হাঁটো\n");
    CHECK_NEAR(world.player()->pos.x, 3.5, 0.01);  // back at the start
    run(world, 0.5f);
    CHECK(world.player()->pos.x < 3.0f);

    world.resetScripts();
    CHECK(world.script(0).source == "ডানে হাঁটো\n");
    run(world, 0.5f);
    CHECK(world.player()->pos.x > 4.0f);
}

GE_TEST(game_demo_runs_and_renders_headless) {
    App app;
    CHECK(app.init(960, 540));
    loadDemo(app.world());
    World& world = app.world();
    for (int i = 0; i < world.scriptCount(); ++i) {
        CHECK_MSG(world.script(i).program.ok(), "demo script '" + world.script(i).name + "' must compile: " +
                                                   (world.script(i).program.ok() ? std::string() : world.script(i).program.errors[0].en));
    }
    CHECK(world.player() != nullptr);
    CHECK(world.find("প্রহরী") != nullptr);

    world.setButton(Button::Right, true);
    for (int i = 0; i < 180; ++i) app.frame(1.0f / 60.0f);
    CHECK(world.player()->pos.x > 8.0f);
    const RenderStats& stats = app.renderer().stats();
    CHECK(stats.sprites > 100);
    CHECK(stats.lights >= 3);
    CHECK(stats.shadowLights >= 2);
    CHECK(stats.shadowEdges > 4);
    CHECK_MSG(stats.drawCalls <= 16, "a frame takes " + std::to_string(stats.drawCalls) + " draw calls");
    const std::string hud = app.hud();
    CHECK(hud.find("\"state\":\"playing\"") != std::string::npos);
    CHECK(hud.find("\"sprites\":") != std::string::npos);

    // The guard's loop moves it around its post.
    f32 minX = 1e9f, maxX = -1e9f;
    for (int i = 0; i < 60 * 7; ++i) {
        world.step();
        if (const Object* g = world.find("প্রহরী")) {
            minX = min(minX, g->pos.x);
            maxX = max(maxX, g->pos.x);
        }
    }
    CHECK_MSG(maxX - minX > 3.0f, "the guard patrolled over " + std::to_string(maxX - minX) + " m");
}

namespace {

/// A simple robot that plays the sample level through the same buttons a person uses.
/// Returns how often it was sent back to the start. Stops when the game ends, when time is
/// up, or when the player has reached `untilX`.
int playDemo(World& world, bool collectOnPlatforms, f32 maxSeconds, f32& seconds, f32 untilX = 1e9f) {
    world.setButton(Button::Right, true);
    const std::string scoreKey = kotha::canonicalName("score");
    f32 lastX = 0, stuck = 0;
    int respawns = 0;
    seconds = 0;
    while (seconds < maxSeconds && world.state() == GameState::Playing && world.player()->pos.x < untilX) {
        const Object* p = world.player();
        const Object* guard = world.find("প্রহরী");
        const f32 x = p->pos.x, feet = p->pos.y - 0.42f;
        const bool onFloor = p->mover.onGround && feet < 1.2f;
        bool jump = false;
        if (collectOnPlatforms && onFloor && world.variable(scoreKey) < 3 && x > 6.4f && x < 7.0f) jump = true;  // up to the first platform
        if (p->mover.onGround && feet > 2.8f && feet < 3.2f && x > 11.55f) jump = true;                          // across to the second
        if (onFloor && x > 21.95f && x < 22.6f) jump = true;                                                     // over the spikes
        if (onFloor && guard != nullptr && guard->pos.x - x > 0.4f && guard->pos.x - x < 1.7f) jump = true;      // over the guard
        if (onFloor && x > 37.2f) jump = true;                                                                   // the last coins hang in the air
        stuck = std::fabs(x - lastX) < 0.01f ? stuck + World::kStep : 0.0f;                                      // something in the way: hop over it
        if (p->mover.onGround && stuck > 0.25f && x < 35.0f) jump = true;
        if (x < lastX - 5.0f) ++respawns;
        lastX = x;
        world.setButton(Button::Jump, jump);
        world.step();
        seconds += World::kStep;
    }
    return respawns;
}

}  // namespace

GE_TEST(game_demo_can_be_won) {
    // Three coins on the platforms, past the spikes and the guard, the door opens (its own
    // script decides that), two more coins behind it.
    World world;
    loadDemo(world);
    const std::string scoreKey = kotha::canonicalName("score");
    f32 seconds = 0;
    const int respawns = playDemo(world, true, 120.0f, seconds);
    CHECK_MSG(world.state() == GameState::Won, "the robot finished with score " + std::to_string(world.variable(scoreKey)) + " at x = " +
                                                   std::to_string(world.player()->pos.x) + " after " + std::to_string(seconds) + " s");
    CHECK_NEAR(world.variable(scoreKey), 5.0, 1e-9);
    CHECK(world.find("দরজা") == nullptr);
    CHECK(world.find("কয়েন") == nullptr);
    CHECK_MSG(respawns <= 3, "the robot was sent back to the start " + std::to_string(respawns) + " times");
    std::printf("    demo won in %.1f s of game time, sent back to the start %d time(s)\n", static_cast<double>(seconds), respawns);
}

GE_TEST(game_demo_door_stays_shut_without_coins) {
    World world;
    loadDemo(world);
    f32 seconds = 0;
    playDemo(world, false, 30.0f, seconds, 35.0f);  // straight along the floor: no coins
    CHECK_MSG(world.player()->pos.x >= 35.0f, "the robot reached the door, x = " + std::to_string(world.player()->pos.x));
    run(world, 0.5f);
    const Object* door = world.find("দরজা");
    CHECK(door != nullptr);
    CHECK(door != nullptr && door->sayText == "৩টা কয়েন আনো" && door->sayTime > 0);  // the door says why it stays shut
    run(world, 4.0f);
    CHECK(world.find("দরজা") != nullptr);
    CHECK(world.state() == GameState::Playing);
    CHECK_NEAR(world.variable(kotha::canonicalName("score")), 0.0, 1e-9);
    CHECK_MSG(world.player()->pos.x > 35.0f && world.player()->pos.x < 36.0f, "the player waits in front of the door, x = " + std::to_string(world.player()->pos.x));
}

GE_TEST(game_many_lights_cast_shadows_at_once) {
    // 40 lamps around the player: more than 32 of them reach the screen. The renderer gives
    // shadows to the 32 nearest and still draws the others (without shadows).
    std::string level = std::string(42, '#') + "\n";
    for (int row = 0; row < 8; ++row) {
        std::string line = "#" + std::string(40, '.') + "#";
        if (row % 2 == 1) {
            for (int i = 0; i < 10; ++i) line[static_cast<std::size_t>(2 + i * 4)] = 'L';
        }
        if (row == 7) line[20] = 'P';
        level += line + "\n";
    }
    level += std::string(42, '#') + "\n";

    App app;
    CHECK(app.init(960, 540));
    loadDemo(app.world());
    app.world().load(level);
    CHECK(app.world().level() == level);
    for (int i = 0; i < 30; ++i) app.frame(1.0f / 60.0f);
    const RenderStats& stats = app.renderer().stats();
    CHECK_MSG(stats.lights > Renderer::kShadowRows, std::to_string(stats.lights) + " lights reach the screen");
    CHECK_MSG(stats.shadowLights == Renderer::kShadowRows, std::to_string(stats.shadowLights) + " lights cast shadows");
    CHECK_MSG(stats.drawCalls <= 16, "a frame takes " + std::to_string(stats.drawCalls) + " draw calls");
    CHECK(app.world().script(0).program.ok());  // the scripts survive a new level
    CHECK(app.world().player() != nullptr && app.world().player()->mover.onGround);
}

GE_TEST(game_odd_level_pictures_are_survived) {
    // Level pictures will come from the editor: nothing in them may crash the engine.
    App app;
    CHECK(app.init(640, 360));
    loadDemo(app.world());
    World& world = app.world();
    const auto play = [&app]() {
        for (int i = 0; i < 20; ++i) app.frame(1.0f / 60.0f);
        return app.hud().find("\"state\":\"playing\"") != std::string::npos;
    };

    world.load("");  // nothing at all
    CHECK(play());
    CHECK(world.player() == nullptr);
    CHECK(app.hud().find("\"player\"") == std::string::npos);

    world.load("\n\n...\n?x~\t\n");  // no walls, no player, unknown characters
    CHECK(play());

    world.load("P\n");  // a player with nothing to stand on falls and is put back
    CHECK(play());
    CHECK(world.player() != nullptr);

    world.load("#.P.P.#\n#######\n");  // two players: the first one counts
    CHECK(play());
    int players = 0;
    for (const Object& o : world.objects()) players += o.alive && o.sprite == Sprite::Player ? 1 : 0;
    CHECK(players == 1);
    CHECK(world.player() != nullptr && world.player()->pos.x < 3.0f);

    // Far too large, and far too many objects: cut down instead of exhausting the memory.
    std::string huge;
    for (int row = 0; row < 300; ++row) huge += std::string(600, row % 3 == 0 ? 'c' : '.') + "\n";
    world.load(huge);
    CHECK(play());
    CHECK(world.width() == World::kMaxLevelWidth && world.height() == World::kMaxLevelHeight);
    CHECK_MSG(static_cast<int>(world.objects().size()) <= World::kMaxObjects, std::to_string(world.objects().size()) + " objects");

    world.load("");
    loadDemo(world);  // ... and a good level still works afterwards
    CHECK(play());
    CHECK(world.scriptCount() == 6);  // loading the same scripts again replaces them
    CHECK(world.player() != nullptr && world.find("প্রহরী") != nullptr);
}

GE_TEST(atlas_is_generated_with_normals) {
    AtlasData atlas;
    generateAtlas(atlas);
    CHECK_EQ(atlas.width, 512);
    CHECK_EQ(atlas.albedo.size(), static_cast<std::size_t>(512 * 512 * 4));
    // The brick tile is opaque and its normal map is not flat.
    const SpriteRegion r = spriteRegion(Sprite::BrickA);
    const int x0 = static_cast<int>(r.u0 * 512), y0 = static_cast<int>(r.v0 * 512);
    int bumps = 0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const std::size_t o = (static_cast<std::size_t>(y0 + y) * 512 + static_cast<std::size_t>(x0 + x)) * 4;
            CHECK(atlas.albedo[o + 3] == 255);
            if (std::abs(atlas.normal[o] - 128) > 12 || std::abs(atlas.normal[o + 1] - 128) > 12) ++bumps;
        }
    }
    CHECK(bumps > 300);
    // The player sprite has transparent corners and glowing eyes.
    const SpriteRegion p = spriteRegion(Sprite::Player);
    const int px = static_cast<int>(p.u0 * 512), py = static_cast<int>(p.v0 * 512);
    CHECK_EQ(static_cast<int>(atlas.albedo[(static_cast<std::size_t>(py + 1) * 512 + static_cast<std::size_t>(px + 1)) * 4 + 3]), 0);
    int glowing = 0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) glowing += atlas.normal[(static_cast<std::size_t>(py + y) * 512 + static_cast<std::size_t>(px + x)) * 4 + 2] > 200 ? 1 : 0;
    }
    CHECK(glowing > 8);
}
