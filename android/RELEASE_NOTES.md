# G-Engine mobile

## সরাসরি ডাউনলোড (APK)

**[g-engine.apk ডাউনলোড করুন](https://github.com/black-glass-dev/G-engine/releases/download/android-latest/g-engine.apk)**

ফোনে ডাউনলোড করে "Install unknown apps" চালু করে ইনস্টল করুন — হয়ে গেল **G-Engine Studio**, অর্থাৎ গেম ইঞ্জিন নিজেই।

## এই অ্যাপে কী আছে

এটা আর শুধু ডেমো নয় — **গেম ইঞ্জিনটা নিজেই** ফোনে চলে:

- **লেভেল এডিটর** — ডান পাশের প্যালেট থেকে ব্রাশ বেছে নিয়ে টেপে/টেনে মানচিত্র আঁকুন
- **কথা স্ক্রিপ্ট এডিটর** — "যখন খেলোয়াড় কয়েন ছোঁয়, তখন কয়েন লুকাও। স্কোর ১ বাড়াও।"
- **নতুন গেম / নমুনা গেম** — খালি ঘর বা ডেমো দিয়ে শুরু করুন
- **সেভ** — খেলা ও সম্পাদনা ফোনেই থাকে; পরের বার ঠিক যেখানে ছিলেন সেখান থেকে
- **Export / Import (.json)** — নিজের গেম ফাইল হিসেবে বাঁচান বা শেয়ার করুন

সব কিছু অফলাইনে চলে (কোনো ইন্টারনেট লাগে না; অনুমতিও কেবল ভেতরের সার্ভারের জন্য)।

## কীভাবে খেলবেন

আঁকার সময় `P` হলো খেলোয়াড়ের শুরু, `G` গার্ড, `c` কয়েন, `D` দরজা, `L` বাতি। খেলার সময় স্ক্রিনের নিচের বোতামগুলো — বামে/ডানে, লাফ, কাজ। সম্পূর্ণ নির্দেশিকা [docs/KOTHA.md](../docs/KOTHA.md)।

## স্ক্রিনশট

ডাউনলোড পদ্ধতি নিচের ছবিতে ([download.jpg](https://github.com/black-glass-dev/G-engine/releases/download/android-latest/download.jpg))। ফোনের সত্যিকারের স্ক্রিনশট ও গেম প্লে ছবি রিলিজে [game.jpg](https://github.com/black-glass-dev/G-engine/releases/download/android-latest/game.jpg)।

## English

This is **G-Engine Studio** — the game engine itself on your phone: draw a level with the
palette, write the rules in Kotha ("When player touches coin, hide the coin. Increase score
by 1."), play, save, and export games as `.json` files. Everything runs offline: the same
C++ engine as the desktop build, compiled to WebAssembly inside a WebView. Install
**[g-engine.apk](https://github.com/black-glass-dev/G-engine/releases/download/android-latest/g-engine.apk)**
(allow "install unknown apps" first). Signed with a build-time keystore — for updates,
uninstall the old version first.

The same studio also runs in any browser at `sh tools/serve.py` → http://localhost:8080/
(web/ folder), and every push builds a fresh APK with GitHub Actions.
