# G-Engine — রোডম্যাপ / Roadmap

> **বাংলা প্রথমে, English below.** দুটো অংশে একই কথা লেখা আছে।
> অবস্থা-চিহ্ন / status: ✅ হয়েছে ও পরীক্ষিত (done & verified) · 🟡 আংশিক (partial) · ⬜ বাকি (to do)

---

# বাংলা

## ১. এক নজরে

**কী বানাচ্ছি:** মোবাইলের জন্য একটা 2D গেম ইঞ্জিন, যার ভেতরটা C++ এ লেখা, আর যেখানে গেমের নিয়ম লেখা যায়
সহজ বাংলা বা ইংরেজি বাক্যে — যেমন `ডান বোতাম চাপলে ডানে হাঁটো` বা `walk right`। সাথে থাকবে উন্নত মানের আলো-ছায়া
(normal map, নরম ছায়া, HDR, bloom) আর বাস্তবসম্মত ফিজিক্স।

**এটা কি সম্ভব?** হ্যাঁ। ধাপ ০ আর ধাপ ১-এর প্রায় সবটা এই রিপোজিটরিতেই বানানো ও পরীক্ষা করা আছে: সবচেয়ে ঝুঁকির
তিনটা অংশ (বাংলা বাক্য বোঝা, আলো-ছায়ার রেন্ডারার, ফিজিক্স) ব্রাউজারে একসাথে কাজ করে — এটা চালিয়ে দেখা, অনুমান নয়।
**যা এখনো প্রমাণিত নয়:** আসল ফোনে গতি। এখন পর্যন্ত কোনো আসল ফোনে এটা চালানো হয়নি — ধাপ ১-এর শেষ ঘরটা সেজন্যই খালি।

**যা সৎভাবে বলা দরকার:**

- "বাংলা বোঝা" মানে AI নয়। এটা একটা **নিয়ন্ত্রিত স্বাভাবিক ভাষা**: ইঞ্জিন একটা শব্দভাণ্ডার (এখন ২৬০০+ শব্দরূপ) আর
  কিছু বাক্য-গঠন চেনে। সুবিধা: ইন্টারনেট লাগে না, ফোনে মুহূর্তে চলে, একই বাক্যের ফল সবসময় একই। সীমা: শব্দভাণ্ডারের
  বাইরের বাক্য বুঝবে না — তখন সে চুপ করে ভুল কিছু করে না, বাংলায় বলে দেয় কোন শব্দটা বোঝেনি এবং কাছাকাছি শব্দ প্রস্তাব করে।
- Unity/Godot এর মতো পূর্ণাঙ্গ ইঞ্জিন একজনের পক্ষে এক ধাপে হয় না। তাই পরিসর ইচ্ছা করে ছোট রাখা: **2D, মোবাইল-প্রথম**।
- Android APK এই স্যান্ডবক্সে বানানো বা চালিয়ে দেখা যায় না (Android SDK/NDK নামানো যায় না)। সেটা ধাপ ২ এ GitHub Actions
  দিয়ে হবে এবং প্রথম APK আপনার ফোনে আপনাকেই পরীক্ষা করতে হবে।
- ব্রাউজারের পরীক্ষা চলে পর্দাহীন (headless) Chromium-এ — সফটওয়্যার রেন্ডারিং আর ফোনের অনুকরণ দিয়ে। তাতে প্রমাণ হয় ছবি ঠিক
  আসে আর টাচ-বোতাম কাজ করে; ফোনে কত দ্রুত চলবে তা প্রমাণ হয় না।

## ২. মূল সিদ্ধান্ত (এবং কেন)

| বিষয় | সিদ্ধান্ত | কারণ |
|---|---|---|
| ভাষা | C++20, exception ও RTTI ছাড়া | ছোট বাইনারি, মোবাইলে দ্রুত; Android NDK ও WebAssembly দুটোতেই চলে |
| গ্রাফিক্স | OpenGL ES 3.0 = WebGL2 (একই শেডার) | প্রায় সব Android ফোনে আছে; একই রেন্ডারার ব্রাউজারেও চলে। Vulkan পরে, দরকার হলে |
| ফিজিক্স | Box2D v3.2 (সোর্সসহ `third_party/` এ) | পরীক্ষিত, দ্রুত, MIT লাইসেন্স; নিজে ফিজিক্স লেখা সময়ের অপচয় |
| স্ক্রিপ্ট | নিজস্ব ভাষা **কথা (Kotha)** → বাইটকোড → ছোট VM | বাংলা/ইংরেজি বাক্য; প্রতিটা লাইন কীভাবে বোঝা হলো তা দেখায় |
| এডিটর | ওয়েব অ্যাপ (পরে PWA), ইঞ্জিন চলে WebAssembly তে | ফোনের ব্রাউজারেই চলে, ইনস্টল লাগে না; বাংলা কিবোর্ড ও লেখা ব্রাউজার সামলায় |
| গেম চালানো | একই C++ কোড: ব্রাউজারে wasm, ফোনে native APK | "এডিটরে যা দেখি, ফোনে তাই পাই" |
| বিল্ড | CMake; wasm এর জন্য zig; Android এর জন্য NDK (CI তে) | সব টুল `pip install` দিয়ে পাওয়া যায়, Emscripten লাগে না |
| পরীক্ষা | নিজস্ব ছোট টেস্ট ফ্রেমওয়ার্ক, GPU ছাড়াই পুরো ইঞ্জিন চলে | প্রতিটা পরিবর্তনের পর কয়েক সেকেন্ডে যাচাই |

## ৩. গঠন

```
 আপনার লেখা  ──►  কথা কম্পাইলার  ──►  নিয়ম + বাইটকোড  ──►  VM
 "ডানে হাঁটো"      (শব্দ → ধারণা)                             │  হাঁটো / লাফ দাও / আলো জ্বালাও ...
                                                               ▼
        ইনপুট ──►  গেম-জগৎ (বস্তু, চরিত্র, আলো)  ◄──►  ফিজিক্স (Box2D)
        (টাচ/কিবোর্ড)             │
                                  ▼
                 রেন্ডারার: G-buffer → ছায়া → আলো → HDR → bloom → পর্দা
                                  │
              ┌───────────────────┴───────────────────┐
        ওয়েব (WebGL2, wasm)                    Android (GLES3, native)
```

| ফোল্ডার | কী আছে |
|---|---|
| `engine/core` | গণিত, লগ, UTF-8, র‍্যান্ডম |
| `engine/script` | কথা ভাষা: শব্দভাণ্ডার, কম্পাইলার, VM |
| `engine/physics` | Box2D এর উপর পাতলা স্তর |
| `engine/gfx` | রেন্ডারার, শেডার, গ্রাফিক্স-স্তর (`gl.h`) |
| `engine/scene`, `engine/app` | গেম-জগৎ, বস্তু, চরিত্র-নিয়ন্ত্রক, গেম-লুপ |
| `engine/platform` | এখন: ওয়েব। ধাপ ২ এ: Android |
| `games/demo`, `web/` | নমুনা খেলা ও ব্রাউজারের খোলস (এডিটরের শুরু) |
| `tests/`, `tools/`, `docs/` | পরীক্ষা, সহায়ক স্ক্রিপ্ট, নথি |

## ৪. ধাপসমূহ

প্রতিটা ধাপের শেষে একটা **চালিয়ে দেখার মতো জিনিস** থাকে। আকার: S = ছোট, M = মাঝারি, L = বড় (এক ধাপ ≈ এক থেকে কয়েকটা কাজের পালা)।

### ধাপ ০ — ভিত ✅
- ✅ রিপোজিটরির কাঠামো, CMake বিল্ড (PC + WebAssembly), টেস্ট ফ্রেমওয়ার্ক
- ✅ Box2D v3.2 যুক্ত; PC ও WebAssembly দুই জায়গায় একই ফল দেয় তা যাচাই করা
- ✅ কোন টুল দিয়ে কী বানানো যায়/যায় না তা হাতে-কলমে যাচাই (ফল: উপরের সিদ্ধান্তগুলো)

### ধাপ ১ — প্রথম চলন্ত নমুনা (vertical slice) 🟡
লক্ষ্য: ব্রাউজারে (ফোনেও) একটা ছোট খেলা, যেখানে বাংলায় লিখলেই চরিত্র তা করে, আলো-ছায়া ও ফিজিক্সসহ।

- ✅ **কথা ভাষা v0.1** — কম্পাইলার + VM; ২৬০০+ শব্দরূপ; ক্রিয়ার রূপ (`হাঁটো/হাঁটবে/হেঁটে/হাঁটলে`) ও বিভক্তি (`খেলোয়াড়ের/কয়েনগুলো`) নিজে চেনে;
  বাংলা-ইংরেজি দুই ভাষায় ভুলের বার্তা ও শব্দ-প্রস্তাব; প্রতিটা লাইন কীভাবে বোঝা হলো তার টীকা; ব্লক (`বারবার:`, `৩ বার:`, `নাহলে:`),
  বার্তা আদান-প্রদান; খেলায় নেই এমন জিনিসের নাম লিখলে ধরে ফেলে। [নির্দেশিকা](docs/KOTHA.md) — তার প্রতিটা উদাহরণ পরীক্ষায় কম্পাইল হয়
- ✅ **ফিজিক্স** — Box2D জগৎ; চরিত্র-নিয়ন্ত্রক: হাঁটা ও থামা, মাপা উচ্চতার লাফ, কিনারা পেরোনোর ঠিক পরেও লাফ, মাটিতে নামার
  ঠিক আগে চাপা লাফ মনে রাখা; ছোঁয়ার ঘটনা
- ✅ **রেন্ডারার** — normal map সহ স্প্রাইট, একসাথে ৩২টা ছায়া-দেওয়া আলো, নরম ছায়া, HDR, bloom, ACES টোন-ম্যাপিং; পুরো দৃশ্য
  ১৪টা draw call ([বিবরণ](docs/RENDERING.md))। যাচাই headless Chromium-এ: WebGL ভুল নেই, ছবি চোখে দেখা। ৩২টা আলোর ছায়া
  শুদ্ধতার জন্য যাচাই করা, গতির জন্য নয়
- ✅ **ওয়েব প্লেয়ার** — টাচ-বোতাম; পাশে স্ক্রিপ্ট লেখার ঘর — লিখে "চালাও" চাপলেই চরিত্র মানে; ভুল স্ক্রিপ্ট ফিরিয়ে দেয় আর আগেরটা
  চলতে থাকে; বাংলা ও ইংরেজি ইন্টারফেস। যাচাই headless Chromium-এ, কম্পিউটারের মাপে ও ফোনের অনুকরণে (৩৯০×৭৮০, টাচ)
- ✅ **নমুনা খেলা** — খেলোয়াড়, পাহারাদার, কয়েন, কাঁটা, দরজা, বাক্স, বাতি; সবার আচরণ কথা-স্ক্রিপ্টে লেখা। পরীক্ষায় একটা রোবট
  খেলাটা শুরু থেকে শেষ পর্যন্ত খেলে জেতে
- ✅ **স্বয়ংক্রিয় পরীক্ষা** — ইঞ্জিন: ৫১টা পরীক্ষা, ৬৮৩০টা যাচাই (মেমরি-ভুল ও undefined behaviour ধরার ব্যবস্থাসহ চালালেও পরিষ্কার);
  ব্রাউজার: ২৪টা যাচাই। চালানো: `sh tools/test.sh`, `--asan`, `--web`
- ⬜ **আসল ফোনে চালিয়ে দেখা** — এটা আপনাকে করতে হবে: পাতাটা ফোনের ব্রাউজারে খুলে জানান ঠিকমতো দেখায় কি না আর কতটা মসৃণ চলে
  (কীভাবে খুলবেন: [README](README.md))। গতি কম হলে মান-স্তর ঠিক করা হবে এই ধাপেই
- **সম্পন্ন যখন:** ব্রাউজারে `ডান বোতাম চাপলে ডানে হাঁটো` লিখে চালালে চরিত্র ডানে হাঁটে, আর আলো-ছায়া পর্দায় দেখা যায় — কম্পিউটারে
  (✅ হয়েছে) এবং একটা আসল ফোনে (⬜ বাকি)

### ধাপ ২ — Android APK (M) ⬜
লক্ষ্য: একই খেলা ফোনে native অ্যাপ হিসেবে চলবে।
- ⬜ `engine/platform/android`: GameActivity + EGL + GLES 3.0, টাচ ইনপুট, pause/resume, পর্দা ঘোরানো
- ⬜ `android/` Gradle প্রজেক্ট (arm64-v8a + armeabi-v7a), অ্যাসেট প্যাকেজিং
- ⬜ GitHub Actions: প্রতিটা push এ টেস্ট চালানো + debug APK বানিয়ে ডাউনলোডের জন্য রাখা
- 🟡 মান-স্তর: ওয়েব প্লেয়ারে পাঁচটা স্তর আছে (ছবির ও আলোর রেজোলিউশন), ধীর চললে নিজে থেকে এক ধাপ নামে — লেখা আছে, কিন্তু
  আসল ফোনে কাজ করতে দেখা হয়নি। বাকি: Android অ্যাপে একই ব্যবস্থা, আর আসল ফোনে মেপে সীমাগুলো ঠিক করা
- **সম্পন্ন যখন:** আপনি GitHub থেকে APK নামিয়ে ফোনে ইনস্টল করে খেলাটা ৩০+ FPS এ চালাতে পারবেন
- **ঝুঁকি:** এখান থেকে যাচাই করা যায় না; প্রথম ২–৩ বার CI ব্যর্থ হওয়া স্বাভাবিক — আপনার ফোনের ফলাফল জানাতে হবে

### ধাপ ৩ — ফোনে এডিটর (L) ⬜
লক্ষ্য: কোড না ছুঁয়ে, শুধু ফোন দিয়ে একটা লেভেল বানানো।
- ⬜ দৃশ্য-এডিটর: আঙুল দিয়ে বস্তু বসানো/সরানো/মোছা, টাইল আঁকা, আলো বসানো ও রং-মাপ ঠিক করা
- 🟡 স্ক্রিপ্ট লেখা: এখন প্রতিটা *ধরনের* জিনিসের একটা স্ক্রিপ্ট, আর লেখার সময়ই টীকা ও ভুল দেখায়। বাকি: প্রতিটা বস্তুর আলাদা স্ক্রিপ্ট,
  লেখার সময় শব্দ-প্রস্তাব
- ⬜ প্রজেক্ট সংরক্ষণ/খোলা (ফোনের ভেতরে + ফাইল হিসেবে রপ্তানি); দৃশ্যের ফাইল-ফরম্যাট (JSON) — এখন পাতা আবার খুললে এডিটরে
  বদলানো স্ক্রিপ্ট হারিয়ে যায়
- ⬜ PWA: হোম স্ক্রিনে বসানো যায়, ইন্টারনেট ছাড়া চলে
- ⬜ Play / Stop, ধাপে ধাপে চালানো, কোন নিয়ম এখন চলছে তা দেখানো
- **সম্পন্ন যখন:** নতুন একজন মানুষ ফোনে ১৫ মিনিটে একটা ছোট লেভেল বানিয়ে খেলতে পারে

### ধাপ ৪ — কনটেন্ট (L) ⬜
- ⬜ ছবি আমদানি (PNG), স্প্রাইট-শিট ও অ্যানিমেশন; ছবি থেকে normal map তৈরির টুল
- ⬜ শব্দ ও সঙ্গীত (`"লাফ" শব্দ বাজাও`)
- ⬜ কণা (আগুন, ধুলো, বৃষ্টি), ক্যামেরা-নিয়ন্ত্রণ, একাধিক লেভেল
- ⬜ খেলার ভেতরে বাংলা লেখা দেখানো (যুক্তাক্ষরসহ — এর জন্য HarfBuzz বা প্ল্যাটফর্মের টেক্সট-রেন্ডারার লাগবে) ও UI (স্কোর, বোতাম, মেনু)
- ⬜ আরও ফিজিক্স: জোড়া (joint), দড়ি, চলন্ত প্ল্যাটফর্ম, একমুখী প্ল্যাটফর্ম

### ধাপ ৫ — কথা ভাষার বিকাশ (M, চলমান) ⬜
- ⬜ শব্দভাণ্ডার বাড়ানো — সত্যিকারের ব্যবহারকারীরা যা লিখে ব্যর্থ হন সেই বাক্যগুলো থেকে
- ⬜ নিজের নিয়মের নাম দেওয়া (`"আক্রমণ" মানে: ...`), হিসাব (`স্কোর = স্কোর + কয়েন × ২`), বস্তু তৈরি (`"গুলি" বানাও`)
- ⬜ আঞ্চলিক ও চলিত বানানের সহনশীলতা, Banglish (`dane hato`) — ঐচ্ছিক
- ⬜ ঐচ্ছিক: অনলাইন AI সহকারী, যে মুক্ত বাংলা বর্ণনা থেকে কথা-স্ক্রিপ্ট লিখে দেয় (ইঞ্জিন তার উপর নির্ভর করবে না)
- 🟡 ভাষা-নির্দেশিকা (বাংলা + ইংরেজি): v0.1-এর জন্য আছে ([docs/KOTHA.md](docs/KOTHA.md)); বাকি: উদাহরণ-সংগ্রহ

### ধাপ ৬ — আরও উন্নত আলো (M) ⬜
- ⬜ পরোক্ষ আলো (global illumination, radiance cascades পদ্ধতি) — আলো দেয়ালে লেগে ছড়াবে
- ⬜ আকৃতিওয়ালা আলো (টর্চ/স্পটলাইট), রঙিন কাচের ছায়া, আলোর রশ্মি (god rays), পানিতে প্রতিফলন
- ⬜ দিন-রাত, আবহাওয়া, রং-সমন্বয় (color grading)
- ⬜ দরকার হলে Vulkan ব্যাকএন্ড

### ধাপ ৭ — প্রকাশ (L) ⬜
- ⬜ এডিটর থেকে এক চাপে APK/AAB (ক্লাউড-বিল্ড, অথবা তৈরি-করা প্লেয়ার APK তে গেমের ডেটা বসিয়ে সই করা)
- ⬜ ওয়েবে প্রকাশ (একটা লিঙ্ক শেয়ার করলেই খেলা যায়)
- ⬜ সেভ-গেম, সেটিংস, অ্যাপ আইকন/নাম; বিজ্ঞাপন ও কেনাকাটার জন্য প্লাগইন-ব্যবস্থা
- ⬜ নমুনা খেলা ৩টা, ভিডিও/লিখিত টিউটোরিয়াল (বাংলা), ১.০ প্রকাশ

## ৫. ঝুঁকি ও প্রতিকার

| ঝুঁকি | প্রতিকার |
|---|---|
| স্বাভাবিক ভাষা দ্ব্যর্থক — ইঞ্জিন ভুল বুঝতে পারে | প্রতিটা লাইনের পাশে দেখায় "আমি এভাবে বুঝেছি"; না বুঝলে থেমে প্রশ্ন করে; প্রতিটা বাক্য-গঠনের জন্য স্বয়ংক্রিয় পরীক্ষা |
| দুর্বল ফোনে আলো-ছায়া ধীর (এখনো মাপা হয়নি) | আলোর বাফার অর্ধেক মাপে আঁকা যায়, ছায়া-দেওয়া আলোর সীমা, মান-স্তর; পুরো দৃশ্য ১৪টা draw call |
| APK এখানে যাচাই করা যায় না | CI + আপনার ফোন; প্ল্যাটফর্ম-স্তর পাতলা — ভাষা, খেলার নিয়ম ও ফিজিক্স PC-তে পরীক্ষিত, রেন্ডারার ব্রাউজারে |
| খেলার ভেতরে বাংলা যুক্তাক্ষর | ধাপ ৪ এ আলাদা কাজ হিসেবে ধরা আছে; ততদিন লেখা দেখায় ব্রাউজার/প্ল্যাটফর্ম |
| পরিসর বেড়ে যাওয়া | প্রতিটা ধাপের "সম্পন্ন যখন" শর্ত; 3D, মাল্টিপ্লেয়ার, ভিজ্যুয়াল নোড-এডিটর ইচ্ছা করে বাদ |
| ফোনে APK বানানো কঠিন | শুরুতে ক্লাউড-বিল্ড; পরে তৈরি-প্লেয়ার + ডেটা বদলের পদ্ধতি |

## ৬. আপনার কাছ থেকে যা জানা দরকার

কোনোটার উত্তর না দিলে পাশে লেখা ডিফল্ট ধরে এগোব।

1. **লাইসেন্স** — ডিফল্ট: এখন কোনো LICENSE ফাইল যোগ করিনি। সবার জন্য উন্মুক্ত করতে চাইলে MIT প্রস্তাব করি।
2. **স্ক্রিপ্ট ভাষার নাম** — ডিফল্ট: "কথা / Kotha" (ফাইল `.kotha`)। বদলানো সহজ।
3. **"মোবাইলের জন্য ইঞ্জিন"** — ধরে নিয়েছি দুটোই: (ক) মোবাইল গেম বানানোর ইঞ্জিন, (খ) যা ফোনেই ব্যবহার করা যায়। শুধু একটা চাইলে ধাপ ৩ এর আকার বদলাবে।
4. **পরের ধাপ** — ডিফল্ট: ধাপ ২ (Android APK)। আগে এডিটর চাইলে ধাপ ৩ আগে করা যায়।
5. **কোন ধরনের খেলা আগে** — ডিফল্ট: প্ল্যাটফর্মার (পাশ থেকে দেখা)। উপর থেকে দেখা (top-down) খেলা চাইলে জানাবেন।

## ৭. কাজের নিয়ম

- প্রতিটা ধাপ শেষে: কী হলো, কীভাবে চালিয়ে দেখবেন, আর কী যাচাই করা যায়নি — এই তিনটা স্পষ্ট লেখা থাকবে।
- যা পরীক্ষা করা যায়নি তা কখনো "হয়েছে" বলা হবে না।
- প্রতিটা নতুন বাক্য-গঠন বা ফিচারের সাথে স্বয়ংক্রিয় পরীক্ষা যোগ হবে।
- এই ফাইলটাই একমাত্র সত্য উৎস — প্রতিটা ধাপের পর হালনাগাদ হবে।

---

# English

## 1. At a glance

**What we are building:** a 2D game engine for mobile with a C++ core, where game rules are written in plain
Bengali or English sentences — `ডান বোতাম চাপলে ডানে হাঁটো` or `when the right button is pressed, walk right` —
together with high quality lighting (normal maps, soft shadows, HDR, bloom) and realistic physics.

**Is it feasible?** Yes. Phase 0 and almost all of phase 1 are built and tested in this repository: the three
riskiest parts (understanding Bengali sentences, the lighting renderer, physics) work together in a browser.
That has been run, not assumed. **What is not proven yet:** speed on a real phone. Nothing has been run on a
real phone so far, which is why the last box of phase 1 is still empty.

**What has to be said plainly:**

- "Understanding Bengali" is not AI. It is a **controlled natural language**: the engine knows a vocabulary
  (2600+ word forms today) and a set of sentence patterns. Upside: no internet, instant on a phone, the same
  sentence always does the same thing. Limit: sentences outside the vocabulary are not understood — in that case
  the engine does not silently do something wrong; it says which word it did not understand and suggests a close one.
- A complete engine like Unity or Godot is not a one-step job for one person. The scope is deliberately small:
  **2D, mobile first**.
- An Android APK cannot be built or run in this sandbox (the Android SDK/NDK cannot be downloaded here). That is
  phase 2, through GitHub Actions, and the first APK has to be tried by you on your phone.
- The browser test runs in headless Chromium, with software rendering and phone emulation. It proves that the
  picture is right and that the touch buttons work; it does not prove how fast a phone will run it.

## 2. Key decisions (and why)

| Topic | Decision | Reason |
|---|---|---|
| Language | C++20, no exceptions, no RTTI | small binary, fast on mobile; works with both the Android NDK and WebAssembly |
| Graphics | OpenGL ES 3.0 = WebGL2 (same shaders) | available on practically every Android phone; the same renderer runs in a browser. Vulkan later if needed |
| Physics | Box2D v3.2 (vendored in `third_party/`) | proven, fast, MIT licensed; writing our own would waste time |
| Scripting | our own language **Kotha (কথা)** → bytecode → small VM | Bengali/English sentences; shows how each line was understood |
| Editor | web app (later a PWA) hosting the engine as WebAssembly | runs in the phone's browser without installing; the browser handles Bengali keyboards and text |
| Running games | the same C++ code: wasm in the browser, native APK on the phone | "what I see in the editor is what I get on the phone" |
| Build | CMake; zig for wasm; the NDK for Android (in CI) | every tool installs with `pip install`; Emscripten is not required |
| Testing | small in-house test framework; the whole engine runs without a GPU | every change is verified in seconds |

## 3. Architecture

```
 what you write ──►  Kotha compiler  ──►  rules + bytecode  ──►  VM
 "walk right"        (words → concepts)                          │  walk / jump / turn on the light ...
                                                                  ▼
        input   ──►  game world (objects, characters, lights)  ◄──►  physics (Box2D)
   (touch/keyboard)                │
                                   ▼
                 renderer: G-buffer → shadows → lights → HDR → bloom → screen
                                   │
               ┌───────────────────┴───────────────────┐
         web (WebGL2, wasm)                      Android (GLES3, native)
```

| Folder | Contents |
|---|---|
| `engine/core` | math, logging, UTF-8, random numbers |
| `engine/script` | the Kotha language: vocabulary, compiler, VM |
| `engine/physics` | thin layer over Box2D |
| `engine/gfx` | renderer, shaders, graphics layer (`gl.h`) |
| `engine/scene`, `engine/app` | game world, objects, character controller, game loop |
| `engine/platform` | now: web. In phase 2: Android |
| `games/demo`, `web/` | sample game and the browser shell (the beginning of the editor) |
| `tests/`, `tools/`, `docs/` | tests, helper scripts, documentation |

## 4. Phases

Every phase ends with **something you can run**. Size: S = small, M = medium, L = large (a phase ≈ one to a few
working sessions).

### Phase 0 — Foundation ✅
- ✅ Repository layout, CMake build (PC + WebAssembly), test framework
- ✅ Box2D v3.2 integrated; verified to give the same result on PC and in WebAssembly
- ✅ Hands-on check of which tools can and cannot build what (result: the decisions above)

### Phase 1 — First playable slice 🟡
Goal: a small game in the browser (also on a phone) where the character does what you write in Bengali, with
lighting and physics.

- ✅ **Kotha language v0.1** — compiler + VM; 2600+ word forms; verb forms (`হাঁটো/হাঁটবে/হেঁটে/হাঁটলে`, `walk/walks/walking`) and case
  endings (`খেলোয়াড়ের/কয়েনগুলো`) are recognised automatically; error messages and word suggestions in Bengali and English; a note per
  line showing how it was understood; blocks (`forever:`, `repeat 3 times:`, `else:`), messages between things; a name of
  something the game does not have is caught. [Guide](docs/KOTHA.md) — every example in it is compiled by a test
- ✅ **Physics** — Box2D world; character controller: walking and stopping, a jump of a set height, a jump that still works
  just after walking off an edge (coyote time), a jump pressed just before landing is remembered (jump buffering); touch events
- ✅ **Renderer** — normal-mapped sprites, 32 shadow-casting lights at once, soft shadows, HDR, bloom, ACES tone mapping; a whole
  scene in 14 draw calls ([details](docs/RENDERING.md)). Verified in headless Chromium: no WebGL errors, pictures looked at
  by eye. The 32 shadow lights are verified for correctness, not for speed
- ✅ **Web player** — touch buttons; a script box next to the game — type, press "Run", the character obeys; a broken script
  is refused and the previous one keeps running; Bengali and English interface. Verified in headless Chromium at desktop
  size and with phone emulation (390×780, touch)
- ✅ **Sample game** — player, guard, coins, spikes, door, crates, lamps; all behaviour written in Kotha. In the tests a robot
  plays it from start to finish and wins
- ✅ **Automatic tests** — engine: 51 tests, 6830 checks (also clean when run with memory-error and undefined-behaviour
  checking); browser: 24 checks. Run them with `sh tools/test.sh`, `--asan`, `--web`
- ⬜ **Tried on a real phone** — this one needs you: open the page in your phone's browser and tell me whether it looks right
  and how smoothly it runs (how to open it: [README](README.md)). If it is slow, the quality levels get tuned in this phase
- **Done when:** typing `ডান বোতাম চাপলে ডানে হাঁটো` in the browser and running it makes the character walk right, with lighting and
  shadows on screen — on a computer (✅ done) and on a real phone (⬜ to do)

### Phase 2 — Android APK (M) ⬜
Goal: the same game runs on a phone as a native app.
- ⬜ `engine/platform/android`: GameActivity + EGL + GLES 3.0, touch input, pause/resume, rotation
- ⬜ `android/` Gradle project (arm64-v8a + armeabi-v7a), asset packaging
- ⬜ GitHub Actions: run the tests on every push + build a debug APK as a downloadable artifact
- 🟡 Quality levels: the web player has five (picture and light resolution) and steps down by itself when it runs slowly —
  written, but never seen working on a real phone. To do: the same in the Android app, and tuning the thresholds on real phones
- **Done when:** you can download the APK from GitHub, install it and play the game at 30+ FPS
- **Risk:** cannot be verified from here; the first 2–3 CI runs failing is normal — I will need the results from your phone

### Phase 3 — Editor on the phone (L) ⬜
Goal: build a level with nothing but a phone, without touching code.
- ⬜ Scene editor: place/move/delete objects with a finger, paint tiles, place lights and set their colour and size
- 🟡 Writing scripts: today there is one script per *kind* of thing, and notes and errors appear while you type. To do: a script
  per object, word suggestions while typing
- ⬜ Save/open projects (on the phone + export as a file); scene file format (JSON) — for now, scripts changed in the editor
  are lost when the page is reloaded
- ⬜ PWA: can be put on the home screen and works offline
- ⬜ Play / Stop, single-stepping, showing which rule is running
- **Done when:** a newcomer can build and play a small level on a phone in 15 minutes

### Phase 4 — Content (L) ⬜
- ⬜ Image import (PNG), sprite sheets and animation; a tool that makes a normal map from a picture
- ⬜ Sound and music (`play the "jump" sound`)
- ⬜ Particles (fire, dust, rain), camera control, several levels
- ⬜ Bengali text inside the game (with conjuncts — needs HarfBuzz or the platform's text renderer) and UI (score, buttons, menus)
- ⬜ More physics: joints, ropes, moving platforms, one-way platforms

### Phase 5 — Growing the Kotha language (M, ongoing) ⬜
- ⬜ A larger vocabulary — driven by the sentences real users write and that fail
- ⬜ Naming your own rules (`"attack" means: ...`), arithmetic (`score = score + coins × 2`), creating objects (`create a "bullet"`)
- ⬜ Tolerance for regional and colloquial spellings, Banglish (`dane hato`) — optional
- ⬜ Optional: an online AI assistant that writes Kotha from a free-form description (the engine will not depend on it)
- 🟡 Language guide (Bengali + English): exists for v0.1 ([docs/KOTHA.md](docs/KOTHA.md)); to do: an example collection

### Phase 6 — More advanced lighting (M) ⬜
- ⬜ Indirect light (global illumination, radiance cascades) — light bounces off walls
- ⬜ Shaped lights (torch/spotlight), coloured glass shadows, god rays, reflections in water
- ⬜ Day and night, weather, colour grading
- ⬜ A Vulkan backend if needed

### Phase 7 — Publishing (L) ⬜
- ⬜ One-tap APK/AAB from the editor (cloud build, or injecting the game data into a prebuilt player APK and signing it)
- ⬜ Publishing to the web (share a link and people can play)
- ⬜ Save games, settings, app icon/name; a plugin system for ads and purchases
- ⬜ Three sample games, video/written tutorials (Bengali), the 1.0 release

## 5. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Natural language is ambiguous — the engine may misunderstand | every line shows "this is how I understood it"; when unsure it stops and asks; an automatic test for every sentence pattern |
| Lighting is slow on weak phones (not measured yet) | the light buffer can be drawn at half size, a cap on shadow-casting lights, quality levels; a whole scene is 14 draw calls |
| The APK cannot be verified here | CI + your phone; the platform layer is thin — the language, game rules and physics are tested on a PC, the renderer in a browser |
| Bengali conjuncts inside the game | scheduled as its own task in phase 4; until then text is drawn by the browser/platform |
| Scope creep | a "done when" condition per phase; 3D, multiplayer and a visual node editor are deliberately excluded |
| Building an APK on a phone is hard | cloud build first; later the prebuilt-player + data replacement approach |

## 6. What I need from you

If you do not answer one, I continue with the default written next to it.

1. **Licence** — default: no LICENSE file has been added. If you want the project open to everyone I suggest MIT.
2. **Name of the script language** — default: "কথা / Kotha" (`.kotha` files). Easy to change.
3. **"Engine for mobile"** — I assumed both: (a) an engine for making mobile games, (b) one that can be used on the phone itself. If you want only one, the size of phase 3 changes.
4. **Next phase** — default: phase 2 (Android APK). The editor (phase 3) can come first if you prefer.
5. **Which kind of game first** — default: platformer (side view). Tell me if you want top-down games.

## 7. Working rules

- Every phase ends with three things written down: what was done, how to run it, and what could not be verified.
- Anything that was not tested is never called "done".
- Every new sentence pattern or feature comes with an automatic test.
- This file is the single source of truth — it is updated after every phase.
