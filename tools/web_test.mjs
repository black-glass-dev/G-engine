// G-Engine — tools/web_test.mjs
// Opens the web player in a real (headless) browser and checks that it works: the engine starts
// without WebGL errors, keyboard and touch buttons move the player, scripts can be edited,
// broken scripts are refused with a message, and every example sentence on the page compiles.
//
//   cd tools && npm install            (once; or set CHROME_PATH to an installed Chrome)
//   python3 tools/serve.py 8080 &      (the page must be served)
//   node tools/web_test.mjs [url] [--shots <folder>]
//
// GE_NODE_MODULES=<folder with node_modules> lets the script use packages installed elsewhere.

import { createRequire } from 'node:module';
import { existsSync, mkdirSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
const shotsAt = args.indexOf('--shots');
const shots = shotsAt >= 0 ? args[shotsAt + 1] : null;
const url = args.find((a, i) => !a.startsWith('--') && i !== shotsAt + 1) || 'http://localhost:8080/';
if (shots) mkdirSync(shots, { recursive: true });

const require = createRequire(join(process.env.GE_NODE_MODULES || here, 'package.json'));
const load = async (name) => import(pathToFileURL(require.resolve(name)).href);

async function launch() {
  const puppeteer = (await load('puppeteer-core')).default;
  const common = ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'];
  if (process.env.CHROME_PATH) {
    return puppeteer.launch({ executablePath: process.env.CHROME_PATH, headless: true, args: common });
  }
  // A Chromium that ships inside an npm package: handy on machines without a browser.
  const sparticuz = await load('@sparticuz/chromium');
  const chromium = sparticuz.default;
  const libs = '/tmp/al2023/lib';
  if (!existsSync(libs)) await sparticuz.inflate(join(dirname(require.resolve('@sparticuz/chromium')), '..', 'bin', 'al2023.tar.br'));
  process.env.LD_LIBRARY_PATH = libs + (process.env.LD_LIBRARY_PATH ? ':' + process.env.LD_LIBRARY_PATH : '');
  return puppeteer.launch({ executablePath: await chromium.executablePath(), headless: 'shell', args: chromium.args });
}

let failures = 0;
const check = (ok, what, detail = '') => {
  if (!ok) ++failures;
  console.log(`${ok ? '  ok  ' : ' FAIL '} ${what}${detail ? '  — ' + detail : ''}`);
};
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

const browser = await launch();
const problems = [];

async function open(width, height, dpr, touch, query = '') {
  const page = await browser.newPage();
  await page.setViewport({ width, height, deviceScaleFactor: dpr, hasTouch: touch, isMobile: touch });
  page.on('console', (m) => {
    const text = m.text();
    if (m.type() === 'error' || /WebGL|GL_INVALID|INVALID_/.test(text)) problems.push(text);
    if (process.env.GE_VERBOSE) console.log('    [page]', text);
  });
  page.on('pageerror', (e) => problems.push('pageerror: ' + e.message));
  page.on('requestfailed', (r) => problems.push('request failed: ' + r.url()));
  await page.goto(url + (query || '?quality=1'), { waitUntil: 'load' });
  await page.waitForFunction('window.ge !== undefined', { timeout: 20000 });
  await sleep(600);
  return page;
}
const hud = (page) => page.evaluate(() => window.ge.hud());
/// Waits for game time, not wall-clock time: a software-rendered test browser can be very slow.
const play = async (page, seconds) => {
  const target = (await hud(page)).time + seconds;
  await page.waitForFunction((t) => window.ge.hud().time >= t, { timeout: 60000, polling: 30 }, target);
};
/// Pictures are taken with the speed readout hidden: a test browser draws in software, so the
/// number it shows says nothing about a phone.
const picture = async (page, take) => {
  if (!shots) return;
  await page.evaluate(() => { document.getElementById('fps').style.visibility = 'hidden'; });
  await take();
  await page.evaluate(() => { document.getElementById('fps').style.visibility = ''; });
};
const shot = (page, name) => picture(page, () => page.screenshot({ path: join(shots, name) }));
const shotGame = (page, name) => picture(page, async () => (await page.$('#screen')).screenshot({ path: join(shots, name) }));

// ------------------------------------------------------------------------------------------
console.log('desktop');
{
  const page = await open(1280, 800, 1, false);
  const glError = await page.evaluate(() => window.ge.gl.getError());
  check(glError === 0, 'no WebGL error after start', 'gl.getError() = ' + glError);
  let h = await hud(page);
  check(h.state === 'playing' && h.stats.sprites > 100 && h.stats.lights >= 2, 'the demo is running', JSON.stringify(h.stats));
  check(h.stats.draws <= 16, 'a frame takes few draw calls', String(h.stats.draws));
  await shot(page, 'desktop-start.png');

  // keyboard (the jump first: at the starting place there is nothing overhead, wherever a
  // slow test machine lets the walk end)
  const y0 = h.player.y;
  await page.keyboard.down('Space');
  await play(page, 0.25);
  const yUp = (await hud(page)).player.y;
  await page.keyboard.up('Space');
  check(yUp > y0 + 0.5, 'space jumps', `${y0.toFixed(2)} -> ${yUp.toFixed(2)}`);
  await play(page, 0.9);
  const x0 = (await hud(page)).player.x;
  await page.keyboard.down('ArrowRight');
  await play(page, 0.7);
  await page.keyboard.up('ArrowRight');
  h = await hud(page);
  check(h.player.x > x0 + 2.0, 'the right arrow key walks right', `${x0.toFixed(2)} -> ${h.player.x.toFixed(2)}`);
  await play(page, 0.3);

  // every example on the page compiles
  const bad = await page.evaluate(async () => {
    const { examples } = await import('./examples.js');
    const names = window.ge.scripts().map((s) => s.name);
    const out = [];
    for (const [name, sets] of Object.entries(examples)) {
      if (!names.includes(name)) out.push(`no script called ${name}`);
      for (const lang of ['bn', 'en']) {
        for (const text of sets[lang]) {
          const result = window.ge.check(text);
          if (!result.ok) out.push(`${text}  =>  ${result.errors[0].en}`);
          if (result.notes.length === 0) out.push(`${text}  =>  understood nothing`);
        }
      }
    }
    return out;
  });
  check(bad.length === 0, 'all example sentences compile', bad.join(' | '));

  // the rest writes scripts, so the behaviour panel must be open
  await page.click('#panel-behavior');

  // editing: a script the engine understands
  await page.evaluate(() => {
    const code = document.getElementById('code');
    code.value = 'ডানে হাঁটো\n';
    code.dispatchEvent(new Event('input'));
  });
  await page.click('#run');
  const start = (await hud(page)).player.x;
  await play(page, 0.9);
  h = await hud(page);
  const status = await page.$eval('#status', (el) => el.className);
  check(status.includes('ok') && h.player.x > start + 2.5, 'a new script runs after pressing Run', `x ${start.toFixed(2)} -> ${h.player.x.toFixed(2)}`);

  // editing: a script it does not understand is refused, with a message, and the game keeps running
  await page.evaluate(() => {
    const code = document.getElementById('code');
    code.value = 'ডানে হাঁটো\nআকাশে উড়ে যাওও\n';
    code.dispatchEvent(new Event('input'));
  });
  await page.click('#run');
  await sleep(300);
  const errors = await page.$$eval('#notes li.error', (items) => items.map((li) => li.textContent));
  const badStatus = await page.$eval('#status', (el) => el.className);
  const before = (await hud(page)).player.x;
  await play(page, 0.4);
  check(errors.length === 1 && badStatus.includes('bad'), 'a line that is not understood is reported', errors.join(' | '));
  const installed = await page.evaluate(() => window.ge.scripts()[0].source);
  check(installed === 'ডানে হাঁটো\n' && Math.abs((await hud(page)).player.x - before) > 0.2, 'the previous script keeps running meanwhile');
  await shot(page, 'desktop-error.png');

  // restore, switch tab, language
  await page.click('#reset');
  await sleep(200);
  const restored = await page.$eval('#code', (el) => el.value);
  check(restored.includes('বোতাম') && (await page.$$eval('#notes li.error', (i) => i.length)) === 0, 'restoring the original script works');
  await page.click('#tabs .tab:nth-child(2)');
  check((await page.$eval('#code', (el) => el.value)).includes('বারবার'), 'the second tab shows the guard script');
  await page.click('#lang');
  const english = await page.$eval('#run', (el) => el.textContent.trim());
  const note = await page.$eval('#notes li .what', (el) => el.textContent);
  const tabNames = await page.$$eval('#tabs .tab', (tabs) => tabs.map((tab) => tab.textContent));
  check(english === 'Run' && /forever/.test(note) && tabNames[0] === 'Player' && tabNames[1] === 'Guard', 'the page switches to English', `${english} / ${note} / ${tabNames.join(', ')}`);
  await shot(page, 'desktop-english.png');
  await page.click('#lang');

  // lighting off / on (for the comparison picture)
  await page.click('#restart');
  await play(page, 1.5);
  await shotGame(page, 'game-lit.png');
  await page.click('#opt-light');
  await sleep(300);
  await shotGame(page, 'game-unlit.png');
  await page.click('#opt-light');
  check((await page.evaluate(() => window.ge.gl.getError())) === 0, 'no WebGL error after all of that');

  // a different level picture: 40 lamps, 32 of them casting shadows at the same time
  const original = await page.evaluate(() => window.ge.level());
  const many = await page.evaluate(() => {
    const rows = ['#'.repeat(42)];
    for (let r = 0; r < 8; ++r) {
      const row = ('#' + '.'.repeat(40) + '#').split('');
      if (r % 2 === 1) for (let i = 0; i < 10; ++i) row[2 + i * 4] = 'L';
      if (r === 6) for (const x of [9, 10, 11, 12, 27, 28, 29]) row[x] = '#';
      if (r === 7) { row[20] = 'P'; row[16] = 'B'; row[24] = 'G'; }
      rows.push(row.join(''));
    }
    rows.push('#'.repeat(42));
    window.ge.setLevel(rows.join('\n') + '\n');
    return rows.length;
  });
  await play(page, 0.5);
  h = await hud(page);
  check(many === 10 && h.stats.lights > 32 && h.stats.shadowLights === 32 && h.stats.draws <= 16, '32 lights cast shadows at the same time', JSON.stringify(h.stats));
  check((await page.evaluate(() => window.ge.gl.getError())) === 0, 'no WebGL error with that many lights');
  await shotGame(page, 'many-lights.png');
  await page.evaluate((picture) => window.ge.setLevel(picture), original);
  await play(page, 0.3);
  h = await hud(page);
  check((await page.evaluate(() => window.ge.level())) === original && h.stats.shadowLights <= 4, 'the original level comes back', JSON.stringify(h.stats));

  // winning shows the message and "play again" starts over
  await page.click('#tabs .tab:nth-child(1)');
  await page.evaluate(() => {
    const code = document.getElementById('code');
    code.value = 'শুরুতে ১ সেকেন্ড অপেক্ষা করে খেলা জিতে যাও\n';
    code.dispatchEvent(new Event('input'));
  });
  await page.click('#run');
  await page.waitForFunction(() => window.ge.hud().state === 'won', { timeout: 60000, polling: 30 });
  await sleep(400);
  const banner = await page.evaluate(() => {
    const el = document.getElementById('overlay');
    return { shown: el !== null && !el.hidden && getComputedStyle(el).display !== 'none', text: el ? el.textContent.trim() : '' };
  });
  check(banner.shown && banner.text.includes('জিতে'), 'winning shows the message', JSON.stringify(banner));
  await shot(page, 'desktop-won.png');
  await page.click('#again');
  await play(page, 0.2);
  h = await hud(page);
  const hidden = await page.evaluate(() => { const el = document.getElementById('overlay'); return el.hidden || getComputedStyle(el).display === 'none'; });
  check(h.state === 'playing' && hidden, '"play again" starts over', h.state);
  await page.click('#reset');
  await page.close();
}

// ------------------------------------------------------------------------------------------
console.log('phone');
{
  const page = await open(390, 780, 2, true);
  let h = await hud(page);
  await page.click('#panel-behavior');
  const layout = await page.evaluate(() => {
    const r = (id) => { const b = document.getElementById(id).getBoundingClientRect(); return { top: b.top, bottom: b.bottom }; };
    return { screen: r('screen'), pad: r('pad'), code: r('code'), scroll: document.documentElement.scrollWidth, width: innerWidth };
  });
  check(layout.scroll <= layout.width, 'no sideways scrolling on a phone', `${layout.scroll} <= ${layout.width}`);
  check(layout.pad.top >= layout.screen.bottom && layout.code.top > layout.pad.bottom, 'game, then buttons, then the script');
  await shot(page, 'phone-start.png');

  // a finger on the on-screen buttons
  const press = async (name, ms) => {
    const box = await (await page.$(`.pad button[data-button="${name}"]`)).boundingBox();
    const x = box.x + box.width / 2, y = box.y + box.height / 2;
    await page.touchscreen.touchStart(x, y);
    await play(page, ms / 1000);
    await page.touchscreen.touchEnd();
  };
  const y0 = h.player.y;
  const box = await (await page.$('.pad button[data-button="jump"]')).boundingBox();
  await page.touchscreen.touchStart(box.x + box.width / 2, box.y + box.height / 2);
  await play(page, 0.25);
  const yUp = (await hud(page)).player.y;
  await page.touchscreen.touchEnd();
  check(yUp > y0 + 0.5, 'the on-screen jump button jumps', `${y0.toFixed(2)} -> ${yUp.toFixed(2)}`);
  await play(page, 0.9);
  const x0 = (await hud(page)).player.x;
  await press('right', 700);
  h = await hud(page);
  check(h.player.x > x0 + 2.0, 'the on-screen right button walks right', `${x0.toFixed(2)} -> ${h.player.x.toFixed(2)}`);
  await play(page, 0.3);
  const x1 = (await hud(page)).player.x;
  await play(page, 0.4);
  check(Math.abs((await hud(page)).player.x - x1) < 0.05, 'lifting the finger stops the player');
  await shotGame(page, 'phone-game.png');
  await page.evaluate(() => document.querySelector('.editor').scrollIntoView());
  await sleep(200);
  await shot(page, 'phone-editor.png');
  await page.close();
}

// ------------------------------------------------------------------------------------------
console.log('studio');
{
  const page = await open(1280, 820, 1, false);
  await play(page, 0.3);
  check(await page.$eval('#level-panel', (el) => !el.hidden), 'the level editor is what you see first');

  // painting the map changes the game
  await page.click('[data-brush="c"]');
  await page.click('.cell[data-r="2"][data-c="4"]');
  await page.click('.cell[data-r="2"][data-c="5"]');
  await new Promise((r) => setTimeout(r, 800));
  const rows = (await page.evaluate(() => window.ge.level())).split('\n');
  check(rows[2][4] === 'c' && rows[2][5] === 'c', 'painted cells change the level', rows[2]);

  // a new game starts from a blank room
  await page.click('#new-game');
  await new Promise((r) => setTimeout(r, 500));
  const blank = await page.evaluate(() => window.ge.level());
  const blankRows = blank.split('\n').filter(Boolean);
  check(blankRows.length === 8 && blankRows[0].startsWith('###') && blank.includes('P'), 'new game makes a blank room', `${blankRows.length} rows`);

  // saving puts the game in the browser; a reload brings it back
  await page.click('#save-game');
  await page.reload({ waitUntil: 'domcontentloaded' });
  for (let i = 0; i < 40 && !(await page.evaluate(() => !!window.ge)); ++i) await new Promise((r) => setTimeout(r, 250));
  await play(page, 0.3);
  const restored = await page.evaluate(() => window.ge.level());
  check(restored === blank, 'the saved game comes back after a reload', `${restored.split('\n').filter(Boolean).length} rows`);
  check((await page.evaluate(() => window.ge.hud().state)) === 'playing', 'the restored game is running');
  check((await page.evaluate(() => window.ge.gl.getError())) === 0, 'no WebGL error in the studio');
}

// ------------------------------------------------------------------------------------------
check(problems.length === 0, 'no errors or WebGL warnings in the browser console', problems.slice(0, 5).join(' | '));
await browser.close();
console.log(failures === 0 ? '\nall web checks passed' : `\n${failures} web check(s) FAILED`);
process.exit(failures === 0 ? 0 : 1);
