# কথা (Kotha) — ভাষার নির্দেশিকা / Language guide

> **বাংলা প্রথমে, English below.**
> এই ফাইলের প্রতিটা উদাহরণ (` ```kotha ` ব্লক ও টেবিলের বাক্য) স্বয়ংক্রিয় পরীক্ষায় কম্পাইল করা হয়
> (`tests/test_docs.cpp`) — তাই এখানে যা লেখা আছে তা সত্যিই চলে।
> Every example in this file is compiled by an automatic test, so what is written here really works.

---

# বাংলা

## ১. মূল ধারণা

- খেলার প্রতিটা **জিনিসের** (খেলোয়াড়, প্রহরী, কয়েন …) নিজের একটা স্ক্রিপ্ট থাকে। স্ক্রিপ্টের ভেতরে "আমি" মানে সেই জিনিসটা।
- প্রতি লাইনে একটা বাক্য। বাংলা আর ইংরেজি একই স্ক্রিপ্টে মেশানো যায়; সংখ্যা `৩` বা `3` — দুভাবেই চলে।
- লাইন দুই রকম:
  - **সাধারণ কাজ** — যেমন `ডানে হাঁটো`। খেলা শুরু হলে এগুলো উপর থেকে নিচে, একটার পর একটা চলে।
  - **নিয়ম** — যেমন `কয়েন ছুঁলে স্কোর ১ বাড়াও`। শর্ত যখনই সত্যি হয়, তখনই চলে। সব নিয়ম একসাথে সজাগ থাকে।
- এক লাইনে একাধিক কাজ: `লাফ দাও, তারপর ডানে ২ মিটার যাও`।
  অথবা শর্তের শেষে `:` দিয়ে কাজগুলো নিচের লাইনে, একটু ভেতরে সরিয়ে (৪টা স্পেস) লিখুন।
- `#` দিয়ে শুরু হওয়া লাইন মন্তব্য — ইঞ্জিন সেটা পড়ে না।
- এডিটরে প্রতিটা লাইনের পাশে দেখা যায় ইঞ্জিন সেটা **কীভাবে বুঝেছে**। ভুল বুঝলে ওখানেই ধরা পড়বে।

```kotha
# খেলোয়াড়
শুরুতে "চলো!" বলো
ডান বোতাম চাপলে ডানে হাঁটো
বাম বোতাম চাপলে বামে হাঁটো
লাফ বোতাম চাপলে লাফ দাও

কয়েন ছুঁলে:
    স্কোর ১ বাড়াও
    পর্দা কাঁপাও
কাঁটা ছুঁলে শুরুতে ফিরে যাও
স্কোর ৫ হলে খেলা জিতে যাও
```

## ২. কাজ — যা করতে বলা যায়

| কী | বাংলা | English |
|---|---|---|
| হাঁটা, দৌড় | `ডানে হাঁটো` · `বামে দৌড়াও` | `walk right` · `run left` |
| নির্দিষ্ট সময় বা দূরত্ব | `ডানে ২ সেকেন্ড হাঁটো` · `বামে ৩ মিটার যাও` | `walk right for 2 seconds` · `go left 3 meters` |
| উপরে-নিচে (ভাসমান জিনিস: বাতি, কয়েন) | `উপরে ১ মিটার যাও` · `নিচে ১ মিটার যাও` | `go up 1 meter` · `go down 1 meter` |
| খেলোয়াড়ের দিকে / উল্টো দিকে | `খেলোয়াড়ের দিকে হাঁটো` · `খেলোয়াড়ের থেকে দূরে যাও` | `walk toward the player` · `walk away from the player` |
| থামা, ঘোরা | `থামো` · `ঘুরে দাঁড়াও` · `বামে মুখ করো` | `stop` · `turn around` · `face left` |
| লাফ | `লাফ দাও` · `উঁচুতে লাফ দাও` | `jump` · `jump high` |
| অপেক্ষা | `১ সেকেন্ড অপেক্ষা করো` | `wait 1 second` |
| কথা বলা | `"হ্যালো" বলো` · `৩ সেকেন্ড ধরে "সাবধান!" বলো` | `say "hello"` · `say "careful!" for 3 seconds` |
| আলো | `আলো জ্বালাও` · `আলো নেভাও` · `আলোর রং লাল করো` · `উজ্জ্বলতা ৫ করো` | `turn the light on` · `turn the light off` · `make the light red` · `set brightness to 5` |
| নিজের গুণ | `গতি ৭ করো` · `লাফের উচ্চতা ৪ করো` · `আকার ১.৫ করো` · `মাধ্যাকর্ষণ ০.৫ করো` | `set speed to 7` · `set jump height to 4` · `set size to 1.5` · `set gravity to 0.5` |
| গোনা | `স্কোর ১ বাড়াও` · `জীবন ১ কমাও` · `স্কোর ০ করো` | `increase score by 1` · `decrease lives by 1` · `set score to 0` |
| বার্তা পাঠানো | `"খোলো" বার্তা পাঠাও` | `send the message "open"` |
| মুছে যাওয়া, ফিরে যাওয়া | `মুছে যাও` · `শুরুতে ফিরে যাও` | `disappear` · `go back to the start` |
| পর্দা | `পর্দা কাঁপাও` | `shake the screen` |
| খেলা | `খেলা জিতে যাও` · `খেলা শেষ` · `আবার শুরু করো` | `win the game` · `game over` · `restart` |

**রং:** লাল, সবুজ, নীল, হলুদ, সাদা, কমলা, বেগুনি, গোলাপি, আকাশি, সোনালি।
**একক:** দূরত্ব মিটারে (দেয়ালের একটা ইট-ঘর = ১ মিটার), সময় সেকেন্ডে, গতি মিটার/সেকেন্ডে। খেলোয়াড়ের স্বাভাবিক গতি ৪.২, লাফের উচ্চতা ২.৬।

## ৩. কখন — নিয়মের শর্ত

| কখন | বাংলা | English |
|---|---|---|
| খেলা শুরু হলে | `শুরুতে গতি ৭ করো` | `on start, set speed to 7` |
| বোতাম চেপে রাখলে | `ডান বোতাম চাপলে ডানে হাঁটো` | `when the right button is pressed, walk right` |
| বোতাম ছাড়লে | `অ্যাকশন বোতাম ছাড়লে আলো জ্বালাও` | `when the action button is released, turn the light on` |
| কিছুক্ষণ পরপর | `প্রতি ৩ সেকেন্ডে লাফ দাও` | `every 3 seconds, jump` |
| কিছু ছুঁলে | `কয়েন ছুঁলে স্কোর ১ বাড়াও` | `when touching a coin, increase score by 1` |
| দেয়ালে ধাক্কা লাগলে | `দেয়ালে ধাক্কা লাগলে ঘুরে দাঁড়াও` | `when hitting a wall, turn around` |
| মাটিতে নামলে | `মাটিতে নামলে পর্দা কাঁপাও` | `when landing, shake the screen` |
| শূন্যে থাকলে | `শূন্যে থাকলে আলোর রং লাল করো` | `when in the air, make the light red` |
| খেলোয়াড় কাছে এলে | `খেলোয়াড় ৪ মিটারের মধ্যে এলে "কে ওখানে?" বলো` | `when the player is within 4 meters, say "who is there?"` |
| সংখ্যা মিললে | `স্কোর ৫ হলে খেলা জিতে যাও` · `জীবন ১ এর কম হলে খেলা শেষ` | `when score is 5, win the game` · `when lives is less than 1, game over` |
| বার্তা পেলে | `"খোলো" বার্তা পেলে মুছে যাও` | `when the message "open" arrives, disappear` |

**বোতাম:** বাম, ডান, উপর, নিচ, লাফ, অ্যাকশন। ফোনে পর্দার বোতাম; কিবোর্ডে তীর চিহ্ন (বা A / D), Space = লাফ, E বা Enter = অ্যাকশন।

`নাহলে` দিয়ে উল্টো অবস্থায় কী হবে তা বলা যায়:

```kotha
খেলোয়াড় ৫ মিটারের মধ্যে এলে খেলোয়াড়ের দিকে দৌড়াও, নাহলে থামো
```

## ৪. বারবার, কয়েকবার, যদি

```kotha
# প্রহরী — পাহারা দেয়
বারবার:
    বামে ২ সেকেন্ড হাঁটো
    ১ সেকেন্ড অপেক্ষা করো
    ডানে ২ সেকেন্ড হাঁটো
    ১ সেকেন্ড অপেক্ষা করো

খেলোয়াড়কে ছুঁলে "ধরে ফেলেছি!" বলো
```

```kotha
৩ বার:
    লাফ দাও
    ১ সেকেন্ড অপেক্ষা করো
```

```kotha
# দরজা — ৩টা কয়েন আনলে খোলে
খেলোয়াড় কাছে এলে:
    যদি স্কোর ২ এর বেশি হয়:
        "খুলছি..." বলো
        ১ সেকেন্ড অপেক্ষা করো
        মুছে যাও
    নাহলে:
        "৩টা কয়েন আনো" বলো
```

দুটো জিনিস বার্তা দিয়ে কথা বলতে পারে — একজন পাঠায়, অন্যজন শোনে:

```kotha
# খেলোয়াড়ের স্ক্রিপ্টে
অ্যাকশন বোতাম চাপলে "খোলো" বার্তা পাঠাও
```

```kotha
# দরজার স্ক্রিপ্টে
"খোলো" বার্তা পেলে মুছে যাও
```

## ৫. নাম

- **জিনিসের নাম** (নমুনা খেলায়): খেলোয়াড়, প্রহরী, কয়েন, কাঁটা, বাক্স, দরজা, বাতি।
- বিভক্তি ইঞ্জিন নিজে বোঝে: `কয়েনগুলো`, `প্রহরীকে`, `খেলোয়াড়ের`।
- **সংখ্যার নাম** (`স্কোর`, `জীবন`, বা যেকোনো নতুন শব্দ) আর **বার্তার নাম** আপনি নিজে বানাতে পারেন।
  সংখ্যাগুলো সবার জন্য এক: খেলোয়াড় `স্কোর` বাড়ালে দরজা সেটাই দেখে।
- `স্কোর` আর `score` একই জিনিস (তেমনি জীবন / lives, কয়েন / coin, দরজা / door …) — তাই বাংলা আর ইংরেজি স্ক্রিপ্ট একসাথে কাজ করে।

## ৬. ইঞ্জিন না বুঝলে

ইঞ্জিন আন্দাজে কিছু চালায় না। ভুল থাকলে নতুন স্ক্রিপ্ট বসে না — আগেরটাই চলতে থাকে — আর কোথায় সমস্যা তা বলে দেয়:

```text
ডানে হাঁটু
  ✗ «হাঁটু» শব্দটা বুঝতে পারিনি। আপনি কি «হাঁটো» বোঝাতে চেয়েছেন?

ভূত ছুঁলে লাফ দাও
  ✗ এই খেলায় «ভূত» নামে কিছু নেই। যা আছে: খেলোয়াড়, প্রহরী, কয়েন, কাঁটা, বাক্স, দরজা, বাতি।

বারবার:
    লাফ দাও
ডানে হাঁটো
  ✗ এই লাইন কখনো চলবে না: উপরের «বারবার» (লাইন ১) কখনো শেষ হয় না। …
```

## ৭. এখন যা পারে না

- হিসাব (`স্কোর = স্কোর × ২`), নিজের বানানো কাজের নাম, নতুন জিনিস তৈরি করা — [রোডম্যাপের](../ROADMAP.md) ধাপ ৫।
- শব্দ, অ্যানিমেশন, কণা — ধাপ ৪।
- মুক্ত বাংলা। এটা AI নয়: শব্দভাণ্ডারের (২৬০০+ শব্দরূপ) আর চেনা বাক্য-গঠনের বাইরে গেলে ইঞ্জিন বলবে "বুঝতে পারিনি"।

## ৮. কম্পিউটারে যাচাই

```sh
sh tools/test.sh                                             # সব পরীক্ষা
./build/native/tests/kotha_check --game আমার_স্ক্রিপ্ট.kotha   # প্রতিটা লাইন কীভাবে বোঝা হলো
```

`--game` থাকলে খেলার মতোই যাচাই হয়: `ভূত ছুঁলে …` লিখলে বলে দেয় খেলায় «ভূত» নামে কিছু নেই।

---

# English

## 1. The idea

- Every **thing** in the game (player, guard, coin …) has a script of its own. Inside a script, "I" is that thing.
- One sentence per line. Bengali and English can be mixed in one script; numbers may be written `৩` or `3`.
- There are two kinds of lines:
  - **plain actions** — `walk right`. When the game starts they run from top to bottom, one after the other.
  - **rules** — `when touching a coin, increase score by 1`. They run whenever their condition becomes true; all rules are on watch at the same time.
- Several actions on one line: `jump, then go right 2 meters`.
  Or end the condition with `:` and write the actions on the following lines, indented (4 spaces).
- A line that begins with `#` is a comment.
- In the editor every line shows **how the engine understood it** — a misunderstanding is visible right there.

```kotha
# player
on start, say "let's go!"
when the right button is pressed, walk right
when the left button is pressed, walk left
when the jump button is pressed, jump

when touching a coin:
    increase score by 1
    shake the screen
when touching spikes, go back to the start
when score is 5, win the game
```

## 2. Actions and conditions

The two tables in the Bengali part above list every sentence in both languages (third column).

**Colours:** red, green, blue, yellow, white, orange, purple, pink, cyan, gold.
**Units:** distances in meters (one wall tile = 1 meter), time in seconds, speed in meters per second. The player's normal speed is 4.2 and jump height 2.6.
**Buttons:** left, right, up, down, jump, action. On a phone these are the on-screen buttons; on a keyboard the arrow keys (or A / D), Space = jump, E or Enter = action.

`else` says what happens in the opposite case:

```kotha
when the player is within 5 meters, run toward the player, else stop
```

## 3. Forever, a number of times, if

```kotha
# guard — patrols
forever:
    walk left for 2 seconds
    wait 1 second
    walk right for 2 seconds
    wait 1 second

when touching the player, say "got you!"
```

```kotha
repeat 3 times:
    jump
    wait 1 second
```

```kotha
# door — opens for 3 coins
when the player is near:
    if score is more than 2:
        say "opening..."
        wait 1 second
        disappear
    else:
        say "bring 3 coins"
```

Two things can talk through messages — one sends, the other listens:

```kotha
# in the player's script
when the action button is pressed, send the message "open"
```

```kotha
# in the door's script
when the message "open" arrives, disappear
```

## 4. Names

- **Things** (in the sample game): player, guard, coin, spikes, box (or crate), door — the same things as খেলোয়াড়, প্রহরী, কয়েন, কাঁটা, বাক্স, দরজা.
- Plurals are understood: `coins`, `boxes`.
- **Numbers** (`score`, `lives`, or any new word) and **messages** are yours to invent.
  Numbers are shared by everything: when the player increases `score`, the door sees the same `score`.
- `score` and `স্কোর` are one and the same (likewise lives / জীবন, coin / কয়েন, door / দরজা …), so Bengali and English scripts work together.

## 5. When the engine does not understand

It never guesses. A script with a mistake is not installed — the previous one keeps running — and the engine says where the problem is:

```text
wlak right
  ✗ I don't know the word "wlak". Did you mean "walk"?

when touching a ghost, jump
  ✗ There is nothing called "ghost" in this game. There is: player, guard, coin, spike, box, door, lamp.

forever:
    jump
walk right
  ✗ This line would never run: the "forever" above (line 1) never ends. …
```

## 6. What it cannot do yet

- Arithmetic (`score = score × 2`), naming your own actions, creating new things — phase 5 of the [roadmap](../ROADMAP.md).
- Sound, animation, particles — phase 4.
- Free-form language. This is not AI: outside its vocabulary (2600+ word forms) and its known sentence patterns the engine says "I don't understand".

## 7. Checking on a computer

```sh
sh tools/test.sh                                          # all tests
./build/native/tests/kotha_check --game my_script.kotha   # how every line is understood
```

With `--game` a script is checked the way the game checks it: `when touching a ghost, …` is reported, because the game has nothing called "ghost".
