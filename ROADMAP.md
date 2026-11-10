# G-Engine — রোডম্যাপ (সংস্করণ ২ · গবেষণাভিত্তিক · ১০ নভেম্বর ২০২৬)

> এটা আগের রোডম্যাপের পুনর্গঠন: আসল ইঞ্জিন-স্থাপত্যের গবেষণার উপর দাঁড়ানো,
> বড় ব্যাচে কাজ, কম ইনপুটে চলে — আর কাজের মূল জায়গা এজেন্ট সংস্করণ (`ge-game`)।

## ০. উদ্দেশ্য

মোবাইলের জন্য ২ডি গেম ইঞ্জিন — গেমের আচরণ লেখা হয় সহজ বাংলা বা ইংরেজি বাক্যে (কথা)। গেম বানাবেন
সাধারণ মানুষ (ফোনের স্টুডিওতে) এবং বানাবে এআই এজেন্টরা (`ge-game` ও MCP সার্ভারে)।

## ১. গবেষণা — আসল ইঞ্জিনগুলো থেকে যা শিখলাম

| উৎস | শিক্ষা | আমাদের সিদ্ধান্ত |
|---|---|---|
| Jason Gregory, *Game Engine Architecture* (শিল্পের মূল বই) | ইঞ্জিন = স্তর: প্ল্যাটফর্ম → মূল সেবা (মেমরি, লগ, সময়) → উপব্যবস্থা (রেন্ডার, পদার্থবিদ্যা, শব্দ, ইনপুট, রিসোর্স) → গেমপ্লে। পদার্থবিদ্যা **নির্দিষ্ট ধাপে** (fixed step) চলবে, রেন্ডার আলাদা গতিতে | আমাদের `World::step()` আগে থেকেই fixed 1/60 — সেটাই মূল সত্য থাকবে; রেন্ডারার সবসময় ওই অবস্থা থেকে ছবি আঁকবে |
| Godot (nodes, scenes, signals) | খেলার জিনিস = নোডের গাছ; ছোট সিন বারবার ব্যবহার করা যায় (instancing); ফাইল ফরম্যাট টেক্সট-ভিত্তিক (.tscn), সংস্করণ-সহ; নোডের জীবনচক্র (_ready, _process, _physics_process) | আমাদের "এক জিনিস = এক স্ক্রিপ্ট" মডেলের সাথে **সিন-ফাইল** যোগ হবে: বস্তু + তার স্ক্রিপ্ট + সেটিংস এক জায়গায়, সংস্করণ নম্বরসহ (game.json v2) |
| ECS চর্চা (components = শুধু ডেটা, logic = systems) | বহু একই ধরনের জিনিসের জন্য ECS; অনন্য জিনিসের জন্য সিন-অবজেক্ট; পদার্থবিদ্যা লাইব্রেরি থেকে ফল লেখা হয় ট্রান্সফর্মে | বর্তমান অবজেক্ট-মডেল থাকবে (২ডি লেভেলের জন্য যথেষ্ট), কিন্তু স্ক্রিপ্ট-VM আলাদা সিস্টেম হিসেবে — বস্তুর ভেতরে লুকানো নয় |
| Milestone চর্চা (GameDeveloper.com, r/gamedev) | ধাপ = SMART শর্তের সমষ্টি; প্রতিটা ধাপের "সম্পন্ন যখন" শর্ত তৃতীয় ব্যক্তি যাচাই করতে পারবে; কঠিন ধাপ আগে রাখা; প্রতিটা ধাপে কিছু একটা চালু-যোগ্য বেরোবে | নিচের প্রতিটা ধাপের নিজস্ব "সম্পন্ন যখন" আছে, আর প্রতিটা ধাপ শেষে APK/লিংক/টুল হাতে পাওয়া যাবে |
| এজেন্ট প্লেটেস্টিং ২০২৬ (godot-mcp, Godot Playtester MCP, Unreal MCP, NodeTool) | এজেন্ট ভালো কাজ করে যখন: ঘড়ি থামিয়ে নির্দিষ্ট ধাপ চালানো যায়; অবস্থা আসে JSON-এ (স্ক্রিনশটের দরকার হয় না); লেখা → যাচাই → চালানো → ফল দেখা → ঠিক করা — বন্ধ লুপ; সব সম্পাদনা "checked operations" | আমাদের `ge-game` (check/simulate/pack) ঠিক এই লুপই; এখন তাতে যোগ হবে: শর্ত-পর্যন্ত চালানো (`--until`), সময়রেখা (watch), প্লেটেস্ট-সিনারিও — নিচের ধাপ ২ |

## ২. কাজের নিয়ম — কাজ যেভাবে চলবে

1. **বড় ব্যাচ, ক্রমবর্ধমান।** প্রতিটা ধাপে ৩–৫টা জোড়ালো ফিচার একসাথে (সাথে টেস্ট + ডক + রিলিজ)। ছোট খুঁটিনাটিতে
   ভাঙা হবে না; প্রতিটা কাজ-সেশন আগের সেশনের চেয়ে বড় হবে এবং এক সেশনে একটা সম্পূর্ণ ধাপ শেষ করাই লক্ষ্য।
2. **এজেন্ট সংস্করণই কাজের মাঠ।** প্রতিটা ফিচার আগে `ge-game` দিয়ে (পর্দা ছাড়া) বানানো ও পরীক্ষা হবে; টেস্ট সবুজ হলে
   তবেই স্টুডিও/অ্যাপে যুক্ত হবে। অর্থাৎ ইঞ্জিনের উন্নতি মানেই এজেন্ট টুলের উন্নতি।
3. **টেস্টই সত্য।** `sh tools/test.sh` (৫১+ টেস্ট, ৬৮৩০+ চেক), CLI স্মোক, ব্রাউজার পরীক্ষা — সব সবুজ ছাড়া কিছুই "শেষ" নয়।
4. **কম ইনপুট।** প্রতিটা ধাপের "সম্পন্ন যখন" শর্ত যাচাইযোগ্য করে লেখা — আপনাকে বলতে হবে না "এখন যাও"। আমি এক ধাপ শেষ
   করে নিজে থেকেই পরেরটা শুরু করব; সত্যিকারের সিদ্ধান্ত লাগলে (যেমন আপনার ফোনের FPS) তবেই জিজ্ঞেস করব।
5. **প্রতিটা ধাপে হাতে কিছু থাকবে** — নতুন APK, চালু লিংক, বা নতুন টুল; টেস্ট-ফল সহ।

## ৩. ধাপসমূহ

### ধাপ ১ — ফোনে সচল ও নির্ভরযোগ্য · এজেন্ট-টুল v2 (এই সেশনে শুরু)
- 🟡 সাদা পর্দার সমাধান: অ্যাপ নিজেই নিজের ফাইল পরিবেশন করে (`shouldInterceptRequest`) — সার্ভার/পোর্ট/নেটওয়ার্ক নেই; ব্যর্থ হলে দৃশ্যমান বার্তা, সাদা পর্দা নয়
- 🟡 নির্দিষ্ট পুনরাবৃত্তি (deterministic replay): একই ইনপুট → একই ফল — `ge-game`-এ যাচাই
- ⬜ `simulate --until "স্কোর >= ৫"` — শর্ত পূরণ না হওয়া পর্যন্ত চালানো (গবেষণা: godot-mcp-এর step-until)
- ⬜ সময়রেখা: প্রতি সেকেন্ডে অবস্থার JSON (অবস্থা, স্কোর, অবস্থান) — এজেন্ট যেন দেখতে পারে কী হলো
- ⬜ প্লেটেস্ট-সিনারিও: গেম ফোল্ডারে `playtest.txt` (ইনপুট + প্রত্যাশা) — CI নিজেই খেলে যাচাই করবে
- **সম্পন্ন যখন:** অ্যাপে স্টুডিও দেখায় (আপনার ফোনের স্ক্রিনশটসহ যাচাই); নমুনা গেমের playtest CI-তে নিজে চলে

### ধাপ ২ — সিন ও রিসোর্স ব্যবস্থা (Godot-অনুপ্রাণিত)
- ⬜ game.json সংস্করণ ২: দৃশ্য = বস্তু + স্ক্রিপ্ট + সেটিংস + সংস্করণ নম্বর; পুরনো ফাইল লোড হবেই (backward compatible)
- ⬜ প্রতি বস্তুর নিজস্ব ভ্যারিয়েবল ও নিজস্ব স্ক্রিপ্ট-কপি (একই "প্রহরী" দুটো, দুটোর আলাদা আচরণ)
- ⬜ অবস্থা সংরক্ষণ (checkpoint / save / load) — খেলা মাঝপথে থামলে যেখানে ছিল সেখান থেকে
- **সম্পন্ন যখন:** একটা গেম ফাইল স্টুডিও ↔ `ge-game` ↔ ফোন — তিন জায়গায় যায়-আসে, কিছুই হারায় না

### ধাপ ৩ — এডিটর পূর্ণতা (ফোন = ওয়েব)
- ⬜ undo/redo, আঙুলে বস্তু সরানো, আলোর রং-মাপ, Play/Stop, ধাপে ধাপে চালানো
- ⬜ ভুল লেখার সময় শব্দ-প্রস্তাব; প্রতি বস্তুতে ট্যাপ করলেই তার স্ক্রিপ্ট
- **সম্পন্ন যখন:** নতুন একজন মানুষ ফোনে ১৫ মিনিটে একটা ছোট লেভেল বানিয়ে খেলতে পারে

### ধাপ ৪ — কনটেন্ট ও গতি (এক ব্যাচে)
- ⬜ শব্দ ও সঙ্গীত (`"লাফ" শব্দ বাজাও`), ছবি আমদানি (PNG → স্প্রাইট), কণা (আগুন, ধুলো)
- ⬜ একাধিক লেভেল/দৃশ্য, ক্যামেরা-নিয়ন্ত্রণ
- ⬜ ফোনে ৩০+ FPS বাজেট: প্রোফাইলার, স্বয়ংক্রিত মান-স্তর, আলোর বাফার অর্ধেক
- **সম্পন্ন যখন:** শব্দসহ ২-লেভেলের গেম; আপনার ফোনে ৩০+ FPS (আপনার রিপোর্ট লাগবে — এটাই একমাত্র কাজ যেখানে আপনার ইনপুট লাগবে)

### ধাপ ৫ — এজেন্ট সংস্করণ v2 ও প্রকাশ
- ⬜ MCP টুলে সিনারিও-প্লেটেস্ট, শর্ত-সিমুলেশন, অবস্থা-দর্শন (watch)
- ⬜ এক কথায় বলা গেম এজেন্ট নিজে বানিয়ে, খেলে, ঠিক করে দেখাবে — এটাই গ্রহণযোগ্যতার পরীক্ষা
- ⬜ স্টুডিও থেকে এক চাপে শেয়ারযোগ্য লিংক; নমুনা গেম ৩টা; বাংলা ভিডিও/টিউটোরিয়াল
- **সম্পন্ন যখন:** আপনি এক লাইনে গেমের কথা বলেন → এজেন্ট বানিয়ে দেয় → ফোনে খেলা যায়

## ৪. চলমান অঙ্গীকার

- প্রতিটা কাজ-সেশনে একটা ধাপ (বা তার বড় অংশ) শেষ — মাঝপথে থামা নেই, ছোট ডেলিভারিও নেই।
- প্রতিটা সেশনের শেষে হাতে যাচাইযোগ্য কিছু: APK, লিংক, টুল বা টেস্ট-ফল।
- ধরে নেওয়া নয় — গবেষণা করে, প্রয়োজনে ছোট প্রোটোটাইপ করে সিদ্ধান্ত।
- ইঞ্জিনের প্রতিটা উন্নতি এজেন্ট টুলেও আসবে (নিয়ম ২)।

---

# Roadmap (English · version 2 · research-based)

## Purpose
A 2D game engine for phones where game behaviour is written in plain Bengali or English
sentences (Kotha). People build games in the phone studio; AI agents build games through
`ge-game` and the MCP server.

## What the research says

| Source | Lesson | Our decision |
|---|---|---|
| Jason Gregory, *Game Engine Architecture* | Layers: platform → core services → subsystems (render, physics, audio, input, resources) → gameplay. Physics on a **fixed step**, rendering at its own rate | `World::step()` at fixed 1/60 stays the single source of truth; the renderer only draws that state |
| Godot (nodes, scenes, signals) | Game things are node trees; small scenes are reused (instancing); file formats are text and versioned; objects have a lifecycle | Add a **scene file**: objects + scripts + settings together, versioned (game.json v2) |
| ECS practice | Components are data only, logic lives in systems; physics writes back into transforms | Keep the object model (enough for 2D levels) but keep the script VM a separate system, not hidden inside objects |
| Milestone practice (GameDeveloper, r/gamedev) | Milestones = SMART goals with a third-person-verifiable "done when"; hard things early; every milestone ships something runnable | Every phase below has its own "done when" and leaves an APK/link/tool in your hands |
| Agent playtesting 2026 (godot-mcp, Playtester MCP, Unreal MCP, NodeTool) | Agents work well with: frozen clock + exact steps; state as JSON (no screenshots needed); a closed loop of edit → check → run → read → fix; "checked operations" | `ge-game` (check/simulate/pack) is exactly that loop; next: `--until` conditions, a state timeline, playtest scenarios |

## How work happens

1. **Big batches, growing.** Each phase = 3–5 connected features at once (with tests + docs + release). No tiny steps; each work session aims to finish a whole phase and is bigger than the last.
2. **The agent edition is the workbench.** Every feature is built and tested headless with `ge-game` first; only green tests reach the studio/app. Engine progress = agent-tool progress.
3. **Tests are the truth.** `sh tools/test.sh` (51+ tests, 6830+ checks), CLI smoke, browser checks — nothing is "done" until all green.
4. **Little input needed.** "Done when" conditions are verifiable, so you don't have to say "go now". I finish a phase and start the next by myself; I ask only for real decisions (like your phone's FPS).
5. **Every phase ships** a fresh APK, a live link, or a new tool — with test results.

## Phases

### Phase 1 — Phone works reliably · agent tools v2 (started this session)
🟡 white-screen fix (app serves its own files, visible errors, never blank) · 🟡 deterministic
replay check · ⬜ `simulate --until` · ⬜ per-second state timeline · ⬜ `playtest.txt` scenarios
run by CI. **Done when:** the studio shows on your phone (verified by your screenshot); a sample
game's playtest runs itself in CI.

### Phase 2 — Scene & resource system (Godot-inspired)
⬜ game.json v2 (versioned, backward compatible) · ⬜ per-object variables and script copies ·
⬜ save/load checkpoints. **Done when:** one game file travels studio ↔ `ge-game` ↔ phone intact.

### Phase 3 — Editor complete (phone = web)
⬜ undo/redo, moving objects by finger, light colour/size, play/stop, single-stepping · ⬜ word
suggestions while typing. **Done when:** a newcomer builds and plays a small level on a phone in
15 minutes.

### Phase 4 — Content & speed (one batch)
⬜ sound & music, PNG sprite import, particles · ⬜ several levels, camera control · ⬜ 30+ FPS
budget on phones (profiler, auto quality). **Done when:** a 2-level game with sound; 30+ FPS on
your phone (your report — the one input I need).

### Phase 5 — Agent edition v2 & publishing
⬜ scenario playtests, until-conditions and watch tools in MCP · ⬜ "a one-line game request →
the agent builds, plays and fixes it" as the acceptance test · ⬜ one-tap share links, 3 sample
games, Bengali video tutorials. **Done when:** you describe a game in one line; the agent ships
it; it plays on your phone.

## Standing commitments

- Each work session finishes a phase (or a large part of one) — no mid-phase stalls, no tiny deliveries.
- Every session ends with something verifiable in your hands.
- No assumptions — research first, small prototypes when unsure.
- Every engine improvement also lands in the agent tools.
