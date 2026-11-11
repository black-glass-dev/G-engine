# Lumen2D

**Lumen2D** একটি মোবাইল-ফার্স্ট 2D গেম ইঞ্জিন, যার বড় শক্তি দুটি: **রিয়েল-টাইম 2D লাইটিং** (1D শ্যাডো ম্যাপ, সফট শ্যাডো, নরমাল ম্যাপ, বloom, ACES টোন ম্যাপিং, ভলিউমেট্রিক ইফেক্ট) এবং **Box2D v3-ভিত্তিক বাস্তবসম্মত ফিজিক্স**।

> 📌 বর্তমান স্ট্যাটাস: **M0 — ভিত্তি** সম্পন্ন। বিস্তারিত ও পরের ধাপ: [PROGRESS.md](PROGRESS.md)।

## কাঠামো

| ফোল্ডার | কাজ |
|---|---|
| `engine/` | `liblumen` কোর লাইব্রেরি (রেন্ডার, লাইটিং, ফিজিক্স, ECS, স্ক্রিপ্ট VM, অডিও) |
| `editor/` | ভিজ্যুয়াল এডিটর (Dear ImGui) — ডেস্কটপ ও Android-এ একই কোড |
| `player/` | স্ট্যান্ডঅ্যালোন প্লেয়ার (শুধু গেম চালায়) |
| `agentkit/` | GUI-ছাড়া CLI `lumen` — এজেন্ট ও মানুষের জন্য |
| `android/` | Gradle শেল প্রজেক্ট (SDL3 + CMake) → APK |
| `games/samples/` | ৩টি নমুনা গেম |
| `games/templates/` | ৩টি জঁর টেমপ্লেট |
| `schemas/` | সব JSON ফরম্যাটের JSON Schema |
| `docs/` | ডকুমেন্টেশন (বাংলা ও ইংরেজি) |
| `tests/` | ইউনিট ও হেডলেস স্মোক টেস্ট |
| `.github/workflows/` | CI: build-cli, build-android, test, release |

## দ্রুত শুরু (ডেস্কটপ / CLI)

প্রয়োজন: C++20 কম্পাইলার (g++ 12+ / clang 15+ / MSVC 2022), CMake 3.22+, Ninja।

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/agentkit/lumen version
```

## ⚠️ মোবাইল এডিটরের সীমাবদ্ধতা

- মোবাইলে এডিটর থেকে **সরাসরি নতুন সাইনড APK তৈরি সম্ভব নয়।**
- এডিটরের এক্সপোর্ট হবে **`.lumenpack`** ফাইল (ইমপোর্ট/এক্সপোর্ট করা যাবে)।
- স্ট্যান্ডঅ্যালোন APK তৈরি হবে **GitHub Actions** বা **CLI** (`lumen build --target android`) দিয়ে।

## লাইসেন্স

ইঞ্জিন নির্ভরতা শুধু MIT / Apache-2.0 / BSD / Zlib। বিস্তারিত: [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)।
