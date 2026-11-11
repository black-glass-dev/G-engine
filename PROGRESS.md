# অগ্রগতি (PROGRESS)

> এই ফাইল পড়ে যেকোনো সেশন ঠিক এখান থেকে শুরু করতে পারবে। প্রতিটি মাইলস্টোন শেষে আপডেট হবে।

## 📌 বর্তমান অবস্থা

**মাইলস্টোন: M1 — উইন্ডো ও ত্রিভুজ** (কোড লেখা ও পুশ হয়েছে; CI যাচাই চলছে — নিচে দেখো)
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

- লোকাল (Linux, g++ 12): শেডার-রিরাইট ও GL-loader-এর "মিসিং ফাংশন" চেক ম্যানুয়ালি পাস; renderer/shader/gl_api `-Wall -Wextra -Wpedantic` warning-মুক্ত কম্পাইল। (লোকালে X11 ডেভ প্যাকেজ নেই, তাই পুরো SDL বিল্ড লোকালি হয়নি — D-015।)
- doctest ইউনিট টেস্ট: শেডার-রিরাইট ৪ কেস যোগ হয়েছে (মোট কেস বাড়বে; সঠিক সংখ্যা CI রানের ফলাফলে)।
- CI `build-cli` ও `build-android`: পুশের পরে যাচাই। ফলাফল নিচে আপডেট হবে।

## ⏭️ পরের ধাপ

- M1 CI সবুজ নিশ্চিত করা (লাল হলে ঠিক করা — নিয়ম অনুযায়ী এটাই অগ্রাধিকার)।
- M2 — 2D রেন্ডারার: স্প্রাইট ব্যাচিং, টেক্সচার অ্যাটলাস, ক্যামেরা, বিটম্যাপ টেক্সট, পারফ কাউন্টার ওভারলে।

## ⚠️ অসম্পূর্ণ / জানা সীমাবদ্ধতা

- Android-এ অ্যাপ ব্যাকগ্রাউন্ডে গেলে GL কনটেক্সট রিস্টোর পরীক্ষিত নয় (SDL3 লাইফসাইকেল হ্যান্ডলিং পরে যাচাই হবে)।
- Windows-এ `APIENTRY` (`__stdcall`) সাপোর্ট কোডে আছে, কিন্তু Windows বিল্ড CI-তে এখনো নেই।
- CLI-এর `new`, `validate`, `run`, `test`, `build`, `pack`, `docs` এখনো নেই (M3/M10)।
- লাইটিং, ফিজিক্স, স্ক্রিপ্ট VM, এডিটর, অডিও — এখনো শুরু হয়নি।
- Editor (`editor/`) ও `schemas/`, `games/` ফোল্ডার শুধু `.gitkeep` সহ।
