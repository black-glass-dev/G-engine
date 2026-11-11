# Third-party licenses

Lumen2D শুধুমাত্র MIT / Apache-2.0 / BSD / Zlib লাইসেন্সের লাইব্রেরি ব্যবহার করে।
নতুন লাইব্রেরি যোগ করলে এই তালিকায় সঙ্গে সঙ্গে লিখতে হবে।

| লাইব্রেরি | ব্যবহার | লাইসেন্স | স্ট্যাটাস |
|---|---|---|---|
| doctest 2.4.11 (github.com/doctest/doctest) | ইউনিট টেস্ট | MIT | M0 থেকে ব্যবহৃত (FetchContent) |
| SDL3 `release-3.4.20` (libsdl-org/SDL) | উইন্ডো, GL কনটেক্সট, ইনপুট, Android লাইফসাইকেল (Java `SDLActivity`) | Zlib | M1 থেকে ব্যবহৃত (FetchContent / CI clone) |
| Box2D v3 (erincatto/box2d) | ফিজিক্স | MIT | M4 থেকে পরিকল্পিত |
| Dear ImGui (stable, docking নয়) | এডিটর UI | MIT | M9 থেকে পরিকল্পিত |
| nlohmann/json | JSON | MIT | M3 থেকে পরিকল্পিত |
| stb_image | ছবি লোড | MIT / Public Domain | M2 থেকে পরিকল্পিত |
| (কোনো GL loader লাইব্রেরি নয়) | GL ফাংশন লোড নিজস্ব কোডে `SDL_GL_GetProcAddress` দিয়ে (D-011) | — | M1 |
| miniaudio (যদি লাগে) | অডিও বিকল্প | MIT / Public Domain | নির্বাচিত নয় (SDL3 audio বেছে নেওয়া হয়েছে) |
