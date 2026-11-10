// G-Engine — web/app.js
// The page around the engine: canvas size, keyboard and touch buttons, the script editor.

import { loadEngine, Button, Option } from './engine.js';
import { examples } from './examples.js';

const $ = (id) => document.getElementById(id);

// ---- words of the page, in two languages -------------------------------------------------
const strings = {
  bn: {
    tagline: 'নিজের গেম বানানোর ইঞ্জিন — লেভেল এঁকে, সহজ বাংলা বা ইংরেজি বাক্যে নিয়ম লিখে',
    other: 'English',
    score: 'স্কোর',
    jump: 'লাফ',
    keys: '← →  চলা   ·   Space  লাফ   ·   E  অ্যাকশন',
    lighting: 'আলো-ছায়া',
    bloom: 'আভা',
    restart: 'আবার শুরু',
    again: 'আবার খেলো',
    won: 'জিতে গেছ!',
    lost: 'খেলা শেষ',
    editorTitle: 'এখানেই আপনার গেম',
    editorLead: '«লেভেল বানাও» ট্যাবে মানচিত্র এঁকে জিনিস বসান, «আচরণ লেখো» ট্যাবে প্রতিটা জিনিসের নিয়ম লিখুন। বাংলা আর ইংরেজি — দুটোই চলে, আর উপরের পর্দায় সাথে সাথে ফল দেখা যায়।',
    run: 'চালাও',
    reset: 'আগের মতো করো',
    understood: 'ইঞ্জিন যা বুঝেছে',
    tryThese: 'এগুলো যোগ করে দেখো',
    nothing: 'এখানে এখনো কিছু লেখা নেই।',
    running: 'চলছে',
    edited: 'বদলেছ — «চালাও» চাপো',
    notUnderstood: (n) => `${n}টা জায়গা বুঝতে পারিনি — ঠিক করে আবার চালাও`,
    stats: (s) => `${s.lights}টি আলো, তার ${s.shadows}টি ছায়া ফেলছে  ·  ${s.draws}টি ড্র-কল  ·  ${s.size}`,
    fps: (n) => `${n} fps`,
    loading: 'ইঞ্জিন চালু হচ্ছে…',
    noWebgl: 'এই ব্রাউজারে WebGL2 নেই, তাই গেমের ছবি আঁকা যাচ্ছে না।',
    noWasm: 'ইঞ্জিনের ফাইল (g-engine.wasm) পাওয়া যায়নি। আগে tools/build_web.sh চালাও।',
    failed: 'ইঞ্জিন চালু করা গেল না। বিস্তারিত ব্রাউজারের কনসোলে আছে।',
    footer: 'G-Engine — C++ ইঞ্জিন (WebAssembly), নিজস্ব আলো-ছায়ার রেন্ডারার, Box2D পদার্থবিদ্যা, আর «কথা» ভাষা। এই পাতায় যা চলছে, অ্যান্ড্রয়েডেও সেই একই কোড চলবে। আপনার খেলা ব্রাউজারে সংরক্ষিত থাকে; .json ফাইলে নামিয়ে বাঁধানো বা আবার লোড করা যায়।',
    labels: { left: 'বামে', right: 'ডানে', jump: 'লাফ', action: 'অ্যাকশন' },
    newGame: 'নতুন খেলা',
    loadSample: 'নমুনা খেলা',
    saveGame: 'সংরক্ষণ',
    exportGame: 'নামাও (.json)',
    importGame: 'লোড করো',
    saved: 'সংরক্ষিত হলো',
    saveFailed: 'সংরক্ষণ করা গেল না',
    loaded: 'লোড হলো',
    badFile: 'ফাইলটা পড়া গেল না',
    levelTab: 'লেভেল বানাও',
    behaviorTab: 'আচরণ লেখো',
    levelLead: 'ট্যাপ বা ড্র্যাগ করে মানচিত্র এঁকুন — দেয়াল, কয়েন, বাতি, দরজা… উপরের পর্দায় সাথে সাথে দেখা যাবে।',
    levelText: 'লেভেলের ছবি (সরাসরিও লিখতে পারেন)',
    levelHint: 'প্রতিটা ঘর ১×১ মিটার। খেলোয়াড় P একটাই থাকবে — নতুন P বসালে আগেরটা সরে যায়।',
    brushes: { '#': 'দেয়াল', '.': 'ফাঁকা', 'P': 'খেলোয়াড়', 'G': 'প্রহরী', 'c': 'কয়েন', '^': 'কাঁটা', 'B': 'বাক্স', 'D': 'দরজা', 'L': 'বাতি' },
    names: {},
  },
  en: {
    tagline: 'Your game-making engine — draw a level, write the rules in plain Bengali or English',
    other: 'বাংলা',
    score: 'Score',
    jump: 'Jump',
    keys: '← →  move   ·   Space  jump   ·   E  action',
    lighting: 'Lighting',
    bloom: 'Bloom',
    restart: 'Restart',
    again: 'Play again',
    won: 'You won!',
    lost: 'Game over',
    editorTitle: 'Make your game here',
    editorLead: 'Draw the map in the “Build the level” tab and write what each thing does in the “Write behaviour” tab. Both Bengali and English work, and the screen above shows the result right away.',
    run: 'Run',
    reset: 'Restore original',
    understood: 'What the engine understood',
    tryThese: 'Try adding these',
    nothing: 'Nothing is written here yet.',
    running: 'Running',
    edited: 'Edited — press Run',
    notUnderstood: (n) => `${n} place${n === 1 ? '' : 's'} not understood — fix and run again`,
    stats: (s) => `${s.lights} lights, ${s.shadows} casting shadows  ·  ${s.draws} draw calls  ·  ${s.size}`,
    fps: (n) => `${n} fps`,
    loading: 'Starting the engine…',
    noWebgl: 'This browser has no WebGL2, so the game cannot be drawn.',
    noWasm: 'The engine file (g-engine.wasm) is missing. Run tools/build_web.sh first.',
    failed: 'The engine could not start. Details are in the browser console.',
    footer: 'G-Engine — a C++ engine (WebAssembly), its own lighting renderer, Box2D physics and the "Kotha" language. The code running on this page is the same code that runs on Android. Your game stays saved in the browser; download it as a .json file to share or load it back.',
    labels: { left: 'Left', right: 'Right', jump: 'Jump', action: 'Action' },
    newGame: 'New game',
    loadSample: 'Sample game',
    saveGame: 'Save',
    exportGame: 'Download .json',
    importGame: 'Load file',
    saved: 'Saved',
    saveFailed: 'Could not save',
    loaded: 'Loaded',
    badFile: 'That file could not be read',
    levelTab: 'Build the level',
    behaviorTab: 'Write behaviour',
    levelLead: 'Tap or drag to draw the map — walls, coins, lamps, a door… The screen above shows it right away.',
    levelText: 'The level picture (you can also type it)',
    levelHint: 'Each cell is 1×1 meter. There is only one player P — placing a new one removes the old.',
    brushes: { '#': 'Wall', '.': 'Empty', 'P': 'Player', 'G': 'Guard', 'c': 'Coin', '^': 'Spikes', 'B': 'Crate', 'D': 'Door', 'L': 'Lamp' },
    // The things in the game are named in Bengali; on the English page the tabs show both.
    names: { 'খেলোয়াড়': 'Player', 'প্রহরী': 'Guard', 'কয়েন': 'Coin', 'দরজা': 'Door', 'বাতি': 'Lamp', 'বাক্স': 'Crate', 'কাঁটা': 'Spikes' },
  },
};

const stored = (() => { try { return localStorage.getItem('ge-lang'); } catch { return null; } })();
let lang = new URLSearchParams(location.search).get('lang') || stored || 'bn';
if (!strings[lang]) lang = 'bn';
const t = (key) => strings[lang][key];
const digits = (value) => (lang === 'bn' ? String(value).replace(/\d/g, (d) => '০১২৩৪৫৬৭৮৯'[d]) : String(value));

// ---- state -------------------------------------------------------------------------------
let engine = null;
let scripts = [];      // [{name, original, installed, draft}]
let current = 0;
let lastCheck = null;  // result of the last compile of the draft
let statusKind = '';   // '', 'ok', 'bad', 'edited'

const canvas = $('canvas');
const screen = $('screen');
const code = $('code');

function applyLanguage() {
  document.documentElement.lang = lang;
  for (const el of document.querySelectorAll('[data-t]')) el.textContent = t(el.dataset.t);
  for (const el of document.querySelectorAll('[data-label]')) el.setAttribute('aria-label', t('labels')[el.dataset.label]);
  $('lang').textContent = t('other');
  renderPalette();
  if (engine) {
    renderTabs();
    renderNotes();
    renderExamples();
    renderStatus();
    lastHudText = '';
  }
}

$('lang').addEventListener('click', () => {
  lang = lang === 'bn' ? 'en' : 'bn';
  try { localStorage.setItem('ge-lang', lang); } catch { /* private mode */ }
  applyLanguage();
});

// ---- picture quality ---------------------------------------------------------------------
// Phones differ a lot. The page starts sharp and, if the frame rate stays low, steps down:
// first the light buffer, then the resolution. (?quality=0..4 in the address fixes the level.)
const qualityLevels = [
  { ratio: 2, light: 1 },
  { ratio: 2, light: 0.5 },
  { ratio: 1.5, light: 0.5 },
  { ratio: 1, light: 0.5 },
  { ratio: 0.75, light: 0.35 },
];
const params = new URLSearchParams(location.search);
const fixedQuality = params.has('quality') ? Math.max(0, Math.min(qualityLevels.length - 1, Number(params.get('quality')) || 0)) : -1;
let quality = fixedQuality >= 0 ? fixedQuality : (matchMedia('(pointer: coarse)').matches ? 1 : 0);

function applyQuality() {
  if (engine) engine.option(Option.lightScale, qualityLevels[quality].light);
  resizeCanvas();
}

// ---- canvas size -------------------------------------------------------------------------
function resizeCanvas() {
  const ratio = Math.min(window.devicePixelRatio || 1, qualityLevels[quality].ratio);
  const width = Math.max(2, Math.round(screen.clientWidth * ratio));
  const height = Math.max(2, Math.round(screen.clientHeight * ratio));
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
    if (engine) engine.resize(width, height);
  }
}

// ---- input -------------------------------------------------------------------------------
// A button is down while at least one key or finger holds it.
const holders = new Map();
function hold(name, source, down) {
  let set = holders.get(name);
  if (!set) holders.set(name, (set = new Set()));
  const was = set.size > 0;
  if (down) set.add(source); else set.delete(source);
  if (engine && was !== set.size > 0) engine.button(Button[name], set.size > 0);
}
function releaseAll() {
  for (const [name, set] of holders) {
    if (set.size > 0 && engine) engine.button(Button[name], false);
    set.clear();
  }
  for (const el of document.querySelectorAll('.pad button.down')) el.classList.remove('down');
}

const keyMap = {
  ArrowLeft: ['left'], KeyA: ['left'],
  ArrowRight: ['right'], KeyD: ['right'],
  ArrowUp: ['up', 'jump'], KeyW: ['up', 'jump'], Space: ['jump'],
  ArrowDown: ['down'], KeyS: ['down'],
  KeyE: ['action'], Enter: ['action'],
};
const typing = (target) => target instanceof HTMLElement && (target.tagName === 'TEXTAREA' || target.tagName === 'INPUT' || target.tagName === 'BUTTON' || target.tagName === 'SELECT');

window.addEventListener('keydown', (event) => {
  const names = keyMap[event.code];
  if (!names || typing(event.target) || event.ctrlKey || event.metaKey || event.altKey) return;
  event.preventDefault();
  for (const name of names) hold(name, event.code, true);
});
window.addEventListener('keyup', (event) => {
  for (const name of keyMap[event.code] || []) hold(name, event.code, false);
});
window.addEventListener('blur', releaseAll);
document.addEventListener('visibilitychange', () => { if (document.hidden) releaseAll(); });

for (const button of document.querySelectorAll('.pad button')) {
  const name = button.dataset.button;
  const source = 'pad-' + name;
  const set = (down) => { button.classList.toggle('down', down); hold(name, source, down); };
  button.addEventListener('pointerdown', (event) => {
    event.preventDefault();
    try { button.setPointerCapture(event.pointerId); } catch { /* not capturable */ }
    set(true);
  });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) button.addEventListener(type, () => set(false));
  button.addEventListener('contextmenu', (event) => event.preventDefault());
}

// ---- heads-up display --------------------------------------------------------------------
let lastHudText = '';
let fpsTime = 0, fpsFrames = 0, fpsShown = 0, slowTime = 0;
const bubblePool = [];

function updateHud(dt) {
  const hud = engine.hud();
  const scoreText = digits(Math.round(hud.score));
  if ($('score').textContent !== scoreText) $('score').textContent = scoreText;

  const overlay = $('overlay');
  if (hud.state === 'playing') {
    overlay.hidden = true;
  } else if (overlay.hidden) {
    overlay.hidden = false;
    $('overlay-title').textContent = t(hud.state === 'won' ? 'won' : 'lost');
  }

  const holder = $('bubbles');
  for (let i = 0; i < hud.bubbles.length; ++i) {
    let el = bubblePool[i];
    if (!el) {
      el = document.createElement('div');
      el.className = 'bubble';
      holder.append(el);
      bubblePool.push(el);
    }
    const b = hud.bubbles[i];
    if (el.textContent !== b.text) el.textContent = b.text;
    el.style.left = (b.x * 100).toFixed(2) + '%';
    el.style.top = (b.y * 100).toFixed(2) + '%';
    el.hidden = false;
  }
  for (let i = hud.bubbles.length; i < bubblePool.length; ++i) bubblePool[i].hidden = true;

  fpsTime += dt;
  fpsFrames += 1;
  if (fpsTime >= 0.5) {
    fpsShown = Math.round(fpsFrames / fpsTime);
    fpsTime = 0;
    fpsFrames = 0;
    lastHudText = '';
    // Two slow seconds in a row: lower the quality one step.
    slowTime = fpsShown < 40 ? slowTime + 0.5 : 0;
    if (slowTime >= 2 && fixedQuality < 0 && quality < qualityLevels.length - 1) {
      quality += 1;
      slowTime = 0;
      applyQuality();
    }
  }
  if (lastHudText === '' && fpsShown > 0) {
    $('fps').textContent = digits(t('fps')(fpsShown));
    const s = hud.stats;
    lastHudText = digits(t('stats')({ lights: s.lights, shadows: s.shadowLights, draws: s.draws, size: `${canvas.width}×${canvas.height}${s.hdr ? ' HDR' : ''}` }));
    $('stats').textContent = lastHudText;
  }
}

// ---- the script editor -------------------------------------------------------------------
function renderTabs() {
  const tabs = $('tabs');
  tabs.replaceChildren(...scripts.map((script, index) => {
    const tab = document.createElement('button');
    tab.type = 'button';
    tab.className = 'tab' + (script.draft !== script.installed ? ' edited' : '');
    tab.setAttribute('role', 'tab');
    tab.setAttribute('aria-selected', index === current ? 'true' : 'false');
    tab.textContent = strings[lang].names[script.name] || script.name;
    tab.title = script.name;
    tab.addEventListener('click', () => selectScript(index));
    return tab;
  }));
}

function selectScript(index) {
  current = index;
  code.value = scripts[index].draft;
  statusKind = '';
  renderTabs();
  renderExamples();
  checkDraft();
}

function checkDraft() {
  lastCheck = engine.check(scripts[current].draft);
  if (statusKind !== 'bad' || lastCheck.ok) statusKind = scripts[current].draft !== scripts[current].installed ? 'edited' : (statusKind === 'ok' ? 'ok' : '');
  renderNotes();
  renderStatus();
}

function renderStatus() {
  const status = $('status');
  status.className = 'status' + (statusKind === 'ok' ? ' ok' : statusKind === 'bad' ? ' bad' : '');
  status.textContent =
    statusKind === 'ok' ? t('running') :
    statusKind === 'bad' ? t('notUnderstood')(digits(lastCheck.errors.length)) :
    statusKind === 'edited' ? t('edited') : '';
}

/// The engine writes "condition ⇒ action"; the arrow is drawn, so it looks the same in every font.
function withArrows(text) {
  const parts = text.split(' ⇒ ');
  const out = [];
  parts.forEach((part, i) => {
    if (i > 0) {
      const arrow = document.createElement('span');
      arrow.className = 'arrow';
      arrow.setAttribute('aria-label', '→');
      out.push(arrow);
    }
    out.push(part);
  });
  return out;
}

function renderNotes() {
  const list = $('notes');
  if (!lastCheck) return;
  const lines = scripts[current].draft.split('\n');
  const items = [
    ...lastCheck.notes.map((n) => ({ line: n.line, text: n[lang], error: false })),
    ...lastCheck.errors.map((e) => ({ line: e.line, text: e[lang].replace(/^✗\s*/, ''), error: true })),
  ].sort((a, b) => a.line - b.line || (a.error ? 1 : 0) - (b.error ? 1 : 0));

  if (items.length === 0) {
    const empty = document.createElement('li');
    empty.className = 'empty';
    empty.textContent = t('nothing');
    list.replaceChildren(empty);
    return;
  }
  list.replaceChildren(...items.map((item) => {
    const li = document.createElement('li');
    const nested = /^\s/.test(lines[item.line - 1] || '');
    li.className = (item.error ? 'error' : '') + (nested && !item.error ? ' nested' : '');
    const n = document.createElement('span');
    n.className = 'n';
    n.textContent = digits(item.line);
    const what = document.createElement('span');
    what.className = 'what';
    what.append(...withArrows(item.text));
    li.append(n, what);
    li.addEventListener('click', () => selectLine(item.line));
    return li;
  }));
}

function selectLine(line) {
  const lines = code.value.split('\n');
  let start = 0;
  for (let i = 0; i < line - 1 && i < lines.length; ++i) start += lines[i].length + 1;
  code.focus();
  code.setSelectionRange(start, start + (lines[line - 1] || '').length);
}

function renderExamples() {
  const holder = $('examples');
  const set = (examples[scripts[current].name] || {})[lang] || [];
  holder.previousElementSibling.hidden = set.length === 0;
  holder.replaceChildren(...set.map((text) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'example';
    button.textContent = text;
    button.addEventListener('click', () => {
      const script = scripts[current];
      script.draft = script.draft.replace(/\s*$/, '') + (script.draft.trim() ? '\n' : '') + text + '\n';
      code.value = script.draft;
      code.scrollTop = code.scrollHeight;
      renderTabs();
      checkDraft();
    });
    return button;
  }));
}

let checkTimer = 0;
code.addEventListener('input', () => {
  scripts[current].draft = code.value;
  clearTimeout(checkTimer);
  checkTimer = setTimeout(() => { renderTabs(); checkDraft(); }, 180);
});
// Tab indents (blocks are written with indentation), Ctrl/Cmd+Enter runs.
code.addEventListener('keydown', (event) => {
  if (event.key === 'Tab' && !event.shiftKey) {
    event.preventDefault();
    code.setRangeText('    ', code.selectionStart, code.selectionEnd, 'end');
    code.dispatchEvent(new Event('input'));
  } else if (event.key === 'Enter' && (event.ctrlKey || event.metaKey)) {
    event.preventDefault();
    run();
  }
});

function run() {
  clearTimeout(checkTimer);
  const script = scripts[current];
  script.draft = code.value;
  lastCheck = engine.setScript(current, script.draft);
  if (lastCheck.ok) {
    script.installed = script.draft;
    statusKind = 'ok';
    releaseAll();
    // On a phone the game is above the editor: bring it back into view.
    if (screen.getBoundingClientRect().top < 0) screen.scrollIntoView({ behavior: 'smooth', block: 'start' });
  } else {
    statusKind = 'bad';
    selectLine(lastCheck.errors[0].line);
  }
  renderTabs();
  renderNotes();
  renderStatus();
}

$('run').addEventListener('click', run);
$('reset').addEventListener('click', () => {
  scripts[current].draft = scripts[current].original;
  code.value = scripts[current].draft;
  run();
});
$('restart').addEventListener('click', () => engine && engine.restart());
$('again').addEventListener('click', () => engine && engine.restart());
$('opt-light').addEventListener('change', (event) => engine && engine.option(Option.lighting, event.target.checked ? 1 : 0));
$('opt-bloom').addEventListener('change', (event) => engine && engine.option(Option.bloom, event.target.checked ? 1 : 0));

// ---- the studio: level editor and saved games -------------------------------------------
const brushes = [
  { ch: '#', cls: 'w' }, { ch: '.', cls: '' }, { ch: 'P', cls: 'P' }, { ch: 'G', cls: 'G' },
  { ch: 'c', cls: 'c' }, { ch: '^', cls: 's' }, { ch: 'B', cls: 'B' }, { ch: 'D', cls: 'D' }, { ch: 'L', cls: 'L' },
];
const cellClass = (ch) => { const b = brushes.find((x) => x.ch === ch); return b ? b.cls : ''; };
const cellLabel = (ch) => ('PGc^BDL'.includes(ch) ? ch : '');
let demoLevel = '';
let demoScripts = [];
let levelRows = [];
let brush = '#';
let painting = false;
let painted = '';
let applyTimer = 0;
let toastTimer = 0;

function toast(text) {
  const el = $('toast');
  el.textContent = text;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.textContent = ''; }, 1800);
}

function parseLevel(text) {
  const rows = text.replace(/\r/g, '').split('\n');
  while (rows.length > 1 && rows[rows.length - 1] === '') rows.pop();
  return rows.length ? rows : [''];
}

const levelText = () => levelRows.join('\n') + '\n';

function renderPalette() {
  $('palette').replaceChildren(...brushes.map((b) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'brush';
    button.dataset.brush = b.ch;
    button.setAttribute('aria-pressed', brush === b.ch ? 'true' : 'false');
    const sw = document.createElement('span');
    sw.className = 'sw ' + b.cls;
    const key = document.createElement('span');
    key.className = 'k';
    key.textContent = b.ch;
    const name = document.createElement('span');
    name.textContent = t('brushes')[b.ch] || '';
    button.append(sw, key, name);
    button.addEventListener('click', () => { brush = b.ch; renderPalette(); });
    return button;
  }));
}

function makeCell(ch, r, c) {
  const cell = document.createElement('div');
  cell.className = 'cell ' + cellClass(ch);
  cell.dataset.r = r;
  cell.dataset.c = c;
  cell.textContent = cellLabel(ch);
  return cell;
}

function renderGrid() {
  const grid = $('level-grid');
  const cols = Math.max(1, ...levelRows.map((row) => row.length));
  grid.style.setProperty('--cols', cols);
  const cells = [];
  levelRows.forEach((row, r) => {
    for (let c = 0; c < cols; ++c) cells.push(makeCell(row[c] || '.', r, c));
  });
  grid.replaceChildren(...cells);
}

function patchCell(r, c, ch) {
  const cell = $('level-grid').querySelector(`.cell[data-r="${r}"][data-c="${c}"]`);
  if (!cell) { renderGrid(); return; }
  cell.className = 'cell ' + cellClass(ch);
  cell.textContent = cellLabel(ch);
}

function scheduleApply() {
  clearTimeout(applyTimer);
  applyTimer = setTimeout(() => { if (engine) engine.setLevel(levelText()); }, 350);
}

function setCell(r, c, ch) {
  while (levelRows.length <= r) levelRows.push('');
  if (ch === 'P') {
    levelRows = levelRows.map((row) => row.replace(/P/g, '.'));
    renderGrid();
  }
  let row = levelRows[r];
  while (row.length <= c) row += '.';
  levelRows[r] = row.slice(0, c) + ch + row.slice(c + 1);
  if (ch !== 'P') patchCell(r, c, ch);
  $('level-text').value = levelText();
  scheduleApply();
}

function syncLevelFromEngine() {
  if (!engine) return;
  levelRows = parseLevel(engine.level());
  $('level-text').value = levelText();
  renderGrid();
}

// painting: tap or drag; each cell becomes the chosen brush
const gridEl = $('level-grid');
function paintAt(event) {
  const el = document.elementFromPoint(event.clientX, event.clientY);
  if (!el || !el.classList || !el.classList.contains('cell')) return;
  const key = el.dataset.r + ',' + el.dataset.c;
  if (key === painted) return;
  painted = key;
  setCell(Number(el.dataset.r), Number(el.dataset.c), brush);
}
gridEl.addEventListener('pointerdown', (event) => {
  event.preventDefault();
  painting = true;
  painted = '';
  try { gridEl.setPointerCapture(event.pointerId); } catch { /* not capturable */ }
  paintAt(event);
});
gridEl.addEventListener('pointermove', (event) => { if (painting) paintAt(event); });
for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
  gridEl.addEventListener(type, () => { painting = false; painted = ''; });
}
gridEl.addEventListener('contextmenu', (event) => event.preventDefault());

$('level-text').addEventListener('input', () => {
  levelRows = parseLevel($('level-text').value);
  renderGrid();
  scheduleApply();
});

// the two panels: drawing the level, and writing behaviour
function showPanel(which) {
  const level = which === 'level';
  $('level-panel').hidden = !level;
  $('behavior-panel').hidden = level;
  $('panel-level').setAttribute('aria-selected', level ? 'true' : 'false');
  $('panel-behavior').setAttribute('aria-selected', level ? 'false' : 'true');
  if (level) syncLevelFromEngine();
}
$('panel-level').addEventListener('click', () => showPanel('level'));
$('panel-behavior').addEventListener('click', () => showPanel('behavior'));

// games are { level, scripts: [{name, source}] } — in the browser, and as a .json file
function gameState() {
  return { app: 'g-engine', version: 1, level: levelText(), scripts: scripts.map((s) => ({ name: s.name, source: s.draft })) };
}

function loadGameState(state) {
  if (!engine || !state || typeof state.level !== 'string' || !Array.isArray(state.scripts)) return false;
  engine.setLevel(state.level.endsWith('\n') ? state.level : state.level + '\n');
  scripts = engine.scripts().map((s) => {
    const found = state.scripts.find((x) => x && x.name === s.name);
    const source = found && typeof found.source === 'string' ? found.source : s.source;
    return { name: s.name, original: source, installed: source, draft: source };
  });
  scripts.forEach((s, i) => {
    if (!engine.setScript(i, s.source).ok) {
      const fallback = engine.scripts()[i].source;
      scripts[i] = { name: s.name, original: fallback, installed: fallback, draft: fallback };
    }
  });
  syncLevelFromEngine();
  selectScript(0);
  return true;
}

function readSavedGame() {
  try {
    const raw = localStorage.getItem('ge-game');
    return raw ? JSON.parse(raw) : null;
  } catch {
    return null;
  }
}

const starterLevel = () => [
  '########################',
  '#......................#',
  '#......................#',
  '#......................#',
  '#.......L..............#',
  '#......................#',
  '#..P...................#',
  '########################',
].join('\n') + '\n';

const starterScripts = () => [
  { name: 'খেলোয়াড়', source: '# খেলোয়াড় — নিয়ম এখানে লেখো\nডান বোতাম চাপলে ডানে হাঁটো\nবাম বোতাম চাপলে বামে হাঁটো\nলাফ বোতাম চাপলে লাফ দাও\n' },
  { name: 'প্রহরী', source: '# প্রহরী\n' },
  { name: 'কয়েন', source: '# কয়েন\nখেলোয়াড়কে ছুঁলে মুছে যাও\n' },
  { name: 'দরজা', source: '# দরজা\n' },
  { name: 'বাতি', source: '# বাতি\n' },
  { name: 'বাক্স', source: '# বাক্স\n' },
];

$('new-game').addEventListener('click', () => {
  if (!engine) return;
  loadGameState({ level: starterLevel(), scripts: starterScripts() });
  applyLanguage();
  toast(t('loaded'));
});
$('sample-game').addEventListener('click', () => {
  if (!engine) return;
  engine.setLevel(demoLevel);
  scripts = demoScripts.map((s) => ({ name: s.name, original: s.source, installed: s.source, draft: s.source }));
  scripts.forEach((s, i) => engine.setScript(i, s.source));
  syncLevelFromEngine();
  selectScript(0);
  applyLanguage();
  toast(t('loaded'));
});
$('save-game').addEventListener('click', () => {
  if (!engine) return;
  try {
    localStorage.setItem('ge-game', JSON.stringify(gameState()));
    toast(t('saved'));
  } catch {
    toast(t('saveFailed'));
  }
});
$('export-game').addEventListener('click', () => {
  if (!engine) return;
  const blob = new Blob([JSON.stringify(gameState(), null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'g-engine-game.json';
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 4000);
});
$('import-game').addEventListener('click', () => $('import-file').click());
$('import-file').addEventListener('change', async (event) => {
  const file = event.target.files[0];
  event.target.value = '';
  if (!file) return;
  try {
    const ok = loadGameState(JSON.parse(await file.text()));
    if (ok) {
      applyLanguage();
      toast(t('loaded'));
    } else {
      toast(t('badFile'));
    }
  } catch {
    toast(t('badFile'));
  }
});

// ---- start -------------------------------------------------------------------------------
async function start() {
  applyLanguage();
  const message = $('message');
  message.textContent = t('loading');
  resizeCanvas();
  try {
    engine = await loadEngine(canvas);
    if (!engine.init(canvas.width, canvas.height)) throw new Error('init-failed');
  } catch (error) {
    console.error(error);
    engine = null;
    message.classList.add('error');
    message.textContent = t(error.message === 'webgl2-unavailable' ? 'noWebgl' : error.message === 'wasm-missing' ? 'noWasm' : 'failed');
    return;
  }
  message.textContent = '';

  applyQuality();

  demoLevel = engine.level();
  demoScripts = engine.scripts().map((s) => ({ name: s.name, source: s.source }));
  scripts = demoScripts.map((s) => ({ name: s.name, original: s.source, installed: s.source, draft: s.source }));
  new ResizeObserver(resizeCanvas).observe(screen);
  window.addEventListener('resize', resizeCanvas);
  const saved = readSavedGame();
  if (saved && loadGameState(saved)) {
    toast(t('loaded'));
  } else {
    selectScript(0);
  }
  showPanel('level');
  applyLanguage();

  // Real elapsed time (not the timestamp handed to the callback, which some browsers
  // advance by a fixed 1/60 s even when frames take longer).
  let last = performance.now();
  const frame = () => {
    const now = performance.now();
    const elapsed = (now - last) / 1000;
    last = now;
    engine.frame(Math.min(elapsed, 0.1));
    if (!document.hidden && elapsed < 1) updateHud(elapsed);
    requestAnimationFrame(frame);
  };
  requestAnimationFrame(frame);

  // For tests and for the curious (try `ge.check("লাফ দাও")` in the console).
  window.ge = engine;
}

start();
