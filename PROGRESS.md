# অগ্রগতি (PROGRESS)

> এই ফাইল পড়ে যেকোনো সেশন ঠিক এখান থেকে শুরু করতে পারবে। প্রতিটি মাইলস্টোন শেষে আপডেট হবে।

## 📌 বর্তমান অবস্থা

**মাইলস্টোন: M1 — উইন্ডো ও ত্রিভুজ** ✅ সম্পন্ন — CI `build-cli` ও `build-android` দুটোই সবুজ (commit `f2ecaaa`)
**ব্রাঞ্চ:** `arena/857c23fa-g-engine` · **PR:** #1 (main-এর সাথে) · D-001 দেখো

## ✅ M0 — সম্পন্ন (CI সবুজ)

- পুরনো ফাইল `legacy_archive_DO_NOT_USE/`-এ move (কিছু ডিলিট হয়নি)।
- ফোল্ডার কাঠামো, root CMake (C++20), `liblumen` স্কেলেটন (version, tagged log), `lumen` CLI (`version`, `help`), doctest টেস্ট, docs, `build-cli` CI।

## 🔨 M1 — যা করা হয়েছে

- **Engine:** `engine/src/render/gl_api.cpp` (GL function table, X-macro, `SDL_GL_GetProcAddress`), `shader.cpp` (GLSL ES 3.00 → ডেস্কটপে `330 core` রিরাইট, compile/link + লগ), `renderer.cpp` (clear + রঙিন ত্রিভুজ পাইপলাইন, RAII cleanup), `platform/app.cpp` (SDL3 উইন্ডো, GL 3.3 core / GLES 3.0 কনটেক্সট, ইভেন্ট+ফ্রেম লুপ, `--frames` সমর্থন)।
- **Player:** `player/main.cpp` — `lumen_player [--frames N] [--title T]`। ব্যাকগ্রাউন্ড পাল্স + ঘূর্ণায়মান ত্রিভুজ।
- **Android:** `android/` Gradle শেল প্রজেক্ট (AGP 8.5.2), `app/src/main/cpp/CMakeLists.txt` (SDL3 shared + liblumen + lumen_player), `LumenActivity` (SDLActivity সাবক্লাস), `AndroidManifest.xml` (GLES 3.0 requirement)।
- **CI:** `build-cli.yml` — Xvfb-তে `lumen_player --frames 120` স্মোক টেস্ট, player artifact। নতুন `build-android.yml` — SDK/NDK/CMake সেটআপ, SDL3 ক্লোন, `gradle :app:assembleDebug`, APK artifact, tag `v*`-এ release-এ সংযুক্ত।
- **ডকুমেন্ট:** `docs/DECISIONS.md` D-011 … D-015, `THIRD_PARTY_LICENSES.md` আপডেট।

## 🧪 টেস্ট

### লোকাল টেস্ট-রান (সেশন ৩, Linux, g++ 12, CMake 4, Ninja)
- লোকালি X11 নেই, তাই SDL বিল্ড হয়েছে `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF` সহ (dummy video; GL উইন্ডো নয়)।
- doctest: **13 টেস্ট কেস, 32 অ্যাসারশন — সব পাস।** (লগ ২, ভার্সন ১, শেডার ৪, GL loader/renderer ৬)
- `ctest`: **16/16 পাস** — ২টি CLI টেস্ট (`cli_version`, `cli_unknown_command_fails`), ১টি প্লেয়ার আর্গুমেন্ট টেস্ট।
- `lumen_player --frames 5` ডিসপ্লে ছাড়া: স্পষ্ট এরর `SDL_Init failed: No available video device` + exit 1 (ক্র্যাশ নয়)।
- GL-নির্ভর পাথ (আসল শেডার কম্পাইল, ত্রিভুজ আঁকা) লোকালি চলে না — CI-র Xvfb স্মোক টেস্টে যাচাই হয়।

### পূর্ববর্তী CI (commit `f2ecaaa`)

- লোকাল (Linux, g++ 12): শেডার-রিরাইট ও GL-loader-এর "মিসিং ফাংশন" চেক ম্যানুয়ালি পাস; renderer/shader/gl_api `-Wall -Wextra -Wpedantic` warning-মুক্ত কম্পাইল। (লোকালে X11 ডেভ প্যাকেজ নেই, তাই পুরো SDL বিল্ড লোকালি হয়নি — D-015।)
- CI `build-cli` (commit `f2ecaaa`): configure ✅ · build ✅ · ctest ✅ (তখন ৭টি টেস্ট কেস) · Xvfb-তে `lumen_player --frames 120` ✅ · artifacts: `lumen-cli-linux`, `lumen-player-linux`।
- CI `build-android` (commit `f2ecaaa`): SDK/NDK/CMake ✅ · Gradle 8.9 ✅ · `:app:assembleDebug` ✅ · artifact `lumen2d-debug-apk` (≈2.5 MB, arm64-v8a + x86_64)।
- ব্যর্থতার ইতিহাস (শিক্ষা): প্রথমে SDL3 configure ব্যর্থ — X11 এক্সটেনশন `libxfixes-dev`, `libxtst-dev` বাদ ছিল (SDL `SDL_missing_dependency` ফatal)। ঠিক করা হয়েছে। সাথে `setup-android` ও `setup-gradle` সরিয়ে runner-এর প্রি-ইনস্টল SDK + সরাসরি Gradle ডাউনলোড।
- **ডিবাগ নোট:** sandbox থেকে GitHub Actions-এর লগ/আর্টিফ্যাক্ট ব্লব-স্টোরেজ থেকে ডাউনলোড হয় না (আউটবাউন্ড অনুমতি নেই)। তাই ব্যর্থ CI-র কারণ জানতে `configure-diagnostics` artifact ও `gh run view --json jobs` ব্যবহার করতে হয়।

## ⏭️ পরের ধাপ

- M1-এর Android APK এখনো শুধু debug-signed ও শুধু প্লেয়ার — এডিটর-সহ APK M9-এ।
- ডিভাইসে APK চালিয়ে দেখা (emulator/ফোন) — এখন শুধু বিল্ড যাচাই হয়েছে, ডিভাইস রান নয়।
- M2 — 2D রেন্ডারার: স্প্রাইট ব্যাচিং, টেক্সচার অ্যাটলাস, ক্যামেরা, বিটম্যাপ টেক্সট, পারফ কাউন্টার ওভারলে।

## ⚠️ অসম্পূর্ণ / জানা সীমাবদ্ধতা

- Android Activity পজ/রিজিউমে GL কনটেক্সট পুনরুদ্ধার ডিভাইসে যাচাই হয়নি।

- Windows-এ `APIENTRY` (`__stdcall`) সাপোর্ট কোডে আছে, কিন্তু Windows বিল্ড CI-তে এখনো নেই।
- CLI-এর `new`, `validate`, `run`, `test`, `build`, `pack`, `docs` এখনো নেই (M3/M10)।
- লাইটিং, ফিজিক্স, স্ক্রিপ্ট VM, এডিটর, অডিও — এখনো শুরু হয়নি।
- Editor (`editor/`) ও `schemas/`, `games/` ফোল্ডার শুধু `.gitkeep` সহ।
