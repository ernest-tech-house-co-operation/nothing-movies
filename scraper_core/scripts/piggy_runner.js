'use strict';

// This is the ONLY JS file in the project. It exists purely as a thin,
// persistent bridge between scraper_core (C++) and the real `nothing-browser`
// (piggy) client library, which is the actual tested implementation of the
// Nothing Browser wire protocol. Source plugins never see this file or
// touch JS at all — they only ever call scraper_core's C++ API.
//
// Protocol (JSON, one object per line, both directions):
//   in:  { id, action: "launch",   opts }
//   in:  { id, action: "register", name, url }
//   in:  { id, action: "call",     site, method, args: [...] }
//   in:  { id, action: "close" }
//   out: { id, ok: true,  data }
//   out: { id, ok: false, error }

const piggy = require('nothing-browser');
const readline = require('readline');

const rl = readline.createInterface({ input: process.stdin, terminal: false });

// Resolve a dot-path method against an object, e.g. "provide.attr" -> site.provide.attr
function resolveMethod(obj, path) {
  const parts = path.split('.');
  let ctx = obj;
  for (let i = 0; i < parts.length - 1; i++) ctx = ctx[parts[i]];
  const fn = ctx[parts[parts.length - 1]];
  if (typeof fn !== 'function') {
    throw new Error(`method not found: ${path}`);
  }
  return { fn, ctx };
}

function reply(id, ok, dataOrError) {
  const msg = ok ? { id, ok: true, data: dataOrError } : { id, ok: false, error: String(dataOrError) };
  process.stdout.write(JSON.stringify(msg) + '\n');
}

async function handle(msg) {
  const { id, action } = msg;
  try {
    let data;
    switch (action) {
      case 'launch':
        await piggy.launch(msg.opts || { mode: 'tab', binary: 'headless' });
        data = true;
        break;

      case 'register':
        await piggy.register(msg.name, msg.url);
        data = true;
        break;

      case 'call': {
        const site = piggy.usePiggy(msg.site);
        const { fn, ctx } = resolveMethod(site, msg.method);
        data = await fn.apply(ctx, msg.args || []);
        break;
      }

      case 'close':
        await piggy.close();
        data = true;
        break;

      default:
        throw new Error('unknown action: ' + action);
    }
    reply(id, true, data);
  } catch (e) {
    reply(id, false, e.message || String(e));
  }
}

rl.on('line', (line) => {
  const trimmed = line.trim();
  if (!trimmed) return;
  let msg;
  try {
    msg = JSON.parse(trimmed);
  } catch {
    return; // ignore malformed lines rather than crashing the runner
  }
  handle(msg);
});

// If our stdin closes (C++ side exited/killed us), shut piggy down cleanly.
rl.on('close', async () => {
  try { await piggy.close({ force: true }); } catch { /* already gone */ }
  process.exit(0);
});