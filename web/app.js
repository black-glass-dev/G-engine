// G-Engine — web/app.js
// The page around the engine: canvas size, keyboard and touch buttons, the script editor.

import { loadEngine, Button, Option } from './engine.js';
import { examples } from './examples.js';

const $ = (id) => document.getElementById(id);

// ---- words of the page, in two languages -------------------------------------------------
const strings = {
  bn: {
    tagline: 'সহজ বাংলা বা ইংরেজি বাক্যে গেম বানাও',
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
    editorTitle: 'কথায় লেখো, গেম সেটাই করবে',
    editorLead: 'নিচের বাক্যগুলো বদলাও, তারপর «চালাও» চাপো। বাংলা আর ইংরেজি — দুটোই চলে।',
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
    footer: 'G-Engine — C++ ইঞ্জিন (WebAssembly), নিজস্ব আলো-ছায়ার রেন্ডারার, Box2D পদার্থবিদ্যা, আর «কথা» ভাষা। এই পাতায় যা চলছে, অ্যান্ড্রয়েডেও সেই একই কোড চলবে।',
    labels: { left: 'বামে', right: 'ডানে', jump: 'লাফ', action: 'অ্যাকশন' },
    names: {},
  },
  en: {
    tagline: 'Make games with plain Bengali or English sentences',
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
    editorTitle: 'Say it in words, the game does it',
    editorLead: 'Change the sentences below, then press Run. Bengali and English both work.',
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
    footer: 'G-Engine — a C++ engine (WebAssembly), its own lighting renderer, Box2D physics and the "Kotha" language. The code running on this page is the same code that will run on Android.',
    labels: { left: 'Left', right: 'Right', jump: 'Jump', action: 'Action' },
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

  scripts = engine.scripts().map((s) => ({ name: s.name, original: s.source, installed: s.source, draft: s.source }));
  new ResizeObserver(resizeCanvas).observe(screen);
  window.addEventListener('resize', resizeCanvas);
  selectScript(0);
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
