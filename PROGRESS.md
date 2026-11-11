# অগ্রগতি (PROGRESS)

> এই ফাইল পড়ে যেকোনো সেশন ঠিক এখান থেকে শুরু করতে পারবে। প্রতিটি মাইলস্টোন শেষে আপডেট হবে।

## 📌 বর্তমান অবস্থা

**মাইলস্টোন: M0 — ভিত্তি** (সম্পন্ন, কমিট-পুশ-পরবর্তী)
**ব্রাঞ্চ:** `arena/857c23fa-g-engine` (D-001 দেখো)

## ✅ M0 — হয়ে গেছে

- পুরনো ফাইল `legacy_archive_DO_NOT_USE/`-এ move (কিছু ডিলিট হয়নি)।
- নতুন ফোল্ডার কাঠামো: `engine/ editor/ player/ agentkit/ android/ games/{samples,templates} schemas/ docs/ tests/ tools/ .github/workflows/`
- Root `CMakeLists.txt` (C++20, `LUMEN_BUILD_TESTS`, `LUMEN_BUILD_AGENTKIT` অপশন)।
- `engine/`: `liblumen` স্কেলেটন — `lumen::version`, ট্যাগড `lumen::log` (সিঙ্ক সহ)।
- `agentkit/`: CLI `lumen` — `version`, `help` কাজ করে; অজানা কমান্ড exit code 2।
- `tests/`: doctest টেস্ট — লগ সিঙ্ক ও ভার্সন।
- `docs/DECISIONS.md` (D-001 … D-010), `docs/LIGHTING_BN.md` (টিয়ার টেবিল), `THIRD_PARTY_LICENSES.md`।
- `.gitignore`, `README.md` (বাংলা, মোবাইল এডিটরের সীমাবদ্ধতাসহ)।
- CI: `.github/workflows/build-cli.yml` (build + ctest + artifact)।

## 🧪 টেস্ট

- লোকাল (Linux, g++ 12, CMake 4.4, Ninja): **3 টেস্ট কেস, 8 অ্যাসারশন — 8 পাস / 0 ফেল।**
- CI-এর সবুজ স্ট্যাটাস: পুশের পরে যাচাই করা হবে (নিচে দেখো)।

## ⏭️ পরের ধাপ

- **M1 — উইন্ডো ও ত্রিভুজ:** SDL3 + OpenGL ES 3.0 / GL 3.3 রেন্ডারার স্কেলেটন, ডেস্কটপ উইন্ডো, Android Gradle প্রজেক্ট, `build-android.yml` এবং প্রথম APK।
- তারপর M2 (2D রেন্ডারার), M3 (ECS + সিন ফরম্যাট + `lumen validate`), M4 (Box2D)।

## ⚠️ অসম্পূর্ণ / জানা সীমাবদ্ধতা

- CLI-এর `new`, `validate`, `run`, `test`, `build`, `pack`, `docs` এখনো নেই — M3/M10-এ আসবে। এগুলো "সম্পন্ন" বলে দাবি করা হচ্ছে না।
- লাইটিং, ফিজিক্স, রেন্ডারার, স্ক্রিপ্ট VM, এডিটর — কোনোটিই এখনো শুরু হয়নি।
- `build-android.yml`, `schemas/`, `games/`, `docs/AGENT.md`, `docs/USER_GUIDE_BN.md`, `docs/SCRIPTING_BN.md` — এখনো নেই।
